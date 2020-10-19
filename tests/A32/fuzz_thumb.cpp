/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <functional>
#include <tuple>

#include <catch.hpp>

#include <dynarmic/A32/a32.h>

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/A32/disassembler/disassembler.h"
#include "frontend/A32/FPSCR.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/PSR.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/ir/basic_block.h"
#include "fuzz_util.h"
#include "ir_opt/passes.h"
#include "rand_int.h"
#include "testenv.h"
#include "unicorn_emu/a32_unicorn.h"

using namespace Dynarmic;

static A32::UserConfig GetUserConfig(ThumbTestEnv* testenv) {
    A32::UserConfig user_config;
    user_config.optimizations &= ~OptimizationFlag::FastDispatch;
    user_config.callbacks = testenv;
    return user_config;
}

using WriteRecords = std::map<u32, u8>;

using ThumbInstruction = boost::variant<u32, u16>;

template <typename InstructionType>
struct ThumbInstGen final {
public:
        
    ThumbInstGen(const char* format, std::function<bool(InstructionType)> is_valid = [](InstructionType){ return true; }) : is_valid(is_valid) {
        const size_t bitsize = Common::BitSize<InstructionType>();
        REQUIRE(strlen(format) == bitsize);
        for (size_t i = 0; i < bitsize; i++) {
            const InstructionType bit = 1 << ((bitsize-1) - i);
            switch (format[i]) {
            case '0':
                mask |= bit;
                break;
            case '1':
                bits |= bit;
                mask |= bit;
                break;
            default:
                // Do nothing
                break;
            }
        }
    }
    ThumbInstruction Generate(u32 pc, bool is_last_inst) const {
        InstructionType inst;
        do {
            const u32 initial = Common::Replicate(0xF, 4);
            const InstructionType random = RandInt<InstructionType>(0, static_cast<InstructionType>(initial));
            inst = bits | (random & ~mask);
        } while (!is_valid(inst) || !ShouldTestA32Inst(inst, pc, true, is_last_inst));

        ASSERT((inst & mask) == bits);

        return ThumbInstruction(inst);
    }
private:
    InstructionType bits = 0;
    InstructionType mask = 0;
    std::function<bool(InstructionType)> is_valid;
};

using Thumb16InstGen = ThumbInstGen<u16>;
using Thumb32InstGen = ThumbInstGen<u32>;

// This validation function prevents PCs in
// thumb32 instruction's register fields
template<bool mask_n, bool mask_t, bool mask_d, bool mask_r>
static bool T32PCMask(u32 inst) {
    if constexpr (mask_n) {
        if (Common::Bits<16, 19>(inst) == 0b1111) {
            return false;
        }
    }
    if constexpr (mask_t) { 
        if (Common::Bits<12, 15>(inst) == 0b1111) {
            return false;
        }
    }
    if constexpr (mask_d) {
        if (Common::Bits<8, 11>(inst) == 0b1111) {
            return false;
        }
    }
    if constexpr (mask_r) {
        if (Common::Bits<0, 3>(inst) == 0b1111) {
            return false;
        }
    }
    return true;
}

static bool DoesBehaviorMatch(const A32Unicorn<ThumbTestEnv>& uni, const A32::Jit& jit,
                              const WriteRecords& interp_write_records, const WriteRecords& jit_write_records) {
    const auto interp_regs = uni.GetRegisters();
    const auto jit_regs = jit.Regs();

    return std::equal(interp_regs.begin(), interp_regs.end(), jit_regs.begin(), jit_regs.end()) &&
           uni.GetCpsr() == jit.Cpsr() &&
           interp_write_records == jit_write_records;
}

