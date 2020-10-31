/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <dynarmic/A32/config.h>

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

// LSL <Rd>, <Rm>, #<imm5>
[[maybe_unused]] bool ThumbTranslatorVisitor::thumb16_LSL_imm(Imm<5> imm5, Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const u8 shift_n = imm5.ZeroExtend<u8>();
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// LSR <Rd>, <Rm>, #<imm5>
bool ThumbTranslatorVisitor::thumb16_LSR_imm(Imm<5> imm5, Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const u8 shift_n = imm5 != 0 ? imm5.ZeroExtend<u8>() : u8(32);
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.LogicalShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// ASR <Rd>, <Rm>, #<imm5>
bool ThumbTranslatorVisitor::thumb16_ASR_imm(Imm<5> imm5, Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const u8 shift_n = imm5 != 0 ? imm5.ZeroExtend<u8>() : u8(32);
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.ArithmeticShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// ADD <Rd>, <Rn>, <Rm>
// Note that it is not possible to encode Rd == R15.
bool ThumbTranslatorVisitor::thumb16_ADD_reg_t1(Reg m, Reg n, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(false));
    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// SUB <Rd>, <Rn>, <Rm>
// Note that it is not possible to encode Rd == R15.
bool ThumbTranslatorVisitor::thumb16_SUB_reg(Reg m, Reg n, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(true));
    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// ADD <Rd>, <Rn>, #<imm3>
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_ADD_imm_t1(Imm<3> imm3, Reg n, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm3.ZeroExtend();
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(false));

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// SUB <Rd>, <Rn>, #<imm3>
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_SUB_imm_t1(Imm<3> imm3, Reg n, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm3.ZeroExtend();
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// MOV <Rd>, #<imm8>
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_MOV_imm(Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend();
    const auto result = ir.Imm32(imm32);

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// CMP <Rn>, #<imm8>
bool ThumbTranslatorVisitor::thumb16_CMP_imm(Reg n, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend();
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// ADD <Rdn>, #<imm8>
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_ADD_imm_t2(Reg d_n, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend();
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(false));

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// SUB <Rd>, <Rn>, #<imm3>
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_SUB_imm_t2(Reg d_n, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend();
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(true));

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// AND <Rdn>, <Rm>
// Note that it is not possible to encode Rdn == R15.
bool ThumbTranslatorVisitor::thumb16_AND_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.And(ir.GetRegister(n), ir.GetRegister(m));

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// EOR <Rdn>, <Rm>
// Note that it is not possible to encode Rdn == R15.
bool ThumbTranslatorVisitor::thumb16_EOR_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.Eor(ir.GetRegister(n), ir.GetRegister(m));

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// LSL <Rdn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_LSL_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto apsr_c = ir.GetCFlag();
    const auto result_carry = ir.LogicalShiftLeft(ir.GetRegister(n), shift_n, apsr_c);

    ir.SetRegister(d, result_carry.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result_carry.result));
        ir.SetZFlag(ir.IsZero(result_carry.result));
        ir.SetCFlag(result_carry.carry);
    }
    return true;
}

// LSR <Rdn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_LSR_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.LogicalShiftRight(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// ASR <Rdn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_ASR_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.ArithmeticShiftRight(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// ADC <Rdn>, <Rm>
// Note that it is not possible to encode Rd == R15.
bool ThumbTranslatorVisitor::thumb16_ADC_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto aspr_c = ir.GetCFlag();
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), aspr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// SBC <Rdn>, <Rm>
// Note that it is not possible to encode Rd == R15.
bool ThumbTranslatorVisitor::thumb16_SBC_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto aspr_c = ir.GetCFlag();
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), aspr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// ROR <Rdn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_ROR_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
    const auto cpsr_c = ir.GetCFlag();
    const auto result = ir.RotateRight(ir.GetRegister(n), shift_n, cpsr_c);

    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    return true;
}

// TST <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_TST_reg(Reg m, Reg n) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.And(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    return true;
}

