/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"
#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {
namespace {
using DivideFunction = IR::U32U64 (IREmitter::*)(const IR::U32U64&, const IR::U32U64&);

bool DivideOperation(ArmTranslatorVisitor& v, Cond cond, Reg d, Reg m, Reg n, DivideFunction fn) {
    if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    if (!v.ConditionPassed(cond)) {
        return true;
    }

    const IR::U32 operand1 = v.ir.GetRegister(n);
    const IR::U32 operand2 = v.ir.GetRegister(m);
    const IR::U32 result = (v.ir.*fn)(operand1, operand2);

    v.ir.SetRegister(d, result);
    return true;
}
}  // Anonymous namespace

// SDIV<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_SDIV(Cond cond, Reg d, Reg m, Reg n) {
    return DivideOperation(*this, cond, d, m, n, &IREmitter::SignedDiv);
}

// UDIV<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_UDIV(Cond cond, Reg d, Reg m, Reg n) {
    return DivideOperation(*this, cond, d, m, n, &IREmitter::UnsignedDiv);
}

// UDIV<c> <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_UDIV(Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13 || n == Reg::R13) {
        return UnpredictableInstruction();
    }

    DivideFunction fn = &IREmitter::UnsignedDiv;
    const IR::U32 operand1 = ir.GetRegister(n);
    const IR::U32 operand2 = ir.GetRegister(m);
    const IR::U32 result = (ir.*fn)(operand1, operand2);

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32
