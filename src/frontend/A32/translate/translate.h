/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */
#pragma once

#include <dynarmic/A32/config.h>

#include "common/common_types.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/types.h"

#include <dynarmic/A32/arch_version.h>

namespace Dynarmic::IR {
class Block;
} // namespace Dynarmic::IR

namespace Dynarmic::A32 {

class LocationDescriptor;

using MemoryReadCodeFuncType = std::function<u32(u32 vaddr, bool thumb)>;

struct TranslationOptions {
    ArchVersion arch_version;

    /// This changes what IR we emit when we translate an unpredictable instruction.
    /// If this is false, the ExceptionRaised IR instruction is emitted.
    /// If this is true, we define some behaviour for some instructions.
    bool define_unpredictable_behaviour = false;

    /// This changes what IR we emit when we translate a hint instruction.
    /// If this is false, we treat the instruction as a NOP.
    /// If this is true, we emit an ExceptionRaised instruction.
    bool hook_hint_instructions = true;
};

enum class ConditionalState {
    /// We haven't met any conditional instructions yet.
    None,
    /// Current instruction is a conditional. This marks the end of this basic block.
    Break,
    /// This basic block is made up solely of conditional instructions.
    Translating,
    /// This basic block is made up of conditional instructions followed by unconditional instructions.
    Trailing,
};

struct A32TranslatorVisitor {

    explicit A32TranslatorVisitor(IR::Block& block, LocationDescriptor descriptor, const TranslationOptions& options) : ir(block, descriptor), options(options) {
    }

    ConditionalState cond_state = ConditionalState::None;

    A32::IREmitter ir;
    TranslationOptions options;

    template <typename FnT> bool EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg n, ExtReg m, const FnT& fn);
    template <typename FnT> bool EmitVfpVectorOperation(bool sz, ExtReg d, ExtReg m, const FnT& fn);

    // Creates an immediate of the given value
    IR::UAny I(size_t bitsize, u64 value) {
        switch (bitsize) {
            case 8:
                return ir.Imm8(static_cast<u8>(value));
            case 16:
                return ir.Imm16(static_cast<u16>(value));
            case 32:
                return ir.Imm32(static_cast<u32>(value));
            case 64:
                return ir.Imm64(value);
            default:
                ASSERT_FALSE("Imm - get: Invalid bitsize");
        }
    }

    bool InterpretThisInstruction() {
        ir.SetTerm(IR::Term::Interpret(ir.current_location));
        return false;
    }

    bool DecodeError() {
        ir.ExceptionRaised(Exception::DecodeError);
        ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
        return false;
    }

    bool UndefinedInstruction() {
        ir.ExceptionRaised(Exception::UndefinedInstruction);
        ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
        return false;
    }

    bool UnpredictableInstruction() {
        ir.ExceptionRaised(Exception::UnpredictableInstruction);
        ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
        return false;
    }
};

/**
 * This function translates instructions in memory into our intermediate representation.
 * @param descriptor The starting location of the basic block. Includes information like PC, Thumb state, &c.
 * @param memory_read_code The function we should use to read emulated memory.
 * @param options Configures how certain instructions are translated.
 * @return A translated basic block in the intermediate representation.
 */
IR::Block Translate(LocationDescriptor descriptor, const MemoryReadCodeFuncType& memory_read_code, const TranslationOptions& options);

/**
 * This function translates a single provided instruction into our intermediate representation.
 * @param block The block to append the IR for the instruction to.
 * @param descriptor The location of the instruction. Includes information like PC, Thumb state, &c.
 * @param instruction The instruction to translate.
 * @return The translated instruction translated to the intermediate representation.
 */
bool TranslateSingleInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction);

} // namespace Dynarmic::A32