static void RunInstance(size_t run_number, ThumbTestEnv& test_env, A32Unicorn<ThumbTestEnv>& uni, A32::Jit& jit, const ThumbTestEnv::RegisterArray& initial_regs,
                        size_t instruction_count, size_t instructions_to_execute_count) {
    uni.ClearPageCache();
    jit.ClearCache();

    // Setup initial state

    uni.SetCpsr(0x000001F0);
    uni.SetRegisters(initial_regs);
    jit.SetCpsr(0x000001F0);
    jit.Regs() = initial_regs;

    // Run interpreter
    test_env.modified_memory.clear();
    test_env.ticks_left = instructions_to_execute_count;
    uni.SetPC(uni.GetPC() | 1);
    uni.Run();
    const bool uni_code_memory_modified = test_env.code_mem_modified_by_guest;
    const auto interp_write_records = test_env.modified_memory;

    // Run jit
    test_env.code_mem_modified_by_guest = false;
    test_env.modified_memory.clear();
    test_env.ticks_left = instructions_to_execute_count;
    jit.Run();
    const bool jit_code_memory_modified = test_env.code_mem_modified_by_guest;
    const auto jit_write_records = test_env.modified_memory;
    test_env.code_mem_modified_by_guest = false;

    REQUIRE(uni_code_memory_modified == jit_code_memory_modified);
    if (uni_code_memory_modified) {
        return;
    }

    // Compare
    if (!DoesBehaviorMatch(uni, jit, interp_write_records, jit_write_records)) {
        printf("Failed at execution number %zu\n", run_number);

        printf("\nInstruction Listing: \n");
        u32 pc = 0;
        for (size_t i = 0; i < instruction_count; i++) {
            const u16 code = test_env.code_mem[pc/2];
            if ((code & 0xF000) != 0xF000 && (code & 0xF800) != 0xE800) {
                printf("%04x %s\n", code, A32::DisassembleThumb16(code).c_str());
                pc += 2;
            } else {
                printf("%04x%04x thumb32\n", test_env.code_mem[pc/2], test_env.code_mem[pc/2+1]);
                pc += 4;
            }
        }

        printf("\nInitial Register Listing: \n");
        for (size_t i = 0; i < initial_regs.size(); i++) {
            printf("%4zu: %08x\n", i, initial_regs[i]);
        }

        printf("\nFinal Register Listing: \n");
        printf("      unicorn   jit\n");
        const auto uni_registers = uni.GetRegisters();
        for (size_t i = 0; i < uni_registers.size(); i++) {
            printf("%4zu: %08x %08x %s\n", i, uni_registers[i], jit.Regs()[i], uni_registers[i] != jit.Regs()[i] ? "*" : "");
        }
        printf("CPSR: %08x %08x %s\n", uni.GetCpsr(), jit.Cpsr(), uni.GetCpsr() != jit.Cpsr() ? "*" : "");

        printf("\nUnicorn Write Records:\n");
        for (const auto& record : interp_write_records) {
            printf("[%08x] = %02x\n", record.first, record.second);
        }

        printf("\nJIT Write Records:\n");
        for (const auto& record : jit_write_records) {
            printf("[%08x] = %02x\n", record.first, record.second);
        }

        A32::PSR cpsr;
        cpsr.T(true);

        size_t num_insts = 0;
        while (num_insts < instructions_to_execute_count) {
            A32::LocationDescriptor descriptor = {u32(num_insts * 4), cpsr, A32::FPSCR{}};
            IR::Block ir_block = A32::Translate(descriptor, [&test_env](u32 vaddr) { return test_env.MemoryReadCode(vaddr); }, {});
            Optimization::A32GetSetElimination(ir_block);
            Optimization::DeadCodeElimination(ir_block);
            Optimization::A32ConstantMemoryReads(ir_block, &test_env);
            Optimization::ConstantPropagation(ir_block);
            Optimization::DeadCodeElimination(ir_block);
            Optimization::VerificationPass(ir_block);
            printf("\n\nIR:\n%s", IR::DumpBlock(ir_block).c_str());
            printf("\n\nx86_64:\n%s", jit.Disassemble().c_str());
            num_insts += ir_block.CycleCount();
        }

#ifdef _MSC_VER
        __debugbreak();
#endif
        FAIL();
    }
}

void FuzzJitThumb(const size_t instruction_count, const size_t instructions_to_execute_count, const size_t run_count, const std::function<ThumbInstruction(u32, bool)> instruction_generator) {
    ThumbTestEnv test_env;
    // Prepare memory.
    test_env.code_mem.resize(instruction_count * 2 + 1);

    // Prepare test subjects
    A32Unicorn uni{test_env};
    A32::Jit jit{GetUserConfig(&test_env)};

    for (size_t run_number = 0; run_number < run_count; run_number++) {
        ThumbTestEnv::RegisterArray initial_regs;
        std::generate_n(initial_regs.begin(), initial_regs.size() - 1, []{ return RandInt<u32>(0, 0xFFFFFFFF); });
        initial_regs[15] = 0;
        
        u32 pc = 0;
        for (size_t i = 0; i < instruction_count; i++) {
            const auto insn = instruction_generator(pc, i == instruction_count - 1);
            if (boost::get<u16>(&insn)) {
                const u16 code = boost::get<u16>(insn);
                test_env.code_mem[pc / 2] = static_cast<u16>(code);
                pc += 2;
            } else {
                const u32 code = boost::get<u32>(insn);
                test_env.code_mem[pc/2] = code >> 16;
                pc += 2;
                test_env.code_mem[pc/2] = static_cast<u16>(code);
                pc += 2;
            }
        }
        test_env.code_mem[pc/2] = 0xE7FE; // b #+0
        RunInstance(run_number, test_env, uni, jit, initial_regs, instruction_count, instructions_to_execute_count);
    }
}

