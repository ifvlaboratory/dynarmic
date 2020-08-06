/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"
#include "frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template <typename Visitor>
using Thumb32Matcher = Decoder::Matcher<Visitor, u32>;

template<typename V>
std::optional<std::reference_wrapper<const Thumb32Matcher<V>>> DecodeThumb32(u32 instruction) {
    static const std::vector<Thumb32Matcher<V>> table = {

#define INST(fn, name, bitstring) Decoder::detail::detail<Thumb32Matcher<V>>::GetMatcher(fn, name, bitstring)

        // Load/Store Multiple
        //INST(&V::thumb32_SRS_1,          "SRS",                      "1110100000-0--------------------"),
        //INST(&V::thumb32_RFE_2,          "RFE",                      "1110100000-1--------------------"),
        //INST(&V::thumb32_STMIA,          "STMIA/STMEA",              "1110100010-0--------------------"),
        //INST(&V::thumb32_POP,            "POP",                      "1110100010111101----------------"),
        //INST(&V::thumb32_LDMIA,          "LDMIA/LDMFD",              "1110100010-1--------------------"),
        //INST(&V::thumb32_PUSH,           "PUSH",                     "1110100100101101----------------"),
        //INST(&V::thumb32_STMDB,          "STMDB/STMFD",              "1110100100-0--------------------"),
        //INST(&V::thumb32_LDMDB,          "LDMDB/LDMEA",              "1110100100-1--------------------"),
        //INST(&V::thumb32_SRS_1,          "SRS",                      "1110100110-0--------------------"),
        //INST(&V::thumb32_RFE_2,          "RFE",                      "1110100110-1--------------------"),

        // Load/Store Dual, Load/Store Exclusive, Table Branch
        //INST(&V::thumb32_STREX,          "STREX",                    "111010000100--------------------"),
        //INST(&V::thumb32_LDREX,          "LDREX",                    "111010000101--------------------"),
        //INST(&V::thumb32_STRD_imm_1,     "STRD (imm)",               "11101000-110--------------------"),
        //INST(&V::thumb32_STRD_imm_2,     "STRD (imm)",               "11101001-1-0--------------------"),
        //INST(&V::thumb32_LDRD_imm_1,     "LDRD (lit)",               "11101000-1111111----------------"),
        //INST(&V::thumb32_LDRD_imm_2,     "LDRD (lit)",               "11101001-1-11111----------------"),
        //INST(&V::thumb32_LDRD_imm_1,     "LDRD (imm)",               "11101000-111--------------------"),
        //INST(&V::thumb32_LDRD_imm_2,     "LDRD (imm)",               "11101001-1-1--------------------"),
        //INST(&V::thumb32_STREXB,         "STREXB",                   "111010001100------------0100----"),
        //INST(&V::thumb32_STREXH,         "STREXH",                   "111010001100------------0101----"),
        //INST(&V::thumb32_STREXD,         "STREXD",                   "111010001100------------0111----"),
        //INST(&V::thumb32_TBB,            "TBB",                      "111010001101------------0000----"),
        //INST(&V::thumb32_TBH,            "TBH",                      "111010001101------------0001----"),
        //INST(&V::thumb32_LDREXB,         "LDREXB",                   "111010001101------------0100----"),
        //INST(&V::thumb32_LDREXH,         "LDREXH",                   "111010001101------------0101----"),
        //INST(&V::thumb32_LDREXD,         "LDREXD",                   "111010001101------------0111----"),

        // Data Processing (Shifted Register)
        //INST(&V::thumb32_TST_reg,        "TST (reg)",                "111010100001--------1111--------"),
        //INST(&V::thumb32_AND_reg,        "AND (reg)",                "11101010000---------------------"),
        //INST(&V::thumb32_BIC_reg,        "BIC (reg)",                "11101010001---------------------"),
        //INST(&V::thumb32_MOV_reg,        "MOV (reg)",                "11101010010-1111-000----0000----"),
        //INST(&V::thumb32_LSL_imm,        "LSL (imm)",                "11101010010-1111----------00----"),
        //INST(&V::thumb32_LSR_imm,        "LSR (imm)",                "11101010010-1111----------01----"),
        //INST(&V::thumb32_ASR_imm,        "ASR (imm)",                "11101010010-1111----------10----"),
        //INST(&V::thumb32_RRX,            "RRX",                      "11101010010-1111-000----0011----"),
        //INST(&V::thumb32_ROR_imm,        "ROR (imm)",                "11101010010-1111----------11----"),
        //INST(&V::thumb32_ORR_reg,        "ORR (reg)",                "11101010010---------------------"),
        //INST(&V::thumb32_MVN_reg,        "MVN (reg)",                "11101010011-1111----------------"),
        //INST(&V::thumb32_ORN_reg,        "ORN (reg)",                "11101010011---------------------"),
        //INST(&V::thumb32_TEQ_reg,        "TEQ (reg)",                "111010101001--------1111--------"),
        //INST(&V::thumb32_EOR_reg,        "EOR (reg)",                "11101010100---------------------"),
        //INST(&V::thumb32_PKH,            "PKH",                      "11101010110---------------------"),
        //INST(&V::thumb32_CMN_reg,        "CMN (reg)",                "111010110001--------1111--------"),
        //INST(&V::thumb32_ADD_reg,        "ADD (reg)",                "11101011000---------------------"),
        //INST(&V::thumb32_ADC_reg,        "ADC (reg)",                "11101011010---------------------"),
        //INST(&V::thumb32_SBC_reg,        "SBC (reg)",                "11101011011---------------------"),
        //INST(&V::thumb32_CMP_reg,        "CMP (reg)",                "111010111011--------1111--------"),
        //INST(&V::thumb32_SUB_reg,        "SUB (reg)",                "11101011101---------------------"),
        //INST(&V::thumb32_RSB_reg,        "RSB (reg)",                "11101011110---------------------"),

        // Data Processing (Modified Immediate)
        INST(&V::thumb32_TST_imm,        "TST (imm)",                "11110i000001rrrr0kkk1111mmmmmmmm"),
        INST(&V::thumb32_AND_imm,        "AND (imm)",                "11110i00000Snnnn0kkkddddmmmmmmmm"),
        //INST(&V::thumb32_BIC_imm,        "BIC (imm)",                "11110-00001-----0---------------"),
        INST(&V::thumb32_MOV_imm,        "MOV (imm)",                "11110i00010S11110kkkddddmmmmmmmm"),
        INST(&V::thumb32_ORR_imm,        "ORR (imm)",                "11110i00010Snnnn0kkkddddmmmmmmmm"),
        //INST(&V::thumb32_MVN_imm,        "MVN (imm)",                "11110000011-11110---------------"),
        //INST(&V::thumb32_ORN_imm,        "ORN (imm)",                "11110-00011-----0---------------"),
        //INST(&V::thumb32_TEQ_imm,        "TEQ (imm)",                "11110-001001----0---1111--------"),
        //INST(&V::thumb32_EOR_imm,        "EOR (imm)",                "11110-00100-----0---------------"),
        //INST(&V::thumb32_CMN_imm,        "CMN (imm)",                "11110-010001----0---1111--------"),
        //INST(&V::thumb32_ADD_imm_1,      "ADD (imm)",                "11110-01000-----0---------------"),
        //INST(&V::thumb32_ADC_imm,        "ADC (imm)",                "11110-01010-----0---------------"),
        //INST(&V::thumb32_SBC_imm,        "SBC (imm)",                "11110-01011-----0---------------"),
        //INST(&V::thumb32_CMP_imm,        "CMP (imm)",                "11110-011011----0---1111--------"),
        //INST(&V::thumb32_SUB_imm_1,      "SUB (imm)",                "11110-01101-----0---------------"),
        //INST(&V::thumb32_RSB_imm,        "RSB (imm)",                "11110-01110-----0---------------"),

        // Data Processing (Plain Binary Immediate)
        //INST(&V::thumb32_ADR,            "ADR",                      "11110-10000011110---------------"),
        //INST(&V::thumb32_ADD_imm_2,      "ADD (imm)",                "11110-100000----0---------------"),
        //INST(&V::thumb32_MOVW_imm,       "MOVW (imm)",               "11110-100100----0---------------"),
        //INST(&V::thumb32_ADR,            "ADR",                      "11110-10101011110---------------"),
        //INST(&V::thumb32_SUB_imm_2,      "SUB (imm)",                "11110-101010----0---------------"),
        //INST(&V::thumb32_MOVT,           "MOVT",                     "11110-101100----0---------------"),
        //INST(&V::thumb32_SSAT,           "SSAT",                     "11110-110000----0---------------"),
        //INST(&V::thumb32_SSAT16,         "SSAT16",                   "11110-110010----0000----00------"),
        //INST(&V::thumb32_SSAT,           "SSAT",                     "11110-110010----0---------------"),
        //INST(&V::thumb32_SBFX,           "SBFX",                     "11110-110100----0---------------"),
        //INST(&V::thumb32_BFC,            "BFC",                      "11110-11011011110---------------"),
        //INST(&V::thumb32_BFI,            "BFI",                      "11110-110110----0---------------"),
        //INST(&V::thumb32_USAT,           "USAT",                     "11110-111000----0---------------"),
        //INST(&V::thumb32_USAT16,         "USAT16",                   "11110-111010----0000----00------"),
        //INST(&V::thumb32_USAT,           "USAT",                     "11110-111010----0---------------"),
        //INST(&V::thumb32_UBFX,           "UBFX",                     "11110-111100----0---------------"),

        // Branches and Miscellaneous Control
        //INST(&V::thumb32_MSR_banked,     "MSR (banked)",             "11110011100-----10-0------1-----"),
        //INST(&V::thumb32_MSR_reg_1,      "MSR (reg)",                "111100111001----10-0------0-----"),
        //INST(&V::thumb32_MSR_reg_2,      "MSR (reg)",                "111100111000----10-0--01--0-----"),
        //INST(&V::thumb32_MSR_reg_3,      "MSR (reg)",                "111100111000----10-0--1---0-----"),
        //INST(&V::thumb32_MSR_reg_4,      "MSR (reg)",                "111100111000----10-0--00--0-----"),

        //INST(&V::thumb32_NOP,            "NOP",                      "111100111010----10-0-00000000000"),
        //INST(&V::thumb32_YIELD,          "YIELD",                    "111100111010----10-0-00000000001"),
        //INST(&V::thumb32_WFE,            "WFE",                      "111100111010----10-0-00000000010"),
        //INST(&V::thumb32_WFI,            "WFI",                      "111100111010----10-0-00000000011"),
        //INST(&V::thumb32_SEV,            "SEV",                      "111100111010----10-0-00000000100"),
        //INST(&V::thumb32_SEVL,           "SEVL",                     "111100111010----10-0-00000000101"),
        //INST(&V::thumb32_DBG,            "DBG",                      "111100111010----10-0-0001111----"),
        //INST(&V::thumb32_CPS,            "CPS",                      "111100111010----10-0------------"),

        //INST(&V::thumb32_ENTERX,         "ENTERX",                   "111100111011----10-0----0001----"),
        //INST(&V::thumb32_LEAVEX,         "LEAVEX",                   "111100111011----10-0----0000----"),
        //INST(&V::thumb32_CLREX,          "CLREX",                    "111100111011----10-0----0010----"),
        //INST(&V::thumb32_DSB,            "DSB",                      "111100111011----10-0----0100----"),
        //INST(&V::thumb32_DMB,            "DMB",                      "111100111011----10-0----0101----"),
        //INST(&V::thumb32_ISB,            "ISB",                      "111100111011----10-0----0110----"),

        //INST(&V::thumb32_BXJ,            "BXJ",                      "111100111100----1000111100000000"),
        //INST(&V::thumb32_ERET,           "ERET",                     "11110011110111101000111100000000"),
        //INST(&V::thumb32_SUBS_pc_lr,     "SUBS PC, LR",              "111100111101111010001111--------"),

        //INST(&V::thumb32_MRS_banked,     "MRS (banked)",             "11110011111-----10-0------1-----"),
        //INST(&V::thumb32_MRS_reg_1,      "MRS (reg)",                "111100111111----10-0------0-----"),
        //INST(&V::thumb32_MRS_reg_2,      "MRS (reg)",                "111100111110----10-0------0-----"),
        //INST(&V::thumb32_HVC,            "HVC",                      "111101111110----1000------------"),
        //INST(&V::thumb32_SMC,            "SMC",                      "111101111111----1000000000000000"),
        //INST(&V::thumb32_UDF,            "UDF",                      "111101111111----1010------------"),

        //INST(&V::thumb32_BL,             "BL",                       "11110-----------11-1------------"),
        //INST(&V::thumb32_BLX,            "BLX",                      "11110-----------11-0------------"),
        //INST(&V::thumb32_B,              "B",                        "11110-----------10-1------------"),
        //INST(&V::thumb32_B_cond,         "B (cond)",                 "11110-----------10-0------------"),

        // Store Single Data Item
        //INST(&V::thumb32_STRB_imm_1,     "STRB (imm)",               "111110000000--------1--1--------"),
        //INST(&V::thumb32_STRB_imm_2,     "STRB (imm)",               "111110000000--------1100--------"),
        //INST(&V::thumb32_STRB_imm_3,     "STRB (imm)",               "111110001000--------------------"),
        //INST(&V::thumb32_STRBT,          "STRBT",                    "111110000000--------1110--------"),
        //INST(&V::thumb32_STRB,           "STRB (reg)",               "111110000000--------000000------"),
        //INST(&V::thumb32_STRH_imm_1,     "STRH (imm)",               "111110000010--------1--1--------"),
        //INST(&V::thumb32_STRH_imm_2,     "STRH (imm)",               "111110000010--------1100--------"),
        //INST(&V::thumb32_STRH_imm_3,     "STRH (imm)",               "111110001010--------------------"),
        //INST(&V::thumb32_STRHT,          "STRHT",                    "111110000010--------1110--------"),
        //INST(&V::thumb32_STRH,           "STRH (reg)",               "111110000010--------000000------"),
        //INST(&V::thumb32_STR_imm_1,      "STR (imm)",                "111110000100--------1--1--------"),
        //INST(&V::thumb32_STR_imm_2,      "STR (imm)",                "111110000100--------1100--------"),
        //INST(&V::thumb32_STR_imm_3,      "STR (imm)",                "111110001100--------------------"),
        //INST(&V::thumb32_STRT,           "STRT",                     "111110000100--------1110--------"),
        //INST(&V::thumb32_STR_reg,        "STR (reg)",                "111110000100--------000000------"),

        // Load Byte and Memory Hints
        //INST(&V::thumb32_PLD_lit,        "PLD (lit)",                "11111000-00111111111------------"),
        //INST(&V::thumb32_PLD_reg,        "PLD (reg)",                "111110000001----1111000000------"),
        //INST(&V::thumb32_PLD_imm8,       "PLD (imm8)",               "1111100000-1----11111100--------"),
        //INST(&V::thumb32_PLD_imm12,      "PLD (imm12)",              "111110001001----1111------------"),
        //INST(&V::thumb32_PLI_lit,        "PLI (lit)",                "11111001-00111111111------------"),
        //INST(&V::thumb32_PLI_reg,        "PLI (reg)",                "111110010001----1111000000------"),
        //INST(&V::thumb32_PLI_imm8,       "PLI (imm8)",               "111110010001----11111100--------"),
        //INST(&V::thumb32_PLI_imm12,      "PLI (imm12)",              "111110011001----1111------------"),
        //INST(&V::thumb32_LDRB_lit,       "LDRB (lit)",               "11111000-0011111----------------"),
        //INST(&V::thumb32_LDRB_reg,       "LDRB (reg)",               "111110000001--------000000------"),
        //INST(&V::thumb32_LDRBT,          "LDRBT",                    "111110000001--------1110--------"),
        //INST(&V::thumb32_LDRB_imm8,      "LDRB (imm8)",              "111110000001--------1-----------"),
        //INST(&V::thumb32_LDRB_imm12,     "LDRB (imm12)",             "111110001001--------------------"),
        //INST(&V::thumb32_LDRSB_lit,      "LDRSB (lit)",              "11111001-0011111----------------"),
        //INST(&V::thumb32_LDRSB_reg,      "LDRSB (reg)",              "111110010001--------000000------"),
        //INST(&V::thumb32_LDRSBT,         "LDRSBT",                   "111110010001--------1110--------"),
        //INST(&V::thumb32_LDRSB_imm8,     "LDRSB (imm8)",             "111110010001--------1-----------"),
        //INST(&V::thumb32_LDRSB_imm12,    "LDRSB (imm12)",            "111110011001--------------------"),

        // Load Halfword and Memory Hints
        //INST(&V::thumb32_LDRH_lit,       "LDRH (lit)",               "11111000-0111111----------------"),
        //INST(&V::thumb32_LDRH_reg,       "LDRH (reg)",               "111110000011--------000000------"),
        //INST(&V::thumb32_LDRHT,          "LDRHT",                    "111110000011--------1110--------"),
        //INST(&V::thumb32_LDRH_imm8,      "LDRH (imm8)",              "111110000011--------1-----------"),
        //INST(&V::thumb32_LDRH_imm12,     "LDRH (imm12)",             "111110001011--------------------"),
        //INST(&V::thumb32_LDRSH_lit,      "LDRSH (lit)",              "11111001-0111111----------------"),
        //INST(&V::thumb32_LDRSH_reg,      "LDRSH (reg)",              "111110010011--------000000------"),
        //INST(&V::thumb32_LDRSHT,         "LDRSHT",                   "111110010011--------1110--------"),
        //INST(&V::thumb32_LDRSH_imm8,     "LDRSH (imm8)",             "111110010011--------1-----------"),
        //INST(&V::thumb32_LDRSH_imm12,    "LDRSH (imm12)",            "111110011011--------------------"),
        //INST(&V::thumb32_NOP,            "NOP",                      "111110010011----1111000000------"),
        //INST(&V::thumb32_NOP,            "NOP",                      "111110010011----11111100--------"),
        //INST(&V::thumb32_NOP,            "NOP",                      "11111001-01111111111------------"),
        //INST(&V::thumb32_NOP,            "NOP",                      "111110011011----1111------------"),

        // Load Word
        //INST(&V::thumb32_LDR_lit,        "LDR (lit)",                "11111000-1011111----------------"),
        //INST(&V::thumb32_LDRT,           "LDRT",                     "111110000101--------1110--------"),
        //INST(&V::thumb32_LDR_reg,        "LDR (reg)",                "111110000101--------000000------"),
        //INST(&V::thumb32_LDR_imm8,       "LDR (imm8)",               "111110000101--------1-----------"),
        //INST(&V::thumb32_LDR_imm12,      "LDR (imm12)",              "111110001101--------------------"),

        // Undefined
        //INST(&V::thumb32_UDF,            "UDF",                      "1111100--111--------------------"),

        // Data Processing (register)
        //INST(&V::thumb32_LSL_reg,        "LSL (reg)",                "11111010000-----1111----0000----"),
        //INST(&V::thumb32_LSR_reg,        "LSR (reg)",                "11111010001-----1111----0000----"),
        //INST(&V::thumb32_ASR_reg,        "ASR (reg)",                "11111010010-----1111----0000----"),
        //INST(&V::thumb32_ROR_reg,        "ROR (reg)",                "11111010011-----1111----0000----"),
        //INST(&V::thumb32_SXTH,           "SXTH",                     "11111010000011111111----1-------"),
        //INST(&V::thumb32_SXTAH,          "SXTAH",                    "111110100000----1111----1-------"),
        //INST(&V::thumb32_UXTH,           "UXTH",                     "11111010000111111111----1-------"),
        //INST(&V::thumb32_UXTAH,          "UXTAH",                    "111110100001----1111----1-------"),
        //INST(&V::thumb32_SXTB16,         "SXTB16",                   "11111010001011111111----1-------"),
        //INST(&V::thumb32_SXTAB16,        "SXTAB16",                  "111110100010----1111----1-------"),
        //INST(&V::thumb32_UXTB16,         "UXTB16",                   "11111010001111111111----1-------"),
        //INST(&V::thumb32_UXTAB16,        "UXTAB16",                  "111110100011----1111----1-------"),
        //INST(&V::thumb32_SXTB,           "SXTB",                     "11111010010011111111----1-------"),
        //INST(&V::thumb32_SXTAB,          "SXTAB",                    "111110100100----1111----1-------"),
        //INST(&V::thumb32_UXTB,           "UXTB",                     "11111010010111111111----1-------"),
        //INST(&V::thumb32_UXTAB,          "UXTAB",                    "111110100101----1111----1-------"),

        // Parallel Addition and Subtraction (signed)
        //INST(&V::thumb32_SADD16,         "SADD16",                   "111110101001----1111----0000----"),
        //INST(&V::thumb32_SASX,           "SASX",                     "111110101010----1111----0000----"),
        //INST(&V::thumb32_SSAX,           "SSAX",                     "111110101110----1111----0000----"),
        //INST(&V::thumb32_SSUB16,         "SSUB16",                   "111110101101----1111----0000----"),
        //INST(&V::thumb32_SADD8,          "SADD8",                    "111110101000----1111----0000----"),
        //INST(&V::thumb32_SSUB8,          "SSUB8",                    "111110101100----1111----0000----"),
        //INST(&V::thumb32_QADD16,         "QADD16",                   "111110101001----1111----0001----"),
        //INST(&V::thumb32_QASX,           "QASX",                     "111110101010----1111----0001----"),
        //INST(&V::thumb32_QSAX,           "QSAX",                     "111110101110----1111----0001----"),
        //INST(&V::thumb32_QSUB16,         "QSUB16",                   "111110101101----1111----0001----"),
        //INST(&V::thumb32_QADD8,          "QADD8",                    "111110101000----1111----0001----"),
        //INST(&V::thumb32_QSUB8,          "QSUB8",                    "111110101100----1111----0001----"),
        //INST(&V::thumb32_SHADD16,        "SHADD16",                  "111110101001----1111----0010----"),
        //INST(&V::thumb32_SHASX,          "SHASX",                    "111110101010----1111----0010----"),
        //INST(&V::thumb32_SHSAX,          "SHSAX",                    "111110101110----1111----0010----"),
        //INST(&V::thumb32_SHSUB16,        "SHSUB16",                  "111110101101----1111----0010----"),
        //INST(&V::thumb32_SHADD8,         "SHADD8",                   "111110101000----1111----0010----"),
        //INST(&V::thumb32_SHSUB8,         "SHSUB8",                   "111110101100----1111----0010----"),

        // Parallel Addition and Subtraction (unsigned)
        //INST(&V::thumb32_UADD16,         "UADD16",                   "111110101001----1111----0100----"),
        //INST(&V::thumb32_UASX,           "UASX",                     "111110101010----1111----0100----"),
        //INST(&V::thumb32_USAX,           "USAX",                     "111110101110----1111----0100----"),
        //INST(&V::thumb32_USUB16,         "USUB16",                   "111110101101----1111----0100----"),
        //INST(&V::thumb32_UADD8,          "UADD8",                    "111110101000----1111----0100----"),
        //INST(&V::thumb32_USUB8,          "USUB8",                    "111110101100----1111----0100----"),
        //INST(&V::thumb32_UQADD16,        "UQADD16",                  "111110101001----1111----0101----"),
        //INST(&V::thumb32_UQASX,          "UQASX",                    "111110101010----1111----0101----"),
        //INST(&V::thumb32_UQSAX,          "UQSAX",                    "111110101110----1111----0101----"),
        //INST(&V::thumb32_UQSUB16,        "UQSUB16",                  "111110101101----1111----0101----"),
        //INST(&V::thumb32_UQADD8,         "UQADD8",                   "111110101000----1111----0101----"),
        //INST(&V::thumb32_UQSUB8,         "UQSUB8",                   "111110101100----1111----0101----"),
        //INST(&V::thumb32_UHADD16,        "UHADD16",                  "111110101001----1111----0110----"),
        //INST(&V::thumb32_UHASX,          "UHASX",                    "111110101010----1111----0110----"),
        //INST(&V::thumb32_UHSAX,          "UHSAX",                    "111110101110----1111----0110----"),
        //INST(&V::thumb32_UHSUB16,        "UHSUB16",                  "111110101101----1111----0110----"),
        //INST(&V::thumb32_UHADD8,         "UHADD8",                   "111110101000----1111----0110----"),
        //INST(&V::thumb32_UHSUB8,         "UHSUB8",                   "111110101100----1111----0110----"),

        // Miscellaneous Operations
        //INST(&V::thumb32_QADD,           "QADD",                     "111110101000----1111----1000----"),
        //INST(&V::thumb32_QDADD,          "QDADD",                    "111110101000----1111----1001----"),
        //INST(&V::thumb32_QSUB,           "QSUB",                     "111110101000----1111----1010----"),
        //INST(&V::thumb32_QDSUB,          "QDSUB",                    "111110101000----1111----1011----"),
        //INST(&V::thumb32_REV,            "REV",                      "111110101001----1111----1000----"),
        //INST(&V::thumb32_REV16,          "REV16",                    "111110101001----1111----1001----"),
        //INST(&V::thumb32_RBIT,           "RBIT",                     "111110101001----1111----1010----"),
        //INST(&V::thumb32_REVSH,          "REVSH",                    "111110101001----1111----1011----"),
        //INST(&V::thumb32_SEL,            "SEL",                      "111110101010----1111----1000----"),
        //INST(&V::thumb32_CLZ,            "CLZ",                      "111110101011----1111----1000----"),

        // Multiply, Multiply Accumulate, and Absolute Difference
        //INST(&V::thumb32_MUL,            "MUL",                      "111110110000----1111----0000----"),
        //INST(&V::thumb32_MLA,            "MLA",                      "111110110000------------0000----"),
        //INST(&V::thumb32_MLS,            "MLS",                      "111110110000------------0001----"),
        //INST(&V::thumb32_SMULXY,         "SMULXY",                   "111110110001----1111----00------"),
        //INST(&V::thumb32_SMLAXY,         "SMLAXY",                   "111110110001------------00------"),
        //INST(&V::thumb32_SMUAD,          "SMUAD",                    "111110110010----1111----000-----"),
        //INST(&V::thumb32_SMLAD,          "SMLAD",                    "111110110010------------000-----"),
        //INST(&V::thumb32_SMULWY,         "SMULWY",                   "111110110011----1111----000-----"),
        //INST(&V::thumb32_SMLAWY,         "SMLAWY",                   "111110110011------------000-----"),
        //INST(&V::thumb32_SMUSD,          "SMUSD",                    "111110110100----1111----000-----"),
        //INST(&V::thumb32_SMLSD,          "SMLSD",                    "111110110100------------000-----"),
        //INST(&V::thumb32_SMMUL,          "SMMUL",                    "111110110101----1111----000-----"),
        //INST(&V::thumb32_SMMLA,          "SMMLA",                    "111110110101------------000-----"),
        //INST(&V::thumb32_SMMLS,          "SMMLS",                    "111110110110------------000-----"),
        //INST(&V::thumb32_USAD8,          "USAD8",                    "111110110111----1111----0000----"),
        //INST(&V::thumb32_USADA8,         "USADA8",                   "111110110111------------0000----"),

        // Long Multiply, Long Multiply Accumulate, and Divide
        //INST(&V::thumb32_SMULL,          "SMULL",                    "111110111000------------0000----"),
        //INST(&V::thumb32_SDIV,           "SDIV",                     "111110111001------------1111----"),
        //INST(&V::thumb32_UMULL,          "UMULL",                    "111110111010------------0000----"),
        //INST(&V::thumb32_UDIV,           "UDIV",                     "111110111011------------1111----"),
        //INST(&V::thumb32_SMLAL,          "SMLAL",                    "111110111100------------0000----"),
        //INST(&V::thumb32_SMLALXY,        "SMLALXY",                  "111110111100------------10------"),
        //INST(&V::thumb32_SMLALD,         "SMLALD",                   "111110111100------------110-----"),
        //INST(&V::thumb32_SMLSLD,         "SMLSLD",                   "111110111101------------110-----"),
        //INST(&V::thumb32_UMLAL,          "UMLAL",                    "111110111110------------0000----"),
        //INST(&V::thumb32_UMAAL,          "UMAAL",                    "111110111110------------0110----"),

        // Coprocessor
        //INST(&V::thumb32_MCRR2,          "MCRR2",                    "111111000100--------------------"),
        //INST(&V::thumb32_MCRR,           "MCRR",                     "111011000100--------------------"),
        //INST(&V::thumb32_STC2,           "STC2",                     "1111110----0--------------------"),
        //INST(&V::thumb32_STC,            "STC",                      "1110110----0--------------------"),
        //INST(&V::thumb32_MRRC2,          "MRRC2",                    "111111000101--------------------"),
        //INST(&V::thumb32_MRRC,           "MRRC",                     "111011000101--------------------"),
        //INST(&V::thumb32_LDC2_lit,       "LDC2 (lit)",               "1111110----11111----------------"),
        //INST(&V::thumb32_LDC_lit,        "LDC (lit)",                "1110110----11111----------------"),
        //INST(&V::thumb32_LDC2_imm,       "LDC2 (imm)",               "1111110----1--------------------"),
        //INST(&V::thumb32_LDC_imm,        "LDC (imm)",                "1110110----1--------------------"),
        //INST(&V::thumb32_CDP2,           "CDP2",                     "11111110-------------------0----"),
        //INST(&V::thumb32_CDP,            "CDP",                      "11101110-------------------0----"),
        //INST(&V::thumb32_MCR2,           "MCR2",                     "11111110---0---------------1----"),
        //INST(&V::thumb32_MCR,            "MCR",                      "11101110---0---------------1----"),
        //INST(&V::thumb32_MRC2,           "MRC2",                     "11111110---1---------------1----"),
        //INST(&V::thumb32_MRC,            "MRC",                      "11101110---1---------------1----"),

        // Branch instructions
        INST(&V::thumb32_BL_imm,         "BL (imm)",                 "11110vvvvvvvvvvv11111vvvvvvvvvvv"), // v4T
        INST(&V::thumb32_BLX_imm,        "BLX (imm)",                "11110vvvvvvvvvvv11101vvvvvvvvvvv"), // v5T

        // Misc instructions
        INST(&V::thumb32_UDF,            "UDF",                      "111101111111----1010------------"), // v6T2

#undef INST

    };

    const auto matches_instruction = [instruction](const auto& matcher){ return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const Thumb32Matcher<V>>>(*iter) : std::nullopt;
}

} // namespace Dynarmic::A32
