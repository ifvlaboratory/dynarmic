/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>

#include <dynarmic/A32/config.h>

#include "common/assert.h"
#include "common/bit_util.h"
#include "frontend/imm.h"
#include "frontend/A32/decoder/thumb16.h"
#include "frontend/A32/decoder/thumb32.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/impl/translate_thumb.h"
#include "frontend/A32/translate/translate.h"

namespace Dynarmic::A32 {
namespace {

enum class ThumbInstSize {
    Thumb16, Thumb32
};

bool IsThumb16(u16 first_part) {
    return (first_part & 0xF800U) < 0xE800;
}

std::tuple<u32, ThumbInstSize> ReadThumbInstruction(u32 arm_pc, MemoryReadCodeFuncType memory_read_code) {
    u32 first_part = memory_read_code(arm_pc & 0xFFFFFFFEU, true);

    if (IsThumb16(static_cast<u16>(first_part))) {
        // 16-bit thumb instruction
        return std::make_tuple(first_part, ThumbInstSize::Thumb16);
    }

    // 32-bit thumb instruction
    // These always start with 0b11101, 0b11110 or 0b11111.
    u32 second_part = memory_read_code((arm_pc + 2) & 0xFFFFFFFEU, true);
    return std::make_tuple(static_cast<u32>((first_part << 16U) | second_part), ThumbInstSize::Thumb32);
}

} // local namespace

static bool CondCanContinue(ConditionalState cond_state, const A32::IREmitter& ir) {
    ASSERT_MSG(cond_state != ConditionalState::Break, "Should never happen.");
    if (cond_state == ConditionalState::None) {
        return true;
    }

    // TODO: This is more conservative than necessary.
    return std::all_of(ir.block.begin(), ir.block.end(), [](const IR::Inst& inst) { return !inst.WritesToCPSR(); });
}

IR::Block TranslateThumb(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, const TranslationOptions& options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    ThumbTranslatorVisitor visitor{block, descriptor, options};

    bool should_continue = true;
    do {
        const u32 arm_pc = visitor.ir.current_location.PC();
        const auto [thumb_instruction, inst_size] = ReadThumbInstruction(arm_pc, memory_read_code);

        if (inst_size == ThumbInstSize::Thumb16) {
            visitor.is_thumb_16 = true;
            if (const auto decoder = DecodeThumb16<ThumbTranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
                should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
            } else {
                should_continue = visitor.thumb16_UDF();
            }
        } else {
            visitor.is_thumb_16 = false;
            if (const auto decoder = DecodeThumb32<ThumbTranslatorVisitor>(thumb_instruction)) {
                should_continue = decoder->get().call(visitor, thumb_instruction);
            } else {
                should_continue = visitor.thumb32_UDF();
            }
        }
        
        if (visitor.cond_state == ConditionalState::Break) {
            break;
        }

        const s32 advance_pc = (inst_size == ThumbInstSize::Thumb16) ? 2 : 4;
        visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
        block.CycleCount()++;

        if (visitor.ir.current_location.IT().IsInITBlock()) {
            visitor.ir.current_location = visitor.ir.current_location.AdvanceIT();
        }
    } while (should_continue && CondCanContinue(visitor.cond_state, visitor.ir) && !single_step);

    if (visitor.cond_state == ConditionalState::Translating || visitor.cond_state == ConditionalState::Trailing || single_step) {
        if (should_continue) {
            if (single_step) {
                visitor.ir.SetTerm(IR::Term::LinkBlock{visitor.ir.current_location});
            } else {
                visitor.ir.SetTerm(IR::Term::LinkBlockFast{visitor.ir.current_location});
            }
        }
    }
    
    ASSERT_MSG(block.HasTerminal(), "Terminal has not been set");

    block.SetEndLocation(visitor.ir.current_location);

    return block;
}

bool TranslateSingleThumbInstruction(IR::Block& block, LocationDescriptor descriptor, u32 thumb_instruction) {
    ThumbTranslatorVisitor visitor{block, descriptor, {}};

    const bool is_thumb_16 = IsThumb16(thumb_instruction >> 16);
    bool should_continue = true;
    visitor.is_thumb_16 = is_thumb_16;
    if (is_thumb_16) {
        if (const auto decoder = DecodeThumb16<ThumbTranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
            should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
        } else {
            should_continue = visitor.thumb16_UDF();
        }
    } else {
        if (const auto decoder = DecodeThumb32<ThumbTranslatorVisitor>(thumb_instruction)) {
            should_continue = decoder->get().call(visitor, thumb_instruction);
        } else {
            should_continue = visitor.thumb32_UDF();
        }
    }

    const s32 advance_pc = is_thumb_16 ? 2 : 4;
    visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
    block.CycleCount()++;

    block.SetEndLocation(visitor.ir.current_location);

    return should_continue;
}

bool ThumbTranslatorVisitor::ConditionPassed() {
    Cond cond;
    const auto it = ir.current_location.IT();
    if (!it.IsInITBlock()) {
       cond = Cond::AL;
    } else {
       cond = it.Cond();
    }
    
    // Do we need to end this block and try again with a new block?
    bool should_stop = false;
    // Are we emitting an instruction to conditional part of this block?
    bool step_cond = false;
    
    switch (cond_state) {
        case ConditionalState::None: {
            if (cond == Cond::AL) {
                // Unconditional
                should_stop = false;
                break;
            }
            // No AL cond
            if (!ir.block.empty()) {
                // Give me an empty block
                should_stop = true;
                break;
            }
            // We've not emitted instructions yet.
            // We'll emit one instruction, and set the block-entry conditional appropriately.
            cond_state = ConditionalState::Translating;
            ir.block.SetCondition(cond);
            step_cond = true;
            break;
        }
        case ConditionalState::Trailing: {
            if (cond == Cond::AL) {
                should_stop = false;
                break;
            }
            // No AL cond
            if (!ir.block.empty()) {
                should_stop = true;
                break;
            }
            break;
        }
        case ConditionalState::Translating: {
            // Jump inside conditional block
            if (ir.block.ConditionFailedLocation() != ir.current_location) {
                cond_state = ConditionalState::Trailing;
                should_stop = !ir.block.empty();
                break;
            }
            // Try adding unconditional instructions to end of this block
            // not stepping the conditional
            if (cond == Cond::AL) {
                cond_state = ConditionalState::Trailing;
                should_stop = false;
                break;
            }
            // cond has changed, abort
            if (cond != ir.block.GetCondition()) {
                should_stop = true;
                break;
            }
            step_cond = true;
            break;
        }
        default: {
            ASSERT_MSG(cond_state != ConditionalState::Break,
                             "This should never happen. We requested a break but that wasn't honored.");
        }
    }

    if (step_cond) {
        auto next_failed_location = ir.current_location;
        if (is_thumb_16) {
            next_failed_location = next_failed_location.AdvancePC(2);
        } else {
            next_failed_location = next_failed_location.AdvancePC(4);
        }
        if (it.IsInITBlock()) {
            next_failed_location = next_failed_location.AdvanceIT();
        }
        ir.block.ConditionFailedCycleCount() = ir.block.CycleCount() + 1;
        ir.block.SetConditionFailedLocation(next_failed_location);
    }
    
    if (should_stop) {
        cond_state = ConditionalState::Break;
        ir.SetTerm(IR::Term::LinkBlockFast{ir.current_location});
        return false;
    }
    
    return true;
}

bool ThumbTranslatorVisitor::InterpretThisInstruction() {
    ir.SetTerm(IR::Term::Interpret(ir.current_location));
    return false;
}

bool ThumbTranslatorVisitor::UnpredictableInstruction() {
    ir.ExceptionRaised(Exception::UnpredictableInstruction);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool ThumbTranslatorVisitor::UndefinedInstruction() {
    ir.ExceptionRaised(Exception::UndefinedInstruction);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool ThumbTranslatorVisitor::RaiseException(Exception exception) {
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 2)); // TODO: T32
    ir.ExceptionRaised(exception);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

} // namespace Dynarmic::A32
