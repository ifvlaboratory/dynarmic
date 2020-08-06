/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

// BL <label>
bool ThumbTranslatorVisitor::thumb32_BL_imm(Imm<11> hi, Imm<11> lo) {
    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>((concatenate(hi, lo).SignExtend<u32>() << 1) + 4);
    const auto new_location = ir.current_location.AdvancePC(imm32);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool ThumbTranslatorVisitor::thumb32_BLX_imm(Imm<11> hi, Imm<11> lo) {
    if (lo.Bit<0>()) {
        return UnpredictableInstruction();
    }

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>(concatenate(hi, lo).SignExtend<u32>() << 1);
    const auto new_location = ir.current_location
                                .SetPC(ir.AlignPC(4) + imm32)
                                .SetTFlag(false);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// MOV<c> <Rd>, #<const>
bool ThumbTranslatorVisitor::thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return false;
    }
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Imm32(imm_carry.imm32);
    
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

// TST<c> <Rn>, #<const>
bool ThumbTranslatorVisitor::thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));
    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

// AND{S}<c> <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
