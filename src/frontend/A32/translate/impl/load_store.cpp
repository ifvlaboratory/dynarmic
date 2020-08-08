/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_LDRBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_LDRHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_LDRT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_STRBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_STRHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool ArmTranslatorVisitor::arm_STRT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

static IR::U32 GetAddress(A32::IREmitter& ir, bool P, bool U, bool W, Reg n, IR::U32 offset) {
    return Helper::GetAddress(ir, P, U, !P || W, n, offset);
}

// LDR <Rt>, [PC, #+/-<imm>]
bool ArmTranslatorVisitor::arm_LDR_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
    if (!ConditionPassed(cond)) {
        return true;
    }

    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm12.ZeroExtend()) : (base - imm12.ZeroExtend());
    const auto data = ir.ReadMemory32(ir.Imm32(address));

    if (t == Reg::PC) {
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDR <Rt>, [<Rn>, #+/-<imm>]{!}
// LDR <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ReadMemory32(address);

    if (t == Reg::PC) {
        ir.LoadWritePC(data);

        if (!P && W && n == Reg::R13) {
            ir.SetTerm(IR::Term::PopRSBHint{});
        } else {
            ir.SetTerm(IR::Term::FastDispatchHint{});
        }

        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDR <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDR <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ReadMemory32(address);

    if (t == Reg::PC) {
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [PC, #+/-<imm>]
bool ArmTranslatorVisitor::arm_LDRB_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRB <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRB <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRD <Rt>, <Rt2>, [PC, #+/-<imm>]
bool ArmTranslatorVisitor::arm_LDRD_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (t+1 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t+1;
    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;

    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data_a = ir.ReadMemory32(ir.Imm32(address));
    const auto data_b = ir.ReadMemory32(ir.Imm32(address + 4));

    ir.SetRegister(t, data_a);
    ir.SetRegister(t2, data_b);
    return true;
}

// LDRD <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRD <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == t || n == t+1)) {
        return UnpredictableInstruction();
    }

    if (t+1 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t+1;
    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();

    const auto offset = ir.Imm32(imm32);
    const auto address_a = GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto data_a = ir.ReadMemory32(address_a);
    const auto data_b = ir.ReadMemory32(address_b);

    ir.SetRegister(t, data_a);
    ir.SetRegister(t2, data_b);
    return true;
}

// LDRD <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRD <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    if (t+1 == Reg::PC || m == Reg::PC || m == t || m == t+1) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t || n == t+1)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t+1;
    const auto offset = ir.GetRegister(m);
    const auto address_a = GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto data_a = ir.ReadMemory32(address_a);
    const auto data_b = ir.ReadMemory32(address_b);

    ir.SetRegister(t, data_a);
    ir.SetRegister(t2, data_b);
    return true;
}

// LDRH <Rt>, [PC, #-/+<imm>]
bool ArmTranslatorVisitor::arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (P == W) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRH <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRH <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [PC, #+/-<imm>]
bool ArmTranslatorVisitor::arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;

    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRSB <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRSB <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [PC, #-/+<imm>]
bool ArmTranslatorVisitor::arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRSH <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRSH <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

// STR <Rt>, [<Rn>, #+/-<imm>]{!}
// STR <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.Imm32(imm12.ZeroExtend());
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory32(address, ir.GetRegister(t));
    return true;
}

// STR <Rt>, [<Rn>, #+/-<Rm>]{!}
// STR <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory32(address, ir.GetRegister(t));
    return true;
}

// STRB <Rt>, [<Rn>, #+/-<imm>]{!}
// STRB <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.Imm32(imm12.ZeroExtend());
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)));
    return true;
}

// STRB <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRB <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)));
    return true;
}

// STRD <Rt>, [<Rn>, #+/-<imm>]{!}
// STRD <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (size_t(t) % 2 != 0) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    const Reg t2 = t + 1;
    if ((!P || W) && (n == Reg::PC || n == t || n == t2)) {
        return UnpredictableInstruction();
    }

    if (t2 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address_a = GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto value_a = ir.GetRegister(t);
    const auto value_b = ir.GetRegister(t2);

    ir.WriteMemory32(address_a, value_a);
    ir.WriteMemory32(address_b, value_b);
    return true;
}

// STRD <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRD <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (size_t(t) % 2 != 0) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    const Reg t2 = t + 1;
    if (t2 == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t || n == t2)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address_a = GetAddress(ir, P, U, W, n, offset);
    const auto address_b = ir.Add(address_a, ir.Imm32(4));
    const auto value_a = ir.GetRegister(t);
    const auto value_b = ir.GetRegister(t2);

    ir.WriteMemory32(address_a, value_a);
    ir.WriteMemory32(address_b, value_b);
    return true;
}

// STRH <Rt>, [<Rn>, #+/-<imm>]{!}
// STRH <Rt>, [<Rn>], #+/-<imm>
bool ArmTranslatorVisitor::arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)));
    return true;
}

// STRH <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRH <Rt>, [<Rn>], #+/-<Rm>
bool ArmTranslatorVisitor::arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)));
    return true;
}

// LDM <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_LDM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(u32(Common::BitCount(list) * 4)));
    return Helper::LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMDA <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_LDMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list) - 4)));
    const auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
    return Helper::LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMDB <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_LDMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
    const auto writeback_address = start_address;
    return Helper::LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMIB <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_LDMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
    const auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
    return Helper::LDMHelper(ir, W, n, list, start_address, writeback_address);
}

bool ArmTranslatorVisitor::arm_LDM_usr() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDM_eret() {
    return InterpretThisInstruction();
}

// STM <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_STM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(u32(Common::BitCount(list) * 4)));
    return Helper::STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMDA <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_STMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list) - 4)));
    const auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
    return Helper::STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMDB <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_STMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
    const auto writeback_address = start_address;
    return Helper::STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMIB <Rn>{!}, <reg_list>
bool ArmTranslatorVisitor::arm_STMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
    const auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
    return Helper::STMHelper(ir, W, n, list, start_address, writeback_address);
}

bool ArmTranslatorVisitor::arm_STM_usr() {
    return InterpretThisInstruction();
}

} // namespace Dynarmic::A32
