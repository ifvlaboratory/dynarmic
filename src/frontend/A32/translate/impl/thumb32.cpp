/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {
namespace {
    using DivideFunction = IR::U32U64 (IREmitter::*)(const IR::U32U64&, const IR::U32U64&);

    bool DivideOperation(ThumbTranslatorVisitor& v, Reg d, Reg m, Reg n,
                         DivideFunction fn) {
        if (!v.ConditionPassed()) {
            return true;
        }
        if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
            return v.UnpredictableInstruction();
        }
        if (d == Reg::R13 || m == Reg::R13 || n == Reg::R13) {
            return v.UnpredictableInstruction();
        }

        const IR::U32 operand1 = v.ir.GetRegister(n);
        const IR::U32 operand2 = v.ir.GetRegister(m);
        const IR::U32 result = (v.ir.*fn)(operand1, operand2);

        v.ir.SetRegister(d, result);
        return true;
    }
} // Anonymous namespace

static IR::U32 Rotate(A32::IREmitter& ir, Reg m, SignExtendRotation rotate) {
    const u8 rotate_by = static_cast<u8>(static_cast<size_t>(rotate) * 8);
    return ir.RotateRight(ir.GetRegister(m), ir.Imm8(rotate_by), ir.Imm1(false)).result;
}

// BL <label>
bool ThumbTranslatorVisitor::thumb32_BL_imm(bool S, Imm<10> hi, bool j1, bool j2, Imm<11> lo) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    bool i1_value = !(j1 ^ S);
    bool i2_value = !(j2 ^ S);
    const Imm<1> i1 = Imm<1>(i1_value);
    const Imm<1> i2 = Imm<1>(i2_value);
    s32 imm32 = concatenate(Imm<1>(S), i1, i2, hi, lo, Imm<1>(0)).SignExtend();

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32(ir.PC() | 1U));
    const auto new_location = ir.current_location.SetPC(ir.PC() + imm32);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool ThumbTranslatorVisitor::thumb32_BLX_imm(bool S, Imm<10> hi, bool j1, bool j2, Imm<11> lo) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
    if (lo.Bit<0>()) {
        return UnpredictableInstruction();
    }

    bool i1_value = !(j1 ^ S);
    bool i2_value = !(j2 ^ S);
    const Imm<1> i1 = Imm<1>(i1_value);
    const Imm<1> i2 = Imm<1>(i2_value);
    s32 imm32 = concatenate(Imm<1>(S), i1, i2, hi, lo, Imm<1>(0)).SignExtend();

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32(ir.PC() | 1U));
    const auto new_location = ir.current_location
                                .SetPC(ir.AlignPC(4) + imm32)
                                .SetTFlag(false);
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// PUSH<c>.W <registers>
// reg_list cannot encode for R15.
bool ThumbTranslatorVisitor::thumb32_PUSH(bool M, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (M) {
        reg_list |= (1U << 14U);
    }
    if (Common::BitCount(reg_list) < 2) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto final_address = ir.Sub(ir.GetRegister(Reg::SP), ir.Imm32(num_bytes));
    auto address = final_address;
    return Helper::STMHelper(ir, true, Reg::SP, reg_list, address, final_address);
}

// B<c>.W <label>
bool ThumbTranslatorVisitor::thumb32_B_cond(Imm<1> S, Cond cond, Imm<6> imm6, Imm<1> j1, Imm<1> j2, Imm<11> imm11) {
    if (Common::Bits<1, 3>(static_cast<u8>(cond)) == 0b111) {
        return UnpredictableInstruction();
    }

    const auto it = ir.current_location.IT();
    if (it.IsInITBlock()) {
        return UnpredictableInstruction();
    }

    s32 imm32 = concatenate(S, j2, j1, imm6, imm11, Imm<1>(0)).SignExtend();
    const auto then_location = ir.current_location.SetPC(ir.PC() + imm32);
    const auto else_location = ir.current_location.AdvancePC(4);

    ir.SetTerm(IR::Term::If{ cond, IR::Term::LinkBlock{ then_location }, IR::Term::LinkBlock{ else_location } });
    return false;
}

