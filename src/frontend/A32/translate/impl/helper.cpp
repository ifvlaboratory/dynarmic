/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/helper.h"

namespace Dynarmic::A32::Helper {

bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.SetRegister(static_cast<Reg>(i), ir.ReadMemory32(address));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W && !Common::Bit(RegNumber(n), list)) {
        ir.SetRegister(n, writeback_address);
    }
    if (Common::Bit<15>(list)) {
        ir.LoadWritePC(ir.ReadMemory32(address));
        if (n == Reg::R13)
            ir.SetTerm(IR::Term::PopRSBHint{});
        else
            ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    return true;
}

bool STMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W) {
        ir.SetRegister(n, writeback_address);
    }
    if (Common::Bit<15>(list)) {
        ir.WriteMemory32(address, ir.Imm32(ir.PC()));
    }
    return true;
}

void PKHHelper(A32::IREmitter& ir, bool tb, Reg d, IR::U32 n, IR::U32 shifted) {
    const auto lower_used = tb ? shifted : n;
    const auto upper_used = tb ? n : shifted;
    const auto lower_half = ir.And(lower_used, ir.Imm32(0x0000FFFF));
    const auto upper_half = ir.And(upper_used, ir.Imm32(0xFFFF0000));

    ir.SetRegister(d, ir.Or(lower_half, upper_half));
}

IR::U32 Pack2x16To1x32(A32::IREmitter& ir, IR::U32 lo, IR::U32 hi) {
    return ir.Or(ir.And(lo, ir.Imm32(0xFFFF)), ir.LogicalShiftLeft(hi, ir.Imm8(16), ir.Imm1(0)).result);
}

IR::U16 MostSignificantHalf(A32::IREmitter& ir, IR::U32 value) {
    return ir.LeastSignificantHalf(ir.LogicalShiftRight(value, ir.Imm8(16), ir.Imm1(0)).result);
}

void SSAT16Helper(A32::IREmitter& ir, Reg d, Reg n, size_t saturate_to) {
    const auto lo_operand = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(ir.GetRegister(n)));
    const auto hi_operand = ir.SignExtendHalfToWord(MostSignificantHalf(ir, ir.GetRegister(n)));
    const auto lo_result = ir.SignedSaturation(lo_operand, saturate_to);
    const auto hi_result = ir.SignedSaturation(hi_operand, saturate_to);

    ir.SetRegister(d, Pack2x16To1x32(ir, lo_result.result, hi_result.result));
    ir.OrQFlag(lo_result.overflow);
    ir.OrQFlag(hi_result.overflow);
}

void SBFXHelper(A32::IREmitter& ir, Reg d, Reg n, u32 lsbit, u32 width_num) {
//    const u32 msb = lsbit + width_num;
    constexpr size_t max_width = Common::BitSize<u32>();
    const u32 width = width_num + 1;
    const u8 left_shift_amount = static_cast<u8>(max_width - width - lsbit);
    const u8 right_shift_amount = static_cast<u8>(max_width - width);
    const IR::U32 operand = ir.GetRegister(n);
    const IR::U32 tmp = ir.LogicalShiftLeft(operand, ir.Imm8(left_shift_amount));
    const IR::U32 result = ir.ArithmeticShiftRight(tmp, ir.Imm8(right_shift_amount));
    ir.SetRegister(d, result);
}

void BFCHelper(A32::IREmitter& ir, Reg d, u32 lsbit, u32 msbit) {
    const u32 mask = ~(Common::Ones<u32>(msbit - lsbit + 1) << lsbit);
    const IR::U32 operand = ir.GetRegister(d);
    const IR::U32 result = ir.And(operand, ir.Imm32(mask));

    ir.SetRegister(d, result);
}

IR::U32 GetAddress(A32::IREmitter& ir, bool P, bool U, bool W, Reg n, IR::U32 offset) {
    const bool index = P;
    const bool add = U;
    const bool wback = W;

    const IR::U32 offset_addr = add ? ir.Add(ir.GetRegister(n), offset) : ir.Sub(ir.GetRegister(n), offset);
    const IR::U32 address = index ? offset_addr : ir.GetRegister(n);

    if (wback) {
        ir.SetRegister(n, offset_addr);
    }

    return address;
}

} // namespace Dynarmic::A32