// RSB <Rd>, <Rn>, #0
// Rd can never encode R15.
bool ThumbTranslatorVisitor::thumb16_RSB_imm(Reg n, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.SubWithCarry(ir.Imm32(0), ir.GetRegister(n), ir.Imm1(true));
    ir.SetRegister(d, result.result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// CMP <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_CMP_reg_t1(Reg m, Reg n) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(true));
    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMN <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_CMN_reg(Reg m, Reg n) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(false));
    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// ORR <Rdn>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_ORR_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.Or(ir.GetRegister(m), ir.GetRegister(n));

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// MUL <Rdn>, <Rm>, <Rdn>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_MUL_reg(Reg n, Reg d_m) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_m;
    const Reg m = d_m;
    const auto result = ir.Mul(ir.GetRegister(m), ir.GetRegister(n));

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// BIC <Rdn>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_BIC_reg(Reg m, Reg d_n) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_n;
    const Reg n = d_n;
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.GetRegister(m)));

    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// MVN <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_MVN_reg(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto result = ir.Not(ir.GetRegister(m));
    ir.SetRegister(d, result);
    if (!ir.current_location.IT().IsInITBlock()) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }
    return true;
}

// ADD <Rdn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d_n = d_n_hi ? (d_n_lo + 8) : d_n_lo;
    const Reg n = d_n;
    if (n == Reg::PC && m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const Reg d = d_n;
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(false));
    if (d == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.ALUWritePC(result.result);
        // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    } else {
        ir.SetRegister(d, result.result);
        return true;
    }
}

// CMP <Rn>, <Rm>
bool ThumbTranslatorVisitor::thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg n = n_hi ? (n_lo + 8) : n_lo;
    if (n < Reg::R8 && m < Reg::R8) {
        return UnpredictableInstruction();
    }
    if (n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(true));
    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// MOV <Rd>, <Rm>
bool ThumbTranslatorVisitor::thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo) {
    if (!ConditionPassed()) {
        return true;
    }
    const Reg d = d_hi ? (d_lo + 8) : d_lo;
    const auto result = ir.GetRegister(m);

    if (d == Reg::PC) {
        const auto it = ir.current_location.IT();
        if (it.IsInITBlock() && !it.IsLastInITBlock()) {
            return UnpredictableInstruction();
        }
        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    } else {
        ir.SetRegister(d, result);
        return true;
    }
}

// LDR <Rt>, <label>
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDR_literal(Reg t, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const u32 address = ir.AlignPC(4) + imm32;
    const auto data = ir.ReadMemory32(ir.Imm32(address));

    ir.SetRegister(t, data);
    return true;
}

// STR <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STR_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.GetRegister(t);

    ir.WriteMemory32(address, data);
    return true;
}

// STRH <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STRH_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.LeastSignificantHalf(ir.GetRegister(t));

    ir.WriteMemory16(address, data);
    return true;
}

// STRB <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STRB_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.LeastSignificantByte(ir.GetRegister(t));

    ir.WriteMemory8(address, data);
    return true;
}

// LDRSB <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDRSB_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDR <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDR_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.ReadMemory32(address);

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDRH_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDRB_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, <Rm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDRSH_reg(Reg m, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto address = ir.Add(ir.GetRegister(n), ir.GetRegister(m));
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// STR <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STR_imm_t1(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend() << 2U;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.GetRegister(t);

    ir.WriteMemory32(address, data);
    return true;
}

// LDR <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDR_imm_t1(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend() << 2U;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.ReadMemory32(address);

    ir.SetRegister(t, data);
    return true;
}

// STRB <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STRB_imm(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend();
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.LeastSignificantByte(ir.GetRegister(t));

    ir.WriteMemory8(address, data);
    return true;
}

// LDRB <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDRB_imm(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend();
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// STRH <Rt>, [<Rn>, #<imm5>]
bool ThumbTranslatorVisitor::thumb16_STRH_imm(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend() << 1U;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.LeastSignificantHalf(ir.GetRegister(t));

    ir.WriteMemory16(address, data);
    return true;
}

// LDRH <Rt>, [<Rn>, #<imm5>]
bool ThumbTranslatorVisitor::thumb16_LDRH_imm(Imm<5> imm5, Reg n, Reg t) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm5.ZeroExtend() << 1U;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// STR <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_STR_imm_t2(Reg t, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const Reg n = Reg::SP;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.GetRegister(t);

    ir.WriteMemory32(address, data);
    return true;
}

// LDR <Rt>, [<Rn>, #<imm>]
// Rt cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_LDR_imm_t2(Reg t, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const Reg n = Reg::SP;
    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm32));
    const auto data = ir.ReadMemory32(address);

    ir.SetRegister(t, data);
    return true;
}

// ADR <Rd>, <label>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_ADR(Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const auto result = ir.Imm32(ir.AlignPC(4) + imm32);

    ir.SetRegister(d, result);
    return true;
}

