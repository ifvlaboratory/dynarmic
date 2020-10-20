/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */
#pragma once

#include "common/common_types.h"

namespace Dynarmic::IR {
class Block;
} // namespace Dynarmic::IR

namespace Dynarmic::A32 {

class LocationDescriptor;

using MemoryReadCodeFuncType = std::function<u32(u32 vaddr, bool thumb)>;

struct TranslationOptions {
    /// This changes what IR we emit when we translate an unpredictable instruction.
    /// If this is false, the ExceptionRaised IR instruction is emitted.
    /// If this is true, we define some behaviour for some instructions.
    bool define_unpredictable_behaviour = false;

    /// This changes what IR we emit when we translate a hint instruction.
    /// If this is false, we treat the instruction as a NOP.
    /// If this is true, we emit an ExceptionRaised instruction.
    bool hook_hint_instructions = true;
};

/**
 * This function translates instructions in memory into our intermediate representation.
 * @param descriptor The starting location of the basic block. Includes information like PC, Thumb state, &c.
 * @param memory_read_code The function we should use to read emulated memory.
 * @param options Configures how certain instructions are translated.
 * @return A translated basic block in the intermediate representation.
 */
IR::Block Translate(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, const TranslationOptions& options);

/**
 * This function translates a single provided instruction into our intermediate representation.
 * @param block The block to append the IR for the instruction to.
 * @param descriptor The location of the instruction. Includes information like PC, Thumb state, &c.
 * @param instruction The instruction to translate.
 * @return The translated instruction translated to the intermediate representation.
 */
bool TranslateSingleInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction);

} // namespace Dynarmic::A32