TEST_CASE("Fuzz Thumb instructions set 1", "[JitX64][Thumb]") {
    const std::array instructions = {
        Thumb16InstGen("00000xxxxxxxxxxx"), // LSL <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("00001xxxxxxxxxxx"), // LSR <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("00010xxxxxxxxxxx"), // ASR <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("000110oxxxxxxxxx"), // ADD/SUB_reg
        Thumb16InstGen("000111oxxxxxxxxx"), // ADD/SUB_imm
        Thumb16InstGen("001ooxxxxxxxxxxx"), // ADD/SUB/CMP/MOV_imm
        Thumb16InstGen("010000ooooxxxxxx"), // Data Processing
        Thumb16InstGen("010001000hxxxxxx"), // ADD (high registers)
        Thumb16InstGen("0100010101xxxxxx",  // CMP (high registers)
                     [](u16 inst){ return Common::Bits<3, 5>(inst) != 0b111; }), // R15 is UNPREDICTABLE
        Thumb16InstGen("0100010110xxxxxx",  // CMP (high registers)
                     [](u16 inst){ return Common::Bits<0, 2>(inst) != 0b111; }), // R15 is UNPREDICTABLE
        Thumb16InstGen("010001100hxxxxxx"), // MOV (high registers)
        Thumb16InstGen("10110000oxxxxxxx"), // Adjust stack pointer
        Thumb16InstGen("10110010ooxxxxxx"), // SXT/UXT
        Thumb16InstGen("1011101000xxxxxx"), // REV
        Thumb16InstGen("1011101001xxxxxx"), // REV16
        Thumb16InstGen("1011101011xxxxxx"), // REVSH
        Thumb16InstGen("01001xxxxxxxxxxx"), // LDR Rd, [PC, #]
        Thumb16InstGen("0101oooxxxxxxxxx"), // LDR/STR Rd, [Rn, Rm]
        Thumb16InstGen("011xxxxxxxxxxxxx"), // LDR(B)/STR(B) Rd, [Rn, #]
        Thumb16InstGen("1000xxxxxxxxxxxx"), // LDRH/STRH Rd, [Rn, #offset]
        Thumb16InstGen("1001xxxxxxxxxxxx"), // LDR/STR Rd, [SP, #]
        Thumb16InstGen("1011010xxxxxxxxx",  // PUSH
                     [](u16 inst){ return Common::Bits<0, 7>(inst) != 0; }), // Empty reg_list is UNPREDICTABLE
        Thumb16InstGen("10111100xxxxxxxx",  // POP (P = 0)
                     [](u16 inst){ return Common::Bits<0, 7>(inst) != 0; }), // Empty reg_list is UNPREDICTABLE
        Thumb16InstGen("1100xxxxxxxxxxxx", // STMIA/LDMIA
                     [](u16 inst) {
                         // Ensure that the architecturally undefined case of
                         // the base register being within the list isn't hit.
                         const u32 rn = Common::Bits<8, 10>(inst);
                         return (inst & (1U << rn)) == 0 && Common::Bits<0, 7>(inst) != 0;
                     }),
        // TODO: We should properly test against swapped
        //       endianness cases, however Unicorn doesn't
        //       expose the intended endianness of a load/store
        //       operation to memory through its hooks.
#if 0
        Thumb16InstGen("101101100101x000"), // SETEND
#endif
    };

    const auto instruction_select = [&](u32 pc, bool is_last_inst) -> ThumbInstruction {
        size_t inst_index = RandInt<size_t>(0, instructions.size() - 1);

        return instructions[inst_index].Generate(pc, is_last_inst);
    };

    SECTION("single instructions") {
        FuzzJitThumb(1, 2, 10000, instruction_select);
    }

    SECTION("short blocks") {
        FuzzJitThumb(5, 6, 3000, instruction_select);
    }

    // TODO: Test longer blocks when Unicorn can consistently
    //       run these without going into an infinite loop.
#if 0
    SECTION("long blocks") {
        FuzzJitThumb16(1024, 1025, 1000, instruction_select);
    }
#endif
}