// ADD <Rd>, SP, #<imm>
bool ThumbTranslatorVisitor::thumb16_ADD_sp_t1(Reg d, Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend() << 2U;
    const auto result = ir.AddWithCarry(ir.GetRegister(Reg::SP), ir.Imm32(imm32), ir.Imm1(false));

    ir.SetRegister(d, result.result);
    return true;
}

// ADD SP, SP, #<imm>
bool ThumbTranslatorVisitor::thumb16_ADD_sp_t2(Imm<7> imm7) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm7.ZeroExtend() << 2U;
    const Reg d = Reg::SP;
    const auto result = ir.AddWithCarry(ir.GetRegister(Reg::SP), ir.Imm32(imm32), ir.Imm1(false));

    ir.SetRegister(d, result.result);
    return true;
}

// SUB SP, SP, #<imm>
bool ThumbTranslatorVisitor::thumb16_SUB_sp(Imm<7> imm7) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm7.ZeroExtend() << 2U;
    const Reg d = Reg::SP;
    const auto result = ir.SubWithCarry(ir.GetRegister(Reg::SP), ir.Imm32(imm32), ir.Imm1(true));

    ir.SetRegister(d, result.result);
    return true;
}

// NOP<c>
bool ThumbTranslatorVisitor::thumb16_NOP() {
    if (!ConditionPassed()) {
        return true;
    }
    return true;
}

// SEV<c>
bool ThumbTranslatorVisitor::thumb16_SEV() {
    if (!ConditionPassed()) {
        return true;
    }
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::SendEvent);
}

// SEVL<c>
bool ThumbTranslatorVisitor::thumb16_SEVL() {
    if (!ConditionPassed()) {
        return true;
    }
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::SendEventLocal);
}

// WFE<c>
bool ThumbTranslatorVisitor::thumb16_WFE() {
    if (!ConditionPassed()) {
        return true;
    }
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::WaitForEvent);
}

// WFI<c>
bool ThumbTranslatorVisitor::thumb16_WFI() {
    if (!ConditionPassed()) {
        return true;
    }
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::WaitForInterrupt);
}

// YIELD<c>
bool ThumbTranslatorVisitor::thumb16_YIELD() {
    if (!ConditionPassed()) {
        return true;
    }
    if (!options.hook_hint_instructions) {
        return true;
    }
    return RaiseException(Exception::Yield);
}

// SXTH <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_SXTH(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto half = ir.LeastSignificantHalf(ir.GetRegister(m));
    ir.SetRegister(d, ir.SignExtendHalfToWord(half));
    return true;
}

// SXTB <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_SXTB(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto byte = ir.LeastSignificantByte(ir.GetRegister(m));
    ir.SetRegister(d, ir.SignExtendByteToWord(byte));
    return true;
}

// UXTH <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_UXTH(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto half = ir.LeastSignificantHalf(ir.GetRegister(m));
    ir.SetRegister(d, ir.ZeroExtendHalfToWord(half));
    return true;
}

// UXTB <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_UXTB(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto byte = ir.LeastSignificantByte(ir.GetRegister(m));
    ir.SetRegister(d, ir.ZeroExtendByteToWord(byte));
    return true;
}

