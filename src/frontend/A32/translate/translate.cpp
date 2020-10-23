/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/ir/basic_block.h"

namespace Dynarmic::A32 {

IR::Block TranslateArm(LocationDescriptor descriptor, const MemoryReadCodeFuncType& memory_read_code, const TranslationOptions& options);
IR::Block TranslateThumb(LocationDescriptor descriptor, const MemoryReadCodeFuncType& memory_read_code, const TranslationOptions& options);

IR::Block Translate(LocationDescriptor descriptor, const MemoryReadCodeFuncType& memory_read_code, const TranslationOptions& options) {
    return (descriptor.TFlag() ? TranslateThumb : TranslateArm)(descriptor, memory_read_code, options);
}

bool TranslateSingleArmInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction);
bool TranslateSingleThumbInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction);

bool TranslateSingleInstruction(IR::Block& block, LocationDescriptor descriptor, u32 instruction) {
    return (descriptor.TFlag() ? TranslateSingleThumbInstruction : TranslateSingleArmInstruction)(block, descriptor, instruction);
}

} // namespace Dynarmic::A32