TEST_CASE("Fuzz Thumb IT blocks", "[JitX64][Thumb]") {
    std::vector instructions = {
        Thumb16InstGen("00000xxxxxxxxxxx"), // LSL <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("00001xxxxxxxxxxx"), // LSR <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("00010xxxxxxxxxxx"), // ASR <Rd>, <Rm>, #<imm5>
        Thumb16InstGen("000110oxxxxxxxxx"), // ADD/SUB_reg
        Thumb16InstGen("000111oxxxxxxxxx"), // ADD/SUB_imm
        Thumb16InstGen("001ooxxxxxxxxxxx"), // ADD/SUB/CMP/MOV_imm
        Thumb16InstGen("010000ooooxxxxxx"), // Data Processing
        Thumb16InstGen("010001000hxxxxxx"), // ADD (high registers)
        Thumb16InstGen("0100010101xxxxxx",  // CMP (high registers)
                     [](u16 inst){ return Common::Bits<3, 5>(inst) != 0b111; }), // R15 is UNPREDICTABLE
        Thumb16InstGen("0100010110xxxxxx",  // CMP (high registers)
                     [](u16 inst){ return Common::Bits<0, 2>(inst) != 0b111; }), // R15 is UNPREDICTABLE
        Thumb16InstGen("010001100hxxxxxx") // MOV (high registers)6 inst)
    };

    const auto cond_instruction_select = [&](u32 pc, bool is_last_inst) -> ThumbInstruction {
        if (pc == 4 || pc == 14) {
            // IT
            return Thumb16InstGen("10111111cccc1mmm",
              [](u16 inst){ return Common::Bits<4, 7>(inst) != 0b1111 && Common::Bits<4, 7>(inst) != 0b1110; }).Generate(pc, is_last_inst);
        }
        const size_t inst_index = RandInt<size_t>(0, instructions.size() - 1);

        return instructions[inst_index].Generate(pc, is_last_inst);
    };

    SECTION("short blocks") {
        FuzzJitThumb(20, 21, 3000, cond_instruction_select);
    }
}

