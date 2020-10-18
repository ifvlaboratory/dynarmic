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
    return (first_part & 0xF800) < 0xE800;
}

u32 last_pc = (u32) -1;
u32 last_code;

u32 ReadCode(u32 pc, MemoryReadCodeFuncType memory_read_code) {
    if(pc == last_pc) {
        return last_code;
    }

    u32 code = memory_read_code(pc);
    last_pc = pc;
    last_code = code;
    return code;
}

std::tuple<u32, ThumbInstSize> ReadThumbInstruction(u32 arm_pc, MemoryReadCodeFuncType memory_read_code) {
    u32 first_part = ReadCode(arm_pc & 0xFFFFFFFC, memory_read_code);
    if ((arm_pc & 0x2) != 0) {
        first_part >>= 16;
    }
    first_part &= 0xFFFF;

    if (IsThumb16(static_cast<u16>(first_part))) {
        // 16-bit thumb instruction
        return std::make_tuple(first_part, ThumbInstSize::Thumb16);
    }

    // 32-bit thumb instruction
    // These always start with 0b11101, 0b11110 or 0b11111.

    u32 second_part = ReadCode((arm_pc + 2) & 0xFFFFFFFC, memory_read_code);
    if (((arm_pc + 2) & 0x2) != 0) {
        second_part >>= 16;
    }
    second_part &= 0xFFFF;

    return std::make_tuple(static_cast<u32>((first_part << 16) | second_part), ThumbInstSize::Thumb32);
}

} // local namespace

IR::Block TranslateThumb(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, const TranslationOptions& options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    ThumbTranslatorVisitor visitor{block, descriptor, options};

    bool should_continue = true;
    do {
        const u32 arm_pc = visitor.ir.current_location.PC();
        const auto [thumb_instruction, inst_size] = ReadThumbInstruction(arm_pc, memory_read_code);

        if (inst_size == ThumbInstSize::Thumb16) {
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

        const s32 advance_pc = (inst_size == ThumbInstSize::Thumb16) ? 2 : 4;
        visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
        block.CycleCount()++;
    } while (should_continue && !single_step);

    if (single_step && should_continue) {
        visitor.ir.SetTerm(IR::Term::LinkBlock{visitor.ir.current_location});
    }

    block.SetEndLocation(visitor.ir.current_location);

    return block;
}

bool TranslateSingleThumbInstruction(IR::Block& block, LocationDescriptor descriptor, u32 thumb_instruction) {
    ThumbTranslatorVisitor visitor{block, descriptor, {}};

    const bool is_thumb_16 = IsThumb16(static_cast<u16>(thumb_instruction));
    bool should_continue = true;
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

IR::ResultAndCarry<IR::U32> ThumbTranslatorVisitor::EmitImmShift(IR::U32 value, ShiftType type, Imm<2> imm2, IR::U1 carry_in) {
    u8 imm2_value = imm2.ZeroExtend<u8>();
    switch (type) {
    case ShiftType::LSL:
        return ir.LogicalShiftLeft(value, ir.Imm8(imm2_value), carry_in);
    case ShiftType::LSR:
        imm2_value = imm2_value ? imm2_value : 32;
        return ir.LogicalShiftRight(value, ir.Imm8(imm2_value), carry_in);
    case ShiftType::ASR:
        imm2_value = imm2_value ? imm2_value : 32;
        return ir.ArithmeticShiftRight(value, ir.Imm8(imm2_value), carry_in);
    case ShiftType::ROR:
        if (imm2_value) {
            return ir.RotateRight(value, ir.Imm8(imm2_value), carry_in);
        } else {
            return ir.RotateRightExtended(value, carry_in);
        }
    }
    UNREACHABLE();
}

IR::ResultAndCarry<IR::U32> ThumbTranslatorVisitor::ThumbExpandImm_C(Imm<12> imm12, IR::U1 carry_in) {
    if (imm12.Bits<10, 11>() == 0b00) {
        u8 imm2_value = imm12.Bits<8, 9>();
        IR::U32 imm32;
        u8 imm8 = imm12.Bits<0, 7>();
        switch (imm2_value) {
            case 0b00:
                imm32 = ir.Imm32(imm8);
                break;
            case 0b11:
            case 0b01:
            case 0b10:
            default:
                UNREACHABLE();
        }
        return { imm32, carry_in };
    } else {
        u32 value = 0x80U | imm12.Bits<0, 6>();
        u32 imm5_value = imm12.Bits<7, 11>();
        if (imm5_value) {
            return ir.RotateRight(ir.Imm32(value), ir.Imm8(imm5_value), carry_in);
        } else {
            return ir.RotateRightExtended(ir.Imm32(value), carry_in);
        }
    }
}

} // namespace Dynarmic::A32
