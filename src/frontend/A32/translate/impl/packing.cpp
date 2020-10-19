/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

// PKHBT<c> <Rd>, <Rn>, <Rm>{, LSL #<imm>}
bool ArmTranslatorVisitor::arm_PKHBT(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }


    const auto shifted = EmitImmShift(ir.GetRegister(m), ShiftType::LSL, imm5, ir.Imm1(false)).result;
    Helper::PKHHelper(ir, false, d, ir.GetRegister(n), shifted);
    return true;
}

// PKHTB<c> <Rd>, <Rn>, <Rm>{, ASR #<imm>}
bool ArmTranslatorVisitor::arm_PKHTB(Cond cond, Reg n, Reg d, Imm<5> imm5, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), ShiftType::ASR, imm5, ir.Imm1(false)).result;
    Helper::PKHHelper(ir, true, d, ir.GetRegister(n), shifted);
    return true;
}

} // namespace Dynarmic::A32