// B<c>.W <label>
bool ThumbTranslatorVisitor::thumb32_B(bool S, Imm<10> imm10, bool j1, bool j2, Imm<11> imm11) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    const bool i1_value = !(j1 ^ S);
    const bool i2_value = !(j2 ^ S);
    const Imm<1> i1 = Imm<1>(i1_value);
    const Imm<1> i2 = Imm<1>(i2_value);
    s32 imm32 = concatenate(Imm<1>(S), i1, i2, imm10, imm11, Imm<1>(0)).SignExtend();

    const auto new_location = ir.current_location.SetPC(ir.PC() + imm32);
    ir.SetTerm(IR::Term::LinkBlock{ new_location });
    return false;
}

// MOV<c> <Rd>, #<const>
bool ThumbTranslatorVisitor::thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return false;
    }
    const auto cpsr_c = ir.GetCFlag();
    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, cpsr_c);
    const auto result = ir.Imm32(imm_carry.result);
    
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
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.result));

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
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.result)));

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
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.result));

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
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(imm_carry.result));

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
    const auto result = ir.Not(ir.Imm32(imm_carry.result));

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
    const auto result = ir.Or(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.result)));

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
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.result));

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
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.result));

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
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(false));

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
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(false));

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
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

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
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

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
    const auto result = ir.SubWithCarry(ir.Imm32(imm32), ir.GetRegister(n), ir.Imm1(true));

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
    auto address = final_address;
    return Helper::STMHelper(ir, W, n, reg_list, address, final_address);
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
    return Helper::LDMHelper(ir, W, n, reg_list, address, final_address);
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
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
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
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(false));

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
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(false));

    // TODO move this to helper function
    if (d == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.BXWritePC(result.result);
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
    }
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
    }
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
    }
    return true;
}

// CMP<c>.W <Rn>, <Rm> {,<shift>}
bool ThumbTranslatorVisitor::thumb32_CMP_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m) {
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto cpsr_c = ir.GetCFlag();
    const auto shifted_m = DecodeShiftedReg(m, imm3, imm2, t, cpsr_c);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(true));

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
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted_m.result, ir.Imm1(true));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
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
    const auto result = ir.SubWithCarry(shifted_m.result, ir.GetRegister(n), ir.Imm1(true));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
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
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(false));

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
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

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

    const IR::U32 imm16 = ir.Imm32(concatenate(imm4, i, imm3, imm8).ZeroExtend() << 16U);
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
bool ThumbTranslatorVisitor::thumb32_SSAT16(Reg n, Reg d, Imm<5> sat_imm) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }
    // Note that to undefined this case, SAT16 mathced one more bit in sat_imm decoder field.
    if (sat_imm.Bit<4>() != 0) {
        return UndefinedInstruction();
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

// BFC<c> <Rd>,#<lsb>,#<width>
bool ThumbTranslatorVisitor::thumb32_BFC(Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 lsbit = concatenate(imm3, imm2).ZeroExtend();
    const u32 msbit = msb.ZeroExtend();

    if (msbit < lsbit) {
        return UnpredictableInstruction();
    }

    Helper::BFCHelper(ir, d, lsbit, msbit);
    return true;
}

// BFI<c> <Rd>, <Rn>, #<lsb>, #<width>
bool ThumbTranslatorVisitor::thumb32_BFI(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC || d == Reg::R13 || n == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto lsb = concatenate(imm3, imm2);
    if (msb < lsb) {
        return UnpredictableInstruction();
    }

    const u32 lsb_value = lsb.ZeroExtend();
    const u32 msb_value = msb.ZeroExtend();
    const u32 inclusion_mask = Common::Ones<u32>(msb_value - lsb_value + 1) << lsb_value;
    const u32 exclusion_mask = ~inclusion_mask;
    const IR::U32 operand1 = ir.And(ir.GetRegister(d), ir.Imm32(exclusion_mask));
    const IR::U32 operand2 = ir.And(ir.LogicalShiftLeft(ir.GetRegister(n), ir.Imm8(u8(lsb_value))), ir.Imm32(inclusion_mask));
    const IR::U32 result = ir.Or(operand1, operand2);

    ir.SetRegister(d, result);
    return true;
}

// STRB<c> <Rt>, [<Rn>, # - <imm8>]
// STRB<c> <Rt>, [<Rn>], # + / -<imm8>
// STRB<c> <Rt>, [<Rn>, # + / -<imm8>]!
bool ThumbTranslatorVisitor::thumb32_STRB_imm_1(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)));
    return true;
}

