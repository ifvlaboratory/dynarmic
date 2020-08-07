/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

// BL <label>
bool ThumbTranslatorVisitor::thumb32_BL_imm(Imm<11> hi, Imm<11> lo) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>((concatenate(hi, lo).SignExtend<u32>() << 1) + 4);
    const auto new_location = ir.current_location.AdvancePC(imm32);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool ThumbTranslatorVisitor::thumb32_BLX_imm(Imm<11> hi, Imm<11> lo) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
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

// BIC{S}<c> <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.imm32)));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
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

// ORR{S}<c> <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

// MVN{S}<c> <Rd>,#<const>
bool ThumbTranslatorVisitor::thumb32_MVN_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Not(ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

// ORN{S}<c> <Rd>,<Rd>,#<const>
bool ThumbTranslatorVisitor::thumb32_ORN_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
       return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Or(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.imm32)));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

// TEQ<c> <Rn>, #<const>
bool ThumbTranslatorVisitor::thumb32_TEQ_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));
    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

// EOR{S}<c> <Rd>,<Rd>,#<const>
bool ThumbTranslatorVisitor::thumb32_EOR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
       return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

// CMN<c> <Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_CMN_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
       return UnpredictableInstruction();
    }
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));
    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// TODO: Add SP imm
// ADD{S}<c>.W <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_ADD_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
     }
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// ADC{S}<c> <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_ADC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
     }
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), cpsr_c);
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// SBC{S}<c> <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_SBC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
    }
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto cpsr_c = ir.GetCFlag();
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), cpsr_c);
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// CMP <Rn>,#<imm8>
bool ThumbTranslatorVisitor::thumb32_CMP_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
    }
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// TODO: Sub SP imm
// ADD{S}<c>.W <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_SUB_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
     }
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    
    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// RSB{S}<c>.W <Rd>,<Rn>,#<const>
bool ThumbTranslatorVisitor::thumb32_RSB_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
         return true;
     }
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.Imm32(imm32), ir.GetRegister(n), ir.Imm1(1));
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// STMIA<c>.W <Rn>{!},<registers>
bool ThumbTranslatorVisitor::thumb32_STMIA(bool W, Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || Common::BitCount(reg_list) < 2) {
    return UnpredictableInstruction();
    }
    // PC and SP cannot be in list
    if (Common::Bit<15>(reg_list) || Common::Bit<13>(reg_list)) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto address = ir.GetRegister(n);
    const auto final_address = ir.Add(ir.GetRegister(n), ir.Imm32(num_bytes));
    return Helper::STMHelper(ir, W, n, reg_list, address, final_address);
}

// LDMIA<c>.W <Rn>{!},<registers>
bool ThumbTranslatorVisitor::thumb32_LDMIA(bool W, Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (W && Common::Bit(static_cast<size_t>(n), reg_list)) {
        return UnpredictableInstruction();
    }
    // If PC is in list, LR cannot be in list
    if (Common::Bit<15>(reg_list) && Common::Bit<14>(reg_list)) {
        return UnpredictableInstruction();
    }
    if (n == Reg::PC || Common::BitCount(reg_list) < 2) {
        return UnpredictableInstruction();
    }
    const auto it = ir.current_location.IT();
    if (Common::Bit<15>(reg_list) && it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto address = ir.GetRegister(n);
    const auto final_address = ir.Add(ir.GetRegister(n), ir.Imm32(num_bytes));
    return Helper::LDMHelper(ir, W, n, reg_list, address, final_address);
}

// STMDB<c> <Rn>{!},<registers>
bool ThumbTranslatorVisitor::thumb32_STMDB(bool W, Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || Common::BitCount(reg_list) < 2) {
        return UnpredictableInstruction();
    }
    // PC and SP cannot be in list
    if (Common::Bit<15>(reg_list) || Common::Bit<13>(reg_list)) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto final_address = ir.Sub(ir.GetRegister(n), ir.Imm32(num_bytes));
    return Helper::STMHelper(ir, W, n, reg_list, final_address, final_address);
}

// LDMDB<c> <Rn>{!},<registers>
bool ThumbTranslatorVisitor::thumb32_LDMDB(bool W, Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (W && Common::Bit(static_cast<size_t>(n), reg_list)) {
        return UnpredictableInstruction();
    }
    // If PC is in list, LR cannot be in list
    if (Common::Bit<15>(reg_list) && Common::Bit<14>(reg_list)) {
        return UnpredictableInstruction();
    }
    if (n == Reg::PC || Common::BitCount(reg_list) < 2) {
        return UnpredictableInstruction();
    }
    const auto it = ir.current_location.IT();
    if (Common::Bit<15>(reg_list) && it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto final_address = ir.Sub(ir.GetRegister(n), ir.Imm32(num_bytes));
    auto address = final_address;
    return Helper::LDMHelper(ir, W, n, reg_list, final_address, final_address);
}

// TST<c>.W <Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_TST_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m) {
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.And(ir.GetRegister(n), shifted_m.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(carry_out);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
