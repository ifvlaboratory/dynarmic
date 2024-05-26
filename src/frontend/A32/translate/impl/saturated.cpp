/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

// Saturation instructions

// SSAT<c> <Rd>, #<imm>, <Rn>{, <shift>}
bool ArmTranslatorVisitor::arm_SSAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend()) + 1;
    const auto shift = !sh ? ShiftType::LSL : ShiftType::ASR;
    const auto operand = EmitImmShift(ir.GetRegister(n), shift, imm5, ir.GetCFlag());
    const auto result = ir.SignedSaturation(operand.result, saturate_to);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// SSAT16<c> <Rd>, #<imm>, <Rn>
bool ArmTranslatorVisitor::arm_SSAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend()) + 1;
    Helper::SSAT16Helper(ir, d, n, saturate_to);
    return true;
}

// USAT<c> <Rd>, #<imm5>, <Rn>{, <shift>}
bool ArmTranslatorVisitor::arm_USAT(Cond cond, Imm<5> sat_imm, Reg d, Imm<5> imm5, bool sh, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend());
    const auto shift = !sh ? ShiftType::LSL : ShiftType::ASR;
    const auto operand = EmitImmShift(ir.GetRegister(n), shift, imm5, ir.GetCFlag());
    const auto result = ir.UnsignedSaturation(operand.result, saturate_to);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// USAT16<c> <Rd>, #<imm4>, <Rn>
bool ArmTranslatorVisitor::arm_USAT16(Cond cond, Imm<4> sat_imm, Reg d, Reg n) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    // UnsignedSaturation takes a *signed* value as input, hence sign extension is required.
    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend());
    const auto lo_operand = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(ir.GetRegister(n)));
    const auto hi_operand = ir.SignExtendHalfToWord(Helper::MostSignificantHalf(ir, ir.GetRegister(n)));
    const auto lo_result = ir.UnsignedSaturation(lo_operand, saturate_to);
    const auto hi_result = ir.UnsignedSaturation(hi_operand, saturate_to);

    ir.SetRegister(d, Helper::Pack2x16To1x32(ir, lo_result.result, hi_result.result));
    ir.OrQFlag(lo_result.overflow);
    ir.OrQFlag(hi_result.overflow);
    return true;
}

// Saturated Add/Subtract instructions

// QADD<c> <Rd>, <Rm>, <Rn>
bool ArmTranslatorVisitor::arm_QADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto a = ir.GetRegister(m);
    const auto b = ir.GetRegister(n);
    const auto result = ir.SignedSaturatedAdd(a, b);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// QSUB<c> <Rd>, <Rm>, <Rn>
bool ArmTranslatorVisitor::arm_QSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto a = ir.GetRegister(m);
    const auto b = ir.GetRegister(n);
    const auto result = ir.SignedSaturatedSub(a, b);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// QDADD<c> <Rd>, <Rm>, <Rn>
bool ArmTranslatorVisitor::arm_QDADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto a = ir.GetRegister(m);
    const auto b = ir.GetRegister(n);
    const auto doubled = ir.SignedSaturatedAdd(b, b);
    ir.OrQFlag(doubled.overflow);

    const auto result = ir.SignedSaturatedAdd(a, doubled.result);
    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// QDSUB<c> <Rd>, <Rm>, <Rn>
bool ArmTranslatorVisitor::arm_QDSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto a = ir.GetRegister(m);
    const auto b = ir.GetRegister(n);
    const auto doubled = ir.SignedSaturatedAdd(b, b);
    ir.OrQFlag(doubled.overflow);

    const auto result = ir.SignedSaturatedSub(a, doubled.result);
    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// Parallel saturated instructions

// QASX<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_QASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.SignExtendHalfToWord(Helper::MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.SignExtendHalfToWord(Helper::MostSignificantHalf(ir, Rm));
    const auto diff = ir.SignedSaturation(ir.Sub(Rn_lo, Rm_hi), 16).result;
    const auto sum = ir.SignedSaturation(ir.Add(Rn_hi, Rm_lo), 16).result;
    const auto result = Helper::Pack2x16To1x32(ir, diff, sum);

    ir.SetRegister(d, result);
    return true;
}

// QSAX<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_QSAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.SignExtendHalfToWord(Helper::MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.SignExtendHalfToWord(Helper::MostSignificantHalf(ir, Rm));
    const auto sum = ir.SignedSaturation(ir.Add(Rn_lo, Rm_hi), 16).result;
    const auto diff = ir.SignedSaturation(ir.Sub(Rn_hi, Rm_lo), 16).result;
    const auto result = Helper::Pack2x16To1x32(ir, sum, diff);

    ir.SetRegister(d, result);
    return true;
}

// UQASX<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_UQASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.ZeroExtendHalfToWord(Helper::MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.ZeroExtendHalfToWord(Helper::MostSignificantHalf(ir, Rm));
    const auto diff = ir.UnsignedSaturation(ir.Sub(Rn_lo, Rm_hi), 16).result;
    const auto sum = ir.UnsignedSaturation(ir.Add(Rn_hi, Rm_lo), 16).result;
    const auto result = Helper::Pack2x16To1x32(ir, diff, sum);

    ir.SetRegister(d, result);
    return true;
}

// UQSAX<c> <Rd>, <Rn>, <Rm>
bool ArmTranslatorVisitor::arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.ZeroExtendHalfToWord(Helper::MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.ZeroExtendHalfToWord(Helper::MostSignificantHalf(ir, Rm));
    const auto sum = ir.UnsignedSaturation(ir.Add(Rn_lo, Rm_hi), 16).result;
    const auto diff = ir.UnsignedSaturation(ir.Sub(Rn_hi, Rm_lo), 16).result;
    const auto result = Helper::Pack2x16To1x32(ir, sum, diff);

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32