// STRB<c>.W <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_STRB_imm_2(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, true, true, false, n, ir.Imm32(imm12.ZeroExtend()));
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)));
    return true;
}

// STRB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_STRB(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)));
    return true;
}

// STRH<c> <Rt>,[<Rn>,#-<imm8>]
// STRH<c> <Rt>,[<Rn>],#+/-<imm8>
// STRH<c> <Rt>,[<Rn>,#+/-<imm8>]!
bool ThumbTranslatorVisitor::thumb32_STRH_imm_1(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, P, U, W, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)));
    return true;
}

// STRH<c>.W <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_STRH_imm_3(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)));
    return true;
}

// STR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_STRH(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)));
    return true;
}

// STR<c> <Rt>,[<Rn>,#-<imm8>]
// STR<c> <Rt>,[<Rn>],#+/-<imm8>
// STR<c> <Rt>,[<Rn>,#+/-<imm8>]!
bool ThumbTranslatorVisitor::thumb32_STR_imm_1(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    ir.WriteMemory32(address, ir.GetRegister(t));
    return true;
}

// STR<c>.W <Rt>,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_STR_imm_3(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);

    ir.WriteMemory32(address, ir.GetRegister(t));
    return true;
}

// STR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_STR_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    ir.WriteMemory32(address, ir.GetRegister(t));
    return true;
}

// LDRB<c> <Rt>,[PC,#+/-<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRB_lit(bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u32 base = ir.AlignPC(4);
    const u32 imm12_value = imm12.ZeroExtend();
    const u32 address = U ? base + imm12_value : base - imm12_value;
    const auto read_byte = ir.ZeroExtendByteToWord(ir.ReadMemory8(ir.Imm32(address)));
    ir.SetRegister(t, read_byte);
    return true;
}

// LDRB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_LDRB_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC || t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRB<c> <Rt>,[<Rn>,#-<imm8>]
// LDRB<c> <Rt>, [<Rn>], # + / -<imm8>
// LDRB<c> <Rt>, [<Rn>, # + / -<imm8>]!
bool ThumbTranslatorVisitor::thumb32_LDRB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRB<c>.W <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRB_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSB<c> <Rt>,[PC,#+/-<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRSB_lit(bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u32 base = ir.AlignPC(4);
    const u32 imm12_value = imm12.ZeroExtend();
    const u32 address = U ? base + imm12_value : base - imm12_value;
    const auto read_byte = ir.SignExtendByteToWord(ir.ReadMemory8(ir.Imm32(address)));
    ir.SetRegister(t, read_byte);
    return true;
}

// LDRSB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_LDRSB_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC || t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSB<c> <Rt>,[<Rn>,#-<imm8>]
// LDRSB<c> <Rt>, [<Rn>], # + / -<imm8>
// LDRSB<c> <Rt>, [<Rn>, # + / -<imm8>]!
bool ThumbTranslatorVisitor::thumb32_LDRSB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSB<c> <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRSB_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRH<c> <Rt>,[PC,#+/-<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRH_lit(bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u32 base = ir.AlignPC(4);
    const u32 imm12_value = imm12.ZeroExtend();
    const u32 address = U ? base + imm12_value : base - imm12_value;
    const auto read_byte = ir.ZeroExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address)));
    ir.SetRegister(t, read_byte);
    return true;
}

