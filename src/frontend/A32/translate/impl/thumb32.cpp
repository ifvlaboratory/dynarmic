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

// AND{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_AND_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.And(ir.GetRegister(n), shifted_m.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

// BIC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_BIC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.And(ir.GetRegister(n), ir.Not(shifted_m.result));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

static void MoveShiftRegisterHelper(ThumbTranslatorVisitor& visitor, bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    auto& ir = visitor.ir;
    const auto cpsr_c = ir.GetCFlag();
    const auto result = visitor.DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
}

// LSL{S}<c>.W <Rd>,<Rm>,#<imm5>
bool ThumbTranslatorVisitor::thumb32_LSL_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    MoveShiftRegisterHelper(*this, S, imm3, d, imm2, Imm<2>(0b00), m);
    return true;
}

// LSR{S}<c>.W <Rd>,<Rm>,#<imm5>
bool ThumbTranslatorVisitor::thumb32_LSR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    MoveShiftRegisterHelper(*this, S, imm3, d, imm2, Imm<2>(0b01), m);
    return true;
}

// ASR{S}<c>.W <Rd>,<Rm>,#<imm5>
bool ThumbTranslatorVisitor::thumb32_ASR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    MoveShiftRegisterHelper(*this, S, imm3, d, imm2, Imm<2>(0b10), m);
    return true;
}

// RRX{S}<c> <Rd>,<Rm>
bool ThumbTranslatorVisitor::thumb32_RRX(bool S, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    MoveShiftRegisterHelper(*this, S, Imm<3>(0b000), d, Imm<2>(0b00), Imm<2>(0b11), m);
    return true;
}

// ROR{S}<c> <Rd>,<Rm>,#<imm5>
bool ThumbTranslatorVisitor::thumb32_ROR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    MoveShiftRegisterHelper(*this, S, imm3, d, imm2, Imm<2>(0b11), m);
    return true;
}

// ORR{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_ORR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.Or(ir.GetRegister(n), shifted_m.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

// MVN{S}<c>.W <Rd>,<Rm>{,shift>}
bool ThumbTranslatorVisitor::thumb32_MVN_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.Not(shifted_m.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

// ORN{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_ORN_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.Or(ir.GetRegister(n), ir.Not(shifted_m.result));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

// TEQ<c> <Rn>, <Rm> {,<shift>}
bool ThumbTranslatorVisitor::thumb32_TEQ_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m) {
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.Eor(ir.GetRegister(n), shifted_m.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(carry_out);
    return true;
}

// EOR{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_EOR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto carry_out = shifted_m.carry;
    const auto result = ir.Eor(ir.GetRegister(n), shifted_m.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(carry_out);
    }
    return true;
}

// PKHBT<c> <Rd>,<Rn>,<Rm>{,LSL #<imm>}
// PKHTB<c> <Rd>,<Rn>,<Rm>{,ASR #<imm>}
bool ThumbTranslatorVisitor::thumb32_PKH(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, bool tb, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, concatenate(Imm<1>(tb), Imm<1>(0)), cpsr_c);

    Helper::PKHHelper(ir, tb, d, ir.GetRegister(n), shifted_m.result);
    return true;
}

// CMN<c>.W <Rn>, <Rm> {,<shift>}
bool ThumbTranslatorVisitor::thumb32_CMN_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m) {
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(0));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// ADD{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_ADD_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(0));

    // TODO move this to helper function
    if (d == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.ALUWritePC(result.result);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    };
    return true;
}

// ADC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_ADC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted_m.result, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    };
    return true;
}

// SBC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_SBC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted_m.result, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    };
    return true;
}

// CMP<c>.W <Rn>, <Rm> {,<shift>}
bool ThumbTranslatorVisitor::thumb32_CMP_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m) {
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(1));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// SUB{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_SUB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(1));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    };
    return true;
}

// RSB{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_RSB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.SubWithCarry(shifted_m.result, ir.GetRegister(n), ir.Imm1(1));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    };
    return true;
}

// ADR<c>.W <Rd>,<label> <label> after current instruction
bool ThumbTranslatorVisitor::thumb32_ADR_after(Imm<1> i, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = concatenate(i, imm3, imm8).ZeroExtend();
    const auto result = ir.Imm32(ir.AlignPC(4) + imm32);

    ir.SetRegister(d, result);
    return true;
}

// ADDW<c> <Rd>,<Rn>,#<imm12>
bool ThumbTranslatorVisitor::thumb32_ADD_imm_2(Imm<1> i, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = concatenate(i, imm3, imm8).ZeroExtend();
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));

    ir.SetRegister(d, result.result);
    return true;
}

// MOVW<c> <Rd>,#<imm16>
bool ThumbTranslatorVisitor::thumb32_MOVW_imm(Imm<1> i, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = concatenate(imm4, i, imm3, imm8).ZeroExtend();
    ir.SetRegister(d, ir.Imm32(imm32));
    return true;
}

// ADR<c>.W <Rd>,<label> <label> before current instruction
bool ThumbTranslatorVisitor::thumb32_ADR_before(Imm<1> i, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = concatenate(i, imm3, imm8).ZeroExtend();
    const auto result = ir.Imm32(ir.AlignPC(4) - imm32);

    ir.SetRegister(d, result);
    return true;
}

// SUBW<c> <Rd>,<Rn>,#<imm12>
bool ThumbTranslatorVisitor::thumb32_SUB_imm_2(Imm<1> i, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = concatenate(i, imm3, imm8).ZeroExtend();
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));

    ir.SetRegister(d, result.result);
    return true;
}

// MOVT<c> <Rd>,#<imm16>
bool ThumbTranslatorVisitor::thumb32_MOVT(Imm<1> i, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 imm16 = ir.Imm32(concatenate(imm4, i, imm3, imm8).ZeroExtend() << 16);
    const IR::U32 operand = ir.GetRegister(d);
    const IR::U32 result = ir.Or(ir.And(operand, ir.Imm32(0x0000FFFFU)), imm16);

    ir.SetRegister(d, result);
    return true;
}

// SSAT<c> <Rd>,#<imm>,<Rn>{,<shift>}
bool ThumbTranslatorVisitor::thumb32_SSAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto t = concatenate(Imm<1>(sh), Imm<1>(0));
    const auto shifted_n = DecodeShiftedReg(n, imm3, imm2, t, cpsr_c);
    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend()) + 1; 
    const auto result = ir.SignedSaturation(shifted_n.result, saturate_to);

    ir.SetRegister(d, result.result);
    ir.OrQFlag(result.overflow);
    return true;
}

// SSAT16<c> <Rd>,#<imm>,<Rn>
bool ThumbTranslatorVisitor::thumb32_SSAT16(Reg n, Reg d, Imm<4> sat_imm) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto saturate_to = static_cast<size_t>(sat_imm.ZeroExtend()) + 1;
    Helper::SSAT16Helper(ir, d, n, saturate_to);
    return true;
}

// SBFX<c> <Rd>,<Rn>,#<lsb>,#<width>
bool ThumbTranslatorVisitor::thumb32_SBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 width_num = widthm.ZeroExtend();

    if (lsbit + width_num >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    Helper::SBFXHelper(ir, d, n, lsbit, width_num);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
