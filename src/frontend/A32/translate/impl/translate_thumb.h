/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/assert.h"
#include "frontend/imm.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/A32/types.h"
#include "frontend/A32/translate/helper.h"

namespace Dynarmic::A32 {

enum class ConditionalState {
    /// We haven't met any conditional instructions yet.
    None,
    /// Current instruction is with a new condition code. This marks the end of this basic block.
    Break,
    /// This basic block is made up solely of conditional instructions.
    Translating,
    /// This basic block is made up of conditional instructions followed by unconditional instructions.
    Trailing,
};

enum class Exception;

struct ThumbTranslatorVisitor final {
    using instruction_return_type = bool;

    explicit ThumbTranslatorVisitor(IR::Block& block, LocationDescriptor descriptor, const TranslationOptions& options) : ir(block, descriptor), options(options) {
        ASSERT_MSG(descriptor.TFlag(), "The processor must be in Thumb mode");
    }
    
    static u32 ThumbExpandImm(Imm<1> i, Imm<3> imm3, Imm<8> imm8) {
        const auto imm12 = concatenate(i, imm3, imm8);
        if (imm12.Bits<10, 11>() == 0) {
            const u32 imm8 = imm12.Bits<0, 7>();
            switch (imm12.Bits<8, 9>()) {
                case 0b00:
                    return imm8;
                case 0b01:
                    return (imm8 << 16) | imm8;
                case 0b10:
                    return (imm8 << 24) | (imm8 << 8);
                case 0b11:
                    return Common::Replicate(imm8, 8);
            }
            assert(false);
        }
        const int rotate = imm12.Bits<7, 11>();
        const Imm<8> unrotated_value = concatenate(Imm<1>(1), Imm<7>(imm12.Bits<0, 6>()));
        return Common::RotateRight<u32>(unrotated_value.ZeroExtend(), rotate);
    }
    
    struct ImmAndCarry {
        u32 imm32;
        IR::U1 carry;
    };
    
    ImmAndCarry ThumbExpandImm_C(Imm<1> i, Imm<3> imm3, Imm<8> imm8, IR::U1 carry_in) {
        const u32 imm32 = ThumbExpandImm(i, imm3, imm8);
        auto carry_out = carry_in;
        if (imm3.Bit<2>() != 0 || i != 0) {
            carry_out = ir.Imm1(Common::Bit<31>(imm32));
        }
        return {imm32, carry_out};
    }

    IR::ResultAndCarry<IR::U32> DecodeShiftedReg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, IR::U1 carry_in) {
        const auto reg = ir.GetRegister(n);
        switch (t.ZeroExtend()) {
            case 0b00: {
                const auto shift_n = concatenate(imm3, imm2).ZeroExtend();
                const auto result = ir.LogicalShiftLeft(reg, ir.Imm8(shift_n), carry_in);
                return result;
            }
            case 0b01: {
                auto shift_n = concatenate(imm3, imm2).ZeroExtend();
                if (shift_n == 0) {
                    shift_n = 32;
                }
                const auto result = ir.LogicalShiftRight(reg, ir.Imm8(shift_n), carry_in);
                return result;
            }
            case 0b10: {
                auto shift_n = concatenate(imm3, imm2).ZeroExtend();
                if (shift_n == 0) {
                    shift_n = 32;
                }
                const auto result = ir.ArithmeticShiftRight(reg, ir.Imm8(shift_n), carry_in);
                return result;
            } 
            case 0b11: {
                auto shift_n = concatenate(imm3, imm2).ZeroExtend();
                if (shift_n == 0) {
                    const auto result = ir.RotateRightExtended(reg, carry_in);
                    return result;
                }
                const auto result = ir.RotateRight(reg, ir.Imm8(shift_n), carry_in);
                return result;
            }  
        }
        assert(false);
    }

    A32::IREmitter ir;
    TranslationOptions options;
    
    ConditionalState cond_state = ConditionalState::None;
    
    bool is_thumb_16;

    bool ConditionPassed();
    bool InterpretThisInstruction();
    bool UnpredictableInstruction();
    bool UndefinedInstruction();
    bool RaiseException(Exception exception);