// LDRH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_LDRH_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC || t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRH<c> <Rt>,[<Rn>,#-<imm8>]
// LDRH<c> <Rt>, [<Rn>], # + / -<imm8>
// LDRH<c> <Rt>, [<Rn>, # + / -<imm8>]!
bool ThumbTranslatorVisitor::thumb32_LDRH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRH<c>.W <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRH_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSH<c> <Rt>,[PC,#+/-<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRSH_lit(bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u32 base = ir.AlignPC(4);
    const u32 imm12_value = imm12.ZeroExtend();
    const u32 address = U ? base + imm12_value : base - imm12_value;
    const auto read_byte = ir.SignExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address)));
    ir.SetRegister(t, read_byte);
    return true;
}

// LDRSH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_LDRSH_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC || t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSH<c> <Rt>,[<Rn>,#-<imm8>]
// LDRSH<c> <Rt>, [<Rn>],#+/-<imm8>
// LDRSH<c> <Rt>, [<Rn>,#+/-<imm8>]!
bool ThumbTranslatorVisitor::thumb32_LDRSH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || (W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDRSH<c> <Rt,[<Rn>,#<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDRSH_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));
    ir.SetRegister(t, data);
    return true;
}

// LDR<c>.W <Rt>,[PC,#+/-<imm12>]
bool ThumbTranslatorVisitor::thumb32_LDR_lit(bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::SP) {
        return UnpredictableInstruction();
    }

    const u32 base = ir.AlignPC(4);
    const u32 imm12_value = imm12.ZeroExtend();
    const u32 address = U ? base + imm12_value : base - imm12_value;
    const auto read_byte = ir.ReadMemory32(ir.Imm32(address));
    if (t == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.BXWritePC(read_byte);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    ir.SetRegister(t, read_byte);
    return true;
}

// LDR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<shift>}]
bool ThumbTranslatorVisitor::thumb32_LDR_reg(Reg n, Reg t, Imm<2> shift, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u8 shift_value = static_cast<u8>(shift.ZeroExtend());
    const auto offset = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_value));
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ReadMemory32(address);
    if (t == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.BXWritePC(data);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    ir.SetRegister(t, data);
    return true;
}

// LDR<c> <Rt>,[<Rn>,#-<imm8>]
// LDR<c> <Rt>,[<Rn>],#+/-<imm8>
// LDR<c> <Rt>,[<Rn>,#+/-<imm8>]!
bool ThumbTranslatorVisitor::thumb32_LDR_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((!P && !W) || n == Reg::PC) {
        return UndefinedInstruction();
    }
    if ((W && n == t)) {
        return UnpredictableInstruction();
    }

    const auto address = Helper::GetAddress(ir, P, U, W, n, ir.Imm32(imm8.ZeroExtend()));
    const auto data = ir.ReadMemory32(address);
    if (t == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.BXWritePC(data);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    ir.SetRegister(t, data);
    return true;
}

// LDR<c>.W <Rt>, [<Rn>{, #<imm12>}]
bool ThumbTranslatorVisitor::thumb32_LDR_imm12(Reg n, Reg t, Imm<12> imm12) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto data = ir.ReadMemory32(address);
    if (t == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.BXWritePC(data);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    ir.SetRegister(t, data);
    return true;
}

// DMB<c> <option>
bool ThumbTranslatorVisitor::thumb32_DMB([[maybe_unused]] Imm<4> option) {
    if (!ConditionPassed()) {
        return true;
    }
    ir.DataMemoryBarrier();
    return true;
}

// MRC<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
bool ThumbTranslatorVisitor::thumb32_MRC(size_t opc1, CoprocReg CRn, Reg t, size_t coproc, size_t opc2, CoprocReg CRm) {
    if (!ConditionPassed()) {
        return true;
    }
    if ((coproc & 0b1110U) == 0b1010) {
        return UndefinedInstruction();
    }
    if(t == Reg::PC) {
        return UndefinedInstruction();
    }

    const auto word = ir.CoprocGetOneWord(coproc, false, opc1, CRn, CRm, opc2);
    ir.SetRegister(t, word);
    return true;
}

// STRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm>}]
// STRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm>
// STRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm>]!
bool ThumbTranslatorVisitor::thumb32_STRD_imm_2(bool P, bool U, bool W, Reg n, Reg t1, Reg t2, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }

    if (!P && !W) {
        return UnpredictableInstruction();
    }
    if(n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(W && (n == t1 || n == t2)) {
        return UnpredictableInstruction();
    }
    if(t1 == Reg::R13 || t1 == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(t2 == Reg::R13 || t2 == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const auto offset = ir.Imm32(imm32);
    const auto address_a = Helper::GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto value_a = ir.GetRegister(t1);
    const auto value_b = ir.GetRegister(t2);

    ir.WriteMemory32(address_a, value_a);
    ir.WriteMemory32(address_b, value_b);
    return true;
}

// LDRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm>}]
// LDRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm>
// LDRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm>]!
bool ThumbTranslatorVisitor::thumb32_LDRD_imm_2(bool P, bool U, bool W, Reg n, Reg t1, Reg t2, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }

    if (!P && !W) {
        return UnpredictableInstruction();
    }
    if(n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(W && (n == t1 || n == t2)) {
        return UnpredictableInstruction();
    }
    if(t1 == Reg::R13 || t1 == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(t2 == Reg::R13 || t2 == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(t1 == t2) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend() << 2U;

    const auto offset = ir.Imm32(imm32);
    const auto address_a = Helper::GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto data_a = ir.ReadMemory32(address_a);
    const auto data_b = ir.ReadMemory32(address_b);

    ir.SetRegister(t1, data_a);
    ir.SetRegister(t2, data_b);
    return true;
}

// STREXH<c> <Rd>, <Rt>, [<Rn>]
bool ThumbTranslatorVisitor::thumb32_STREXH(Reg n, Reg t, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::R13 || d == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (t == Reg::R13 || t == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantHalf(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory16(address, value);
    ir.SetRegister(d, passed);
    return true;
}

// LDREXH<c> <Rt>, [<Rn>]
bool ThumbTranslatorVisitor::thumb32_LDREXH(Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    if (t == Reg::PC || t == Reg::R13 || t == Reg::R14 || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    ir.SetRegister(t, ir.ZeroExtendHalfToWord(ir.ExclusiveReadMemory16(address)));
    return true;
}

// UXTH<c>.W <Rd>, <Rm>{, <rotation>}
bool ThumbTranslatorVisitor::thumb32_UXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

// LDREX<c> <Rt>, [<Rn>{, #<imm>}]
bool ThumbTranslatorVisitor::thumb32_LDREX(Reg n, Reg t, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }

    if (t == Reg::R13 || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    ir.SetRegister(t, ir.ExclusiveReadMemory32(address));
    return true;
}

// STREX<c> <Rd>, <Rt>, [<Rn>{, #<imm>}]
bool ThumbTranslatorVisitor::thumb32_STREX(Reg n, Reg t, Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }

    if (n == Reg::PC || d == Reg::R13 || d == Reg::PC || t == Reg::R13 || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const auto offset = ir.Imm32(imm32);
    const auto address = Helper::GetAddress(ir, true, true, false, n, offset);
    const auto value = ir.GetRegister(t);
    const auto passed = ir.ExclusiveWriteMemory32(address, value);
    ir.SetRegister(d, passed);
    return true;
}

// PLD{W}<c> [<Rn>, #<imm12>]
bool ThumbTranslatorVisitor::thumb32_PLD_imm12(bool W,
                                       [[maybe_unused]] Reg n,
                                       [[maybe_unused]] Imm<12> imm12) {
    if (!options.hook_hint_instructions) {
        return true;
    }

    bool is_pldw = W;
    const auto exception = !is_pldw ? Exception::PreloadData
                             : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

// MLA<c> <Rd>, <Rn>, <Rm>, <Ra>
bool ThumbTranslatorVisitor::thumb32_MLA(Reg n, Reg a, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || n == Reg::R13 || m == Reg::R13 || a == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto result = ir.Add(ir.Mul(ir.GetRegister(n), ir.GetRegister(m)), ir.GetRegister(a));
    ir.SetRegister(d, result);
    return true;
}

// MUL<c> <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_MUL(Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto result = ir.Mul(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// MLS<c> <Rd>, <Rn>, <Rm>, <Ra>
bool ThumbTranslatorVisitor::thumb32_MLS(Reg n, Reg a, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if (d == Reg::PC || a == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || a == Reg::R13 || m == Reg::R13 || n == Reg::R13) {
        return UnpredictableInstruction();
    }

    const IR::U32 operand1 = ir.GetRegister(n);
    const IR::U32 operand2 = ir.GetRegister(m);
    const IR::U32 operand3 = ir.GetRegister(a);
    const IR::U32 result = ir.Sub(operand3, ir.Mul(operand1, operand2));

    ir.SetRegister(d, result);
    return true;
}

// CLZ<c> <Rd>, <Rm>
bool ThumbTranslatorVisitor::thumb32_CLZ(Reg m1, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if(m1 != m) {
        return UnpredictableInstruction();
    }

    if (d == Reg::R13 || d == Reg::PC || m == Reg::R13 || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    ir.SetRegister(d, ir.CountLeadingZeros(ir.GetRegister(m)));
    return true;
}

// LSR{S}<c>.W <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_LSR_reg(bool S, Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if(d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(d == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.LogicalShiftRight(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// ROR{S}<c>.W <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_ROR_reg(bool S, Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if(d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(d == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.RotateRight(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// LSL{S}<c>.W <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_LSL_reg(bool S, Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if(d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if(d == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.LogicalShiftLeft(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// TBB<c> [<Rn>, <Rm>]
bool ThumbTranslatorVisitor::thumb32_TBB(Reg n, Reg m) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    if(n == Reg::R13 || m == Reg::R13 || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto index = ir.GetRegister(m);
    const auto table_address = ir.Add(ir.GetRegister(n), index);
    const auto byte = ir.ZeroExtendByteToWord(ir.ReadMemory8(table_address));
    const auto pc_relative = ir.LogicalShiftLeft(byte, ir.Imm8(1), ir.Imm1(false));

    ir.BranchWritePC(ir.Add(ir.GetRegister(Reg::PC), pc_relative.result));
    ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

// TBH<c> [<Rn>, <Rm>, LSL #1]
bool ThumbTranslatorVisitor::thumb32_TBH(Reg n, Reg m) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    if(n == Reg::R13 || m == Reg::R13 || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto index = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(1), ir.Imm1(false));
    const auto table_address = ir.Add(ir.GetRegister(n), index.result);
    const auto half_word = ir.ZeroExtendHalfToWord(ir.ReadMemory16(table_address));
    const auto pc_relative = ir.LogicalShiftLeft(half_word, ir.Imm8(1), ir.Imm1(false));

    ir.BranchWritePC(ir.Add(ir.GetRegister(Reg::PC), pc_relative.result));
    ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

// REV<c> <Rd>, <Rm>
bool ThumbTranslatorVisitor::thumb32_REV(Reg m1, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if(m1 != m) {
        return UnpredictableInstruction();
    }
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto result = ir.ByteReverseWord(ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// REV16<c> <Rd>, <Rm>
bool ThumbTranslatorVisitor::thumb32_REV16(Reg m1, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if(m1 != m) {
        return UnpredictableInstruction();
    }
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto lo = ir.And(ir.LogicalShiftRight(reg_m, ir.Imm8(8), ir.Imm1(false)).result, ir.Imm32(0x00FF00FF));
    const auto hi = ir.And(ir.LogicalShiftLeft(reg_m, ir.Imm8(8), ir.Imm1(false)).result, ir.Imm32(0xFF00FF00));
    const auto result = ir.Or(lo, hi);

    ir.SetRegister(d, result);
    return true;
}

// RBIT<c> <Rd>, <Rm>
bool ThumbTranslatorVisitor::thumb32_RBIT(Reg m1, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if(m1 != m) {
        return UnpredictableInstruction();
    }

    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const IR::U32 swapped = ir.ByteReverseWord(ir.GetRegister(m));

    // ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4)
    const IR::U32 first_lsr = ir.LogicalShiftRight(ir.And(swapped, ir.Imm32(0xF0F0F0F0)), ir.Imm8(4));
    const IR::U32 first_lsl = ir.LogicalShiftLeft(ir.And(swapped, ir.Imm32(0x0F0F0F0F)), ir.Imm8(4));
    const IR::U32 corrected = ir.Or(first_lsl, first_lsr);

    // ((x & 0x88888888) >> 3) | ((x & 0x44444444) >> 1) |
    // ((x & 0x22222222) << 1) | ((x & 0x11111111) << 3)
    const IR::U32 second_lsr = ir.LogicalShiftRight(ir.And(corrected, ir.Imm32(0x88888888)), ir.Imm8(3));
    const IR::U32 third_lsr = ir.LogicalShiftRight(ir.And(corrected, ir.Imm32(0x44444444)), ir.Imm8(1));
    const IR::U32 second_lsl = ir.LogicalShiftLeft(ir.And(corrected, ir.Imm32(0x22222222)), ir.Imm8(1));
    const IR::U32 third_lsl = ir.LogicalShiftLeft(ir.And(corrected, ir.Imm32(0x11111111)), ir.Imm8(3));

    const IR::U32 result = ir.Or(ir.Or(ir.Or(second_lsr, third_lsr), second_lsl), third_lsl);

    ir.SetRegister(d, result);
    return true;
}

// UBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
bool ThumbTranslatorVisitor::thumb32_UBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1) {
    if (!ConditionPassed()) {
        return true;
    }

    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || n == Reg::R13) {
        return UnpredictableInstruction();
    }

    const u32 lsb_value = concatenate(imm3, imm2).ZeroExtend();
    const u32 widthm1_value = widthm1.ZeroExtend();
    const u32 msb = lsb_value + widthm1_value;
    if (msb >= Common::BitSize<u32>()) {
        return UnpredictableInstruction();
    }

    const IR::U32 operand = ir.GetRegister(n);
    const IR::U32 mask = ir.Imm32(Common::Ones<u32>(widthm1_value + 1));
    const IR::U32 result = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(u8(lsb_value))), mask);

    ir.SetRegister(d, result);
    return true;
}

// SXTB<c>.W <Rd>, <Rm>{, <rotation>}
bool ThumbTranslatorVisitor::thumb32_SXTB(Reg d, SignExtendRotation rotate, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated));

    ir.SetRegister(d, result);
    return true;
}

// UXTB<c>.W <Rd>, <Rm>{, <rotation>}
bool ThumbTranslatorVisitor::thumb32_UXTB(Reg d, SignExtendRotation rotate, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC || d == Reg::R13 || m == Reg::PC || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated));
    ir.SetRegister(d, result);
    return true;
}

// UXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool ThumbTranslatorVisitor::thumb32_UXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::PC || m == Reg::PC || n == Reg::R13) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated)));

    ir.SetRegister(d, result);
    return true;
}

// UMULL<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_UMULL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }

    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (dLo == Reg::R13 || dHi == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Mul(n64, m64);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    return true;
}

// SXTH<c> <Rd>, <Rm>{, <rotation>}
bool ThumbTranslatorVisitor::thumb32_SXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

// SDIV<c> <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_SDIV(Reg n, Reg d, Reg m) {
    return DivideOperation(*this, d, m, n, &IREmitter::SignedDiv);
}

// UADD8<c> <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_UADD8(Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == Reg::R13 || n == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto result = ir.PackedAddU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SEL<c> <Rd>, <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb32_SEL(Reg n, Reg d, Reg m) {
    if (!ConditionPassed()) {
        return true;
    }
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (n == Reg::R13 || d == Reg::R13 || m == Reg::R13) {
        return UnpredictableInstruction();
    }

    const auto to = ir.GetRegister(m);
    const auto from = ir.GetRegister(n);
    const auto result = ir.PackedSelect(ir.GetGEFlags(), to, from);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
