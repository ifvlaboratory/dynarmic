/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"
#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_DMB([[maybe_unused]] Imm<4> option) {
    ir.DataMemoryBarrier();
    return true;
}

bool ArmTranslatorVisitor::arm_DSB([[maybe_unused]] Imm<4> option) {
    ir.DataSynchronizationBarrier();
    return true;
}

bool ArmTranslatorVisitor::arm_ISB([[maybe_unused]] Imm<4> option) {
    ir.InstructionSynchronizationBarrier();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

// DMB<c> <option>
bool ThumbTranslatorVisitor::thumb32_DMB([[maybe_unused]] Imm<4> option) {
    ir.DataMemoryBarrier();
    return true;
}

} // namespace Dynarmic::A32