    // thumb16
    bool thumb16_LSL_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_LSR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ASR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ADD_reg_t1(Reg m, Reg n, Reg d);
    bool thumb16_SUB_reg(Reg m, Reg n, Reg d);
    bool thumb16_ADD_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_SUB_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_MOV_imm(Reg d, Imm<8> imm8);
    bool thumb16_CMP_imm(Reg n, Imm<8> imm8);
    bool thumb16_ADD_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_SUB_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_AND_reg(Reg m, Reg d_n);
    bool thumb16_EOR_reg(Reg m, Reg d_n);
    bool thumb16_LSL_reg(Reg m, Reg d_n);
    bool thumb16_LSR_reg(Reg m, Reg d_n);
    bool thumb16_ASR_reg(Reg m, Reg d_n);
    bool thumb16_ADC_reg(Reg m, Reg d_n);
    bool thumb16_SBC_reg(Reg m, Reg d_n);
    bool thumb16_ROR_reg(Reg m, Reg d_n);
    bool thumb16_TST_reg(Reg m, Reg n);
    bool thumb16_RSB_imm(Reg n, Reg d);
    bool thumb16_CMP_reg_t1(Reg m, Reg n);
    bool thumb16_CMN_reg(Reg m, Reg n);
    bool thumb16_ORR_reg(Reg m, Reg d_n);
    bool thumb16_MUL_reg(Reg n, Reg d_m);
    bool thumb16_BIC_reg(Reg m, Reg d_n);
    bool thumb16_MVN_reg(Reg m, Reg d);
    bool thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo);
    bool thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo);
    bool thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo);
    bool thumb16_LDR_literal(Reg t, Imm<8> imm8);
    bool thumb16_STR_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDR_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_LDR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_ADR(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t1(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t2(Imm<7> imm7);
    bool thumb16_SUB_sp(Imm<7> imm7);
    bool thumb16_NOP();
    bool thumb16_SEV();
    bool thumb16_SEVL();
    bool thumb16_WFE();
    bool thumb16_WFI();
    bool thumb16_YIELD();
    bool thumb16_SXTH(Reg m, Reg d);
    bool thumb16_SXTB(Reg m, Reg d);
    bool thumb16_UXTH(Reg m, Reg d);
    bool thumb16_UXTB(Reg m, Reg d);
    bool thumb16_PUSH(bool M, RegList reg_list);
    bool thumb16_POP(bool P, RegList reg_list);
    bool thumb16_SETEND(bool E);
    bool thumb16_CPS(bool, bool, bool, bool);
    bool thumb16_REV(Reg m, Reg d);
    bool thumb16_REV16(Reg m, Reg d);
    bool thumb16_REVSH(Reg m, Reg d);
    bool thumb16_BKPT([[maybe_unused]] Imm<8> imm8);
    bool thumb16_STMIA(Reg n, RegList reg_list);
    bool thumb16_LDMIA(Reg n, RegList reg_list);
    bool thumb16_CBZ_CBNZ(bool nonzero, Imm<1> i, Imm<5> imm5, Reg n);
    bool thumb16_UDF();
    bool thumb16_BX(Reg m);
    bool thumb16_BLX_reg(Reg m);
    bool thumb16_SVC(Imm<8> imm8);
    bool thumb16_B_t1(Cond cond, Imm<8> imm8);
    bool thumb16_B_t2(Imm<11> imm11);
    bool thumb16_IT(Cond firstcond, Imm<4> mask);

    // thumb32
    bool thumb32_STMIA(bool W, Reg n, RegList reg_list);
    bool thumb32_LDMIA(bool W, Reg n, RegList reg_list);
    bool thumb32_STMDB(bool W, Reg n, RegList reg_list);
    bool thumb32_LDMDB(bool W, Reg n, RegList reg_list);

    bool thumb32_TST_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_AND_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_BIC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_LSL_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m);
    bool thumb32_LSR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m);
    bool thumb32_ASR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m);
    bool thumb32_RRX(bool S, Reg d, Reg m);
    bool thumb32_ROR_imm(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Reg m);
    bool thumb32_ORR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_MVN_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_ORN_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_TEQ_reg(Reg n, Imm<3> imm3, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_EOR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<2> t, Reg m);
    bool thumb32_PKH(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, bool tb, Reg m);

    bool thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_MVN_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORN_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_TEQ_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_EOR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMN_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_ADD_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_SBC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMP_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_SUB_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_RSB_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BL_imm(Imm<11> hi, Imm<11> lo);
    bool thumb32_BLX_imm(Imm<11> hi, Imm<11> lo);
    bool thumb32_UDF();
};

} // namespace Dynarmic::A32