// PUSH <reg_list>
// reg_list cannot encode for R15.
bool ThumbTranslatorVisitor::thumb16_PUSH(bool M, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (M) {
        reg_list |= 1U << 14U;
    }
    if (Common::BitCount(reg_list) < 1) {
        return UnpredictableInstruction();
    }

    const u32 num_bytes_to_push = static_cast<u32>(4 * Common::BitCount(reg_list));
    const auto final_address = ir.Sub(ir.GetRegister(Reg::SP), ir.Imm32(num_bytes_to_push));
    auto address = final_address;
    for (size_t i = 0; i < 16; i++) {
        if (Common::Bit(i, reg_list)) {
            // TODO: Deal with alignment
            const auto Ri = ir.GetRegister(static_cast<Reg>(i));
            ir.WriteMemory32(address, Ri);
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    ir.SetRegister(Reg::SP, final_address);
    // TODO(optimization): Possible location for an RSB push.
    return true;
}

// POP <reg_list>
bool ThumbTranslatorVisitor::thumb16_POP(bool P, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (P) {
        reg_list |= 1U << 15U;
    }
    if (Common::BitCount(reg_list) < 1) {
        return UnpredictableInstruction();
    }

    auto address = ir.GetRegister(Reg::SP);
    for (size_t i = 0; i < 15; i++) {
        if (Common::Bit(i, reg_list)) {
            // TODO: Deal with alignment
            const auto data = ir.ReadMemory32(address);
            ir.SetRegister(static_cast<Reg>(i), data);
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    if (Common::Bit<15>(reg_list)) {
        // TODO(optimization): Possible location for an RSB pop.
        const auto data = ir.ReadMemory32(address);
        ir.LoadWritePC(data);
        address = ir.Add(address, ir.Imm32(4));
        ir.SetRegister(Reg::SP, address);
        ir.SetTerm(IR::Term::PopRSBHint{});
        return false;
    } else {
        ir.SetRegister(Reg::SP, address);
        return true;
    }
}

// SETEND <endianness>
bool ThumbTranslatorVisitor::thumb16_SETEND(bool E) {
    if (!ConditionPassed()) {
        return true;
    }
    if (E == ir.current_location.EFlag()) {
        return true;
    }

    ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(2).SetEFlag(E)});
    return false;
}

// CPS{IE,ID} <a,i,f>
// A CPS is treated as a NOP in User mode.
bool ThumbTranslatorVisitor::thumb16_CPS(bool, bool, bool, bool) {
    if (!ConditionPassed()) {
        return true;
    }
    return true;
}

// REV <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_REV(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    ir.SetRegister(d, ir.ByteReverseWord(ir.GetRegister(m)));
    return true;
}

// REV16 <Rd>, <Rm>
// Rd cannot encode R15.
// TODO: Consider optimizing
bool ThumbTranslatorVisitor::thumb16_REV16(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto Rm = ir.GetRegister(m);
    const auto upper_half = ir.LeastSignificantHalf(ir.LogicalShiftRight(Rm, ir.Imm8(16), ir.Imm1(false)).result);
    const auto lower_half = ir.LeastSignificantHalf(Rm);
    const auto rev_upper_half = ir.ZeroExtendHalfToWord(ir.ByteReverseHalf(upper_half));
    const auto rev_lower_half = ir.ZeroExtendHalfToWord(ir.ByteReverseHalf(lower_half));
    const auto result = ir.Or(ir.LogicalShiftLeft(rev_upper_half, ir.Imm8(16), ir.Imm1(false)).result,
                              rev_lower_half);

    ir.SetRegister(d, result);
    return true;
}

// REVSH <Rd>, <Rm>
// Rd cannot encode R15.
bool ThumbTranslatorVisitor::thumb16_REVSH(Reg m, Reg d) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto rev_half = ir.ByteReverseHalf(ir.LeastSignificantHalf(ir.GetRegister(m)));
    ir.SetRegister(d, ir.SignExtendHalfToWord(rev_half));
    return true;
}

// BKPT #<imm8>
bool ThumbTranslatorVisitor::thumb16_BKPT([[maybe_unused]] Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }

    ir.ExceptionRaised(Exception::Breakpoint);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

