/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

// B<c>.W <label>
bool ThumbTranslatorVisitor::thumb32_B_cond(Imm<1> S, Cond cond, Imm<6> imm6, Imm<1> j1, Imm<2> j2, Imm<11> imm11) {
    if (cond == Cond::AL || cond == Cond::NV) {
        return thumb16_UDF();
    }

    const s32 imm32 = static_cast<s32>((concatenate(S, j2, j1, imm6, imm11).SignExtend<u32>() << 1U) + 4);
    const auto then_location = ir.current_location.AdvancePC(imm32);
    const auto else_location = ir.current_location.AdvancePC(4);

    ir.SetTerm(IR::Term::If{cond, IR::Term::LinkBlock{then_location}, IR::Term::LinkBlock{else_location}});
    return false;
}

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

// PUSH<c>.W <registers>
// reg_list cannot encode for R15.
bool ThumbTranslatorVisitor::thumb32_PUSH(bool M, RegList reg_list) {
    if (M) {
        reg_list |= (1U << 14U);
    }
    if (Common::BitCount(reg_list) < 2) {
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

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
