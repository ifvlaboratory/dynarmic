/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"
#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

// CPS<effect> <iflags>{, #<mode>}
// CPS #<mode>
bool ArmTranslatorVisitor::arm_CPS() {
    return InterpretThisInstruction();
}

// MRS<c> <Rd>, <spec_reg>
bool ArmTranslatorVisitor::arm_MRS(Cond cond, Reg d) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    ir.SetRegister(d, ir.GetCpsr());
    return true;
}

// MSR<c> <spec_reg>, #<const>
bool ArmTranslatorVisitor::arm_MSR_imm(Cond cond, int mask, int rotate, Imm<8> imm8) {
    ASSERT_MSG(mask != 0, "Decode error");

    if (!ConditionPassed(cond)) {
        return true;
    }

    const bool write_nzcvq = Common::Bit<3>(mask);
    const bool write_g = Common::Bit<2>(mask);
    const bool write_e = Common::Bit<1>(mask);
    const u32 imm32 = ArmExpandImm(rotate, imm8);

    if (write_nzcvq) {
        ir.SetCpsrNZCVQ(ir.Imm32(imm32 & 0xF8000000));
    }

    if (write_g) {
        ir.SetGEFlagsCompressed(ir.Imm32(imm32 & 0x000F0000));
    }

    if (write_e) {
        const bool E = (imm32 & 0x00000200) != 0;
        if (E != ir.current_location.EFlag()) {
            ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(4).SetEFlag(E)});
            return false;
        }
    }

    return true;
}

// MSR<c> <spec_reg>, <Rn>
bool ArmTranslatorVisitor::arm_MSR_reg(Cond cond, int mask, Reg n) {
    if (mask == 0) {
        return UnpredictableInstruction();
    }

    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const bool write_nzcvq = Common::Bit<3>(mask);
    const bool write_g = Common::Bit<2>(mask);
    const bool write_e = Common::Bit<1>(mask);
    const auto value = ir.GetRegister(n);

    if (!write_e) {
        if (write_nzcvq) {
            ir.SetCpsrNZCVQ(ir.And(value, ir.Imm32(0xF8000000)));
        }

        if (write_g) {
            ir.SetGEFlagsCompressed(ir.And(value, ir.Imm32(0x000F0000)));
        }
    } else {
        const u32 cpsr_mask = (write_nzcvq ? 0xF8000000 : 0) | (write_g ? 0x000F0000 : 0) | 0x00000200;
        const auto old_cpsr = ir.And(ir.GetCpsr(), ir.Imm32(~cpsr_mask));
        const auto new_cpsr = ir.And(value, ir.Imm32(cpsr_mask));
        ir.SetCpsr(ir.Or(old_cpsr, new_cpsr));
        ir.PushRSB(ir.current_location.AdvancePC(4));
        ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
        ir.SetTerm(IR::Term::CheckHalt{IR::Term::PopRSBHint{}});
        return false;
    }

    return true;
}

// RFE{<amode>} <Rn>{!}
bool ArmTranslatorVisitor::arm_RFE() {
    return InterpretThisInstruction();
}

// SETEND <endian_specifier>
bool ArmTranslatorVisitor::arm_SETEND(bool E) {
    ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(4).SetEFlag(E)});
    return false;
}

// SRS{<amode>} SP{!}, #<mode>
bool ArmTranslatorVisitor::arm_SRS() {
    return InterpretThisInstruction();
}

}  // namespace Dynarmic::A32