// STM <Rn>!, <reg_list>
bool ThumbTranslatorVisitor::thumb16_STMIA(Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (Common::BitCount(reg_list) == 0) {
        return UnpredictableInstruction();
    }
    if (Common::Bit(static_cast<size_t>(n), reg_list) && n != static_cast<Reg>(Common::LowestSetBit(reg_list))) {
        return UnpredictableInstruction();
    }

    auto address = ir.GetRegister(n);
    for (size_t i = 0; i < 8; i++) {
        if (Common::Bit(i, reg_list)) {
            const auto Ri = ir.GetRegister(static_cast<Reg>(i));
            ir.WriteMemory32(address, Ri);
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    ir.SetRegister(n, address);
    return true;
}

// LDM <Rn>!, <reg_list>
bool ThumbTranslatorVisitor::thumb16_LDMIA(Reg n, RegList reg_list) {
    if (!ConditionPassed()) {
        return true;
    }
    if (Common::BitCount(reg_list) == 0) {
        return UnpredictableInstruction();
    }

    const bool write_back = !Common::Bit(static_cast<size_t>(n), reg_list);
    auto address = ir.GetRegister(n);

    for (size_t i = 0; i < 8; i++) {
        if (Common::Bit(i, reg_list)) {
            const auto data = ir.ReadMemory32(address);
            ir.SetRegister(static_cast<Reg>(i), data);
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    if (write_back) {
        ir.SetRegister(n, address);
    }
    return true;
}

// CB{N}Z <Rn>, <label>
bool ThumbTranslatorVisitor::thumb16_CBZ_CBNZ(bool nonzero, Imm<1> i, Imm<5> imm5, Reg n) {
    if (ir.current_location.IT().IsInITBlock()) {
        return UnpredictableInstruction();
    }
    const u32 imm = concatenate(i, imm5, Imm<1>{0}).ZeroExtend();
    const IR::U32 rn = ir.GetRegister(n);

    ir.SetCheckBit(ir.IsZero(rn));

    const auto [cond_pass, cond_fail] = [this, imm, nonzero] {
        const u32 target = ir.PC() + imm;
        const auto skip = IR::Term::LinkBlock{ir.current_location.AdvancePC(2)};
        const auto branch = IR::Term::LinkBlock{ir.current_location.SetPC(target)};

        if (nonzero) {
            return std::make_pair(skip, branch);
        } else {
            return std::make_pair(branch, skip);
        }
    }();

    ir.SetTerm(IR::Term::CheckBit{cond_pass, cond_fail});
    return false;
}

bool ThumbTranslatorVisitor::thumb16_UDF() {
    return UndefinedInstruction();
}

// BX <Rm>
bool ThumbTranslatorVisitor::thumb16_BX(Reg m) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
    ir.BXWritePC(ir.GetRegister(m));
    if (m == Reg::R14)
        ir.SetTerm(IR::Term::PopRSBHint{});
    else
        ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

// BLX <Rm>
bool ThumbTranslatorVisitor::thumb16_BLX_reg(Reg m) {
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && !it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
    ir.PushRSB(ir.current_location.AdvancePC(2));
    ir.BXWritePC(ir.GetRegister(m));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 2) | 1U));
    ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

// SVC #<imm8>
bool ThumbTranslatorVisitor::thumb16_SVC(Imm<8> imm8) {
    if (!ConditionPassed()) {
        return true;
    }
    const u32 imm32 = imm8.ZeroExtend();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 2));
    ir.PushRSB(ir.current_location.AdvancePC(2));
    ir.CallSupervisor(ir.Imm32(imm32));
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::PopRSBHint{}});
    return false;
}

// B<cond> <label>
bool ThumbTranslatorVisitor::thumb16_B_t1(Cond cond, Imm<8> imm8) {
    if (ir.current_location.IT().IsInITBlock()) {
        return UnpredictableInstruction();
    }
    if (cond == Cond::AL) {
        return thumb16_UDF();
    }

    const s32 imm32 = static_cast<s32>((imm8.SignExtend<u32>() << 1U) + 4);
    const auto then_location = ir.current_location.AdvancePC(imm32);
    const auto else_location = ir.current_location.AdvancePC(2);

    ir.SetTerm(IR::Term::If{cond, IR::Term::LinkBlock{then_location}, IR::Term::LinkBlock{else_location}});
    return false;
}

// B <label>
bool ThumbTranslatorVisitor::thumb16_B_t2(Imm<11> imm11) {
    if (!ConditionPassed()) {
        return true;
    }
    const auto it = ir.current_location.IT();
    if (it.IsInITBlock() && ! it.IsLastInITBlock()) {
        return UnpredictableInstruction();
    }
    const s32 imm32 = static_cast<s32>((imm11.SignExtend<u32>() << 1U) + 4);
    const auto next_location = ir.current_location.AdvancePC(imm32);

    ir.SetTerm(IR::Term::LinkBlock{next_location});
    return false;
}

// IT{x{y{z}}} <firstcond>
bool ThumbTranslatorVisitor::thumb16_IT(Cond firstcond, Imm<4> mask) {
    if (mask == 0b0000) {
        return UndefinedInstruction();
    }
    if (firstcond == Cond::NV) {
       // NV conditional is obsolete
       return UnpredictableInstruction();
    }
    u32 mask_value = mask.ZeroExtend();
    if (firstcond == Cond::AL && Common::BitCount(mask_value) != 1) {
        return UndefinedInstruction();
    }
    if (ir.current_location.IT().IsInITBlock()) {
        return UndefinedInstruction();
    }

    auto new_it = ITState(0);
    new_it.Cond(firstcond);
    new_it.Mask(mask_value);

    ir.current_location = ir.current_location.SetIT(new_it);
    ir.current_location = ir.current_location.AdvancePC(2);
    ir.SetTerm(IR::Term::LinkBlockFast{ir.current_location});
    cond_state = ConditionalState::Break;
    return true;
}

} // namespace Dynarmic::A32