TEST_CASE("Fuzz Thumb2 instructions set 1", "[JitX64][Thumb2]") {
    const std::array instructions = {
        Thumb32InstGen("11110x00010011110xxx00xxxxxxxxxx"), // MOV (imm)
        Thumb32InstGen("11110x00001xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // BIC (imm)
        Thumb32InstGen("11110x00000xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // AND (imm)
        Thumb32InstGen("11110x00010xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // ORR (imm)
        Thumb32InstGen("11110x000001nnnn0xxx1111xxxxxxxx"), // TST (imm)
        Thumb32InstGen("11110x00011x11110xxxddddxxxxxxxx", T32PCMask<0, 0, 1, 0>), // MVN (imm)
        Thumb32InstGen("11110x00011xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // ORN (imm)
        Thumb32InstGen("11110x001001nnnn0xxx1111xxxxxxxx", T32PCMask<1, 0, 0, 0>), // TEQ (imm)
        Thumb32InstGen("11110x00100xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // EOR (imm)
        Thumb32InstGen("11110x010001nnnn0xxx1111xxxxxxxx", T32PCMask<1, 0, 0, 0>), // CMN (imm)
        Thumb32InstGen("11110x01000xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // ADD (imm)
        Thumb32InstGen("11110x01010xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>),// ADC (imm)
        Thumb32InstGen("11110x01011xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // SBC (imm)
        Thumb32InstGen("11110x011011nnnn0xxx1111xxxxxxxx", T32PCMask<1, 0, 0, 0>), // CMP (imm)
        Thumb32InstGen("11110x01101xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // SUB (imm)
        Thumb32InstGen("11110x01110xnnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // RSB (imm)
        Thumb32InstGen("111010100001nnnn0xxx1111xxxxrrrr", T32PCMask<1, 0, 0, 1>), // TST (reg) 
        Thumb32InstGen("11101010000xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // AND (reg)
        Thumb32InstGen("11101010001xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // BIC (reg)
        Thumb32InstGen("11101010010x11110xxxddddxxxxrrrr", T32PCMask<0, 0, 1, 1>), // LSL (imm) / LSR (imm) / ASR(imm) / RRX / ROR (imm)
        Thumb32InstGen("11101010010xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // ORR (reg)
        Thumb32InstGen("11101010011x11110xxxddddxxxxrrrr", T32PCMask<0, 0, 1, 1>), // MVN (reg)
        Thumb32InstGen("11101010011xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // ORN (reg)
        Thumb32InstGen("11101010100xnnnn0xxx1111xxxxrrrr", T32PCMask<1, 0, 0, 1>), // TEQ (reg) 
        Thumb32InstGen("11101010100xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // EOR (reg)
        Thumb32InstGen("111010101100nnnn0xxxddddxxx0rrrr", T32PCMask<1, 0, 1, 1>), // PKH
        Thumb32InstGen("111010110001nnnn0xxx1111xxxxrrrr", T32PCMask<1, 0, 0, 1>), // CMN (reg) 
        Thumb32InstGen("11101011000xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // ADD (reg) 
        Thumb32InstGen("11101011010xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // ADC (reg) 
        Thumb32InstGen("11101011011xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // SBC (reg) 
        Thumb32InstGen("111010111011nnnn0xxx1111xxxxrrrr", T32PCMask<1, 0, 0, 1>), // CMP (reg) 
        Thumb32InstGen("11101011101xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // SUB (reg) 
        Thumb32InstGen("11101011110xnnnn0xxxddddxxxxrrrr", T32PCMask<1, 0, 1, 1>), // RSB (reg) 
        Thumb32InstGen("11110x10000011110xxxddddxxxxxxxx", T32PCMask<0, 0, 1, 0>), // ADR after  
        Thumb32InstGen("11110x100000nnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // ADDW
        Thumb32InstGen("11110x100100xxxx0xxxddddxxxxxxxx", T32PCMask<0, 0, 1, 0>), // MOVW
        Thumb32InstGen("11110x10101011110xxxddddxxxxxxxx", T32PCMask<0, 0, 1, 0>), // ADR before
        Thumb32InstGen("11110x101010nnnn0xxxddddxxxxxxxx", T32PCMask<1, 0, 1, 0>), // SUBW
        Thumb32InstGen("11110x101100xxxx0xxxddddxxxxxxxx", T32PCMask<0, 0, 1, 0>), // MOVT
        Thumb32InstGen("111100110010nnnn0000dddd0000mmmm", T32PCMask<1, 0, 1, 0>), // SSAT16
        Thumb32InstGen("111110001000nnnnttttxxxxxxxxxxxx", T32PCMask<1, 1, 0, 0>), // STRB (imm) 2
        Thumb32InstGen("111110000000nnnntttt000000xxrrrr", T32PCMask<1, 1, 0, 1>), // STRB
        Thumb32InstGen("1111001100s0nnnn0kkkddddii0mmmmm", T32PCMask<1, 0, 1, 0>), // SSAT
        Thumb32InstGen("111100110100nnnn0kkkddddii0mmmmm", T32PCMask<1, 0, 1, 0>), // SBFX
        Thumb32InstGen("11110011011011110kkkddddii0mmmmm", T32PCMask<0, 0, 1, 0>), // BFC
        Thumb32InstGen("111110000000nnnntttt1puwmmmmmmmm", T32PCMask<1, 1, 0, 0>), // STRB (imm) 1
        Thumb32InstGen("1110100xx0W0nnnn0r0rrrrrrrrrrrrr", // STMIA / STMDB
                     [](u32 inst) {
            // Ensure that the undefined case of
            // the base register being within the list isn't hit when W is true.
            // In the spec, when LowestBit(reg_list) == r_n, it's defined, 
            // but unicorn sees it as an invalid instruction.
            const u32 op = Common::Bits<23, 24>(inst);
            const u32 rn = Common::Bits<16, 19>(inst);
            const bool W = Common::Bit<21>(inst);
            const u32 reg_list = Common::Bits<0, 15>(inst);
            // These are not STMIA or STMDB
            if (op == 0b11 || op == 0b00) {
                return false;
            }
            return rn != 15 && !(W && Common::Bit(rn, reg_list)) && Common::BitCount(reg_list) >= 2;
        }),
        Thumb32InstGen("1110100xx0W1nnnnrrrrrrrrrrrrrrrr", // LDMIA / LDMDB
                     [](u32 inst) {
            const u32 op = Common::Bits<23, 24>(inst);
            const u32 rn = Common::Bits<16, 19>(inst);
            const bool w = Common::Bit<21>(inst);
            const u32 reg_list = Common::Bits<0, 15>(inst);
            // These are not LDMIA or LDMDB
            if (op == 0b11 || op == 0b00) {
                return false;
            }
            return rn != 15 &&
                !(w && Common::Bit(rn, reg_list)) && // Base register should not be in list when w is true
                !Common::Bit(15, reg_list) && // Prevent jump 
                Common::BitCount(reg_list) >= 2;
        }),
    };

    const auto instruction_select = [&](u32 pc, bool is_last_inst) -> ThumbInstruction {
        const size_t inst_index = RandInt<size_t>(0, instructions.size() - 1);

        return instructions[inst_index].Generate(pc, is_last_inst);
    };

    SECTION("single instructions") {
        FuzzJitThumb(1, 2, 10000, instruction_select);
    }

    SECTION("short blocks") {
        FuzzJitThumb(5, 6, 3000, instruction_select);
    }
}


TEST_CASE("Fuzz Thumb instructions set 2 (affects PC)", "[JitX64][Thumb]") {
    const std::array instructions = {
        // TODO: We currently can't test BX/BLX as we have
        //       no way of preventing the unpredictable
        //       condition from occurring with the current interface.
        //       (bits zero and one within the specified register
        //       must not be address<1:0> == '10'.
#if 0
        ThumbInstGen("01000111xmmmm000",  // BLX/BX
                     [](u16 inst){
                         const u32 Rm = Common::Bits<3, 6>(inst);
                         return Rm != 15;
                     }),
#endif
        Thumb16InstGen("1010oxxxxxxxxxxx"), // add to pc/sp
        Thumb16InstGen("11100xxxxxxxxxxx"), // B
        Thumb16InstGen("01000100h0xxxxxx"), // ADD (high registers)
        Thumb16InstGen("01000110h0xxxxxx"), // MOV (high registers)
        Thumb16InstGen("1101ccccxxxxxxxx",  // B<cond>
                     [](u16 inst){
                         const u32 c = Common::Bits<9, 12>(inst);
                         return c < 0b1110; // Don't want SWI or undefined instructions.
                     }),
        Thumb16InstGen("1011o0i1iiiiinnn"), // CBZ/CBNZ
        Thumb16InstGen("10110110011x0xxx"), // CPS

        // TODO: We currently have no control over the generated
        //       values when creating new pages, so we can't
        //       reliably test this yet.
#if 0
        Thumb16InstGen("10111101xxxxxxxx"), // POP (R = 1)
#endif
    };

    const auto instruction_select = [&](u32 pc, bool is_last) -> ThumbInstruction {
        size_t inst_index = RandInt<size_t>(0, instructions.size() - 1);

        return instructions[inst_index].Generate(pc, is_last);
    };

    FuzzJitThumb(1, 1, 10000, instruction_select);
}

TEST_CASE("Verify fix for off by one error in MemoryRead32 worked", "[Thumb]") {
    ThumbTestEnv test_env;

    // Prepare test subjects
    A32Unicorn<ThumbTestEnv> uni{test_env};
    A32::Jit jit{GetUserConfig(&test_env)};

    constexpr ThumbTestEnv::RegisterArray initial_regs {
        0xe90ecd70,
        0x3e3b73c3,
        0x571616f9,
        0x0b1ef45a,
        0xb3a829f2,
        0x915a7a6a,
        0x579c38f4,
        0xd9ffe391,
        0x55b6682b,
        0x458d8f37,
        0x8f3eb3dc,
        0xe18c0e7d,
        0x6752657a,
        0x00001766,
        0xdbbf23e3,
        0x00000000,
    };

    test_env.code_mem = {
        0x40B8, // lsls r0, r7, #0
        0x01CA, // lsls r2, r1, #7
        0x83A1, // strh r1, [r4, #28]
        0x708A, // strb r2, [r1, #2]
        0xBCC4, // pop {r2, r6, r7}
        0xE7FE, // b +#0
    };

    RunInstance(1, test_env, uni, jit, initial_regs, 5, 5);
}
