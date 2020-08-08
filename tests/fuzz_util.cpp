/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstring>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <dynarmic/A32/a32.h>

#include "common/assert.h"
#include "common/fp/fpcr.h"
#include "common/fp/rounding_mode.h"
#include "frontend/A32/disassembler/disassembler.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/ir/basic_block.h"
#include "frontend/ir/location_descriptor.h"
#include "frontend/ir/opcodes.h"
#include "fuzz_util.h"
#include "rand_int.h"

using namespace Dynarmic;

std::ostream& operator<<(std::ostream& o, Vector vec) {
    return o << fmt::format("{:016x}'{:016x}", vec[1], vec[0]);
}

Vector RandomVector() {
    return {RandInt<u64>(0, ~u64(0)), RandInt<u64>(0, ~u64(0))};
}

u32 RandomFpcr() {
    FP::FPCR fpcr;
    fpcr.AHP(RandInt(0, 1) == 0);
    fpcr.DN(RandInt(0, 1) == 0);
    fpcr.FZ(RandInt(0, 1) == 0);
    fpcr.RMode(static_cast<FP::RoundingMode>(RandInt(0, 3)));
    fpcr.FZ16(RandInt(0, 1) == 0);
    return fpcr.Value();
}

InstructionGenerator::InstructionGenerator(const char* format){
    ASSERT(std::strlen(format) == 32);

    for (int i = 0; i < 32; i++) {
        const u32 bit = 1u << (31 - i);
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

u32 InstructionGenerator::Generate() const {
    const u32 random = RandInt<u32>(0, 0xFFFFFFFF);
    return bits | (random & ~mask);
}

bool ShouldTestA32Inst(u32 instruction, u32 pc, bool thumb, bool is_last_inst) {
    A32::LocationDescriptor location{ pc, {}, {} };
    location = location.SetTFlag(thumb);
    IR::Block block{ location };
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);

    if (!should_continue && !is_last_inst) {
        return false;
    }

    if (auto terminal = block.GetTerminal(); boost::get<IR::Term::Interpret>(&terminal)) {
        return false;
    }

    for (const auto& ir_inst : block) {
        switch (ir_inst.GetOpcode()) {
        case IR::Opcode::A32ExceptionRaised:
        case IR::Opcode::A32CallSupervisor:
        case IR::Opcode::A32CoprocInternalOperation:
        case IR::Opcode::A32CoprocSendOneWord:
        case IR::Opcode::A32CoprocSendTwoWords:
        case IR::Opcode::A32CoprocGetOneWord:
        case IR::Opcode::A32CoprocGetTwoWords:
        case IR::Opcode::A32CoprocLoadWords:
        case IR::Opcode::A32CoprocStoreWords:
            return false;
            // Currently unimplemented in Unicorn
        case IR::Opcode::FPVectorRecipEstimate16:
        case IR::Opcode::FPVectorRSqrtEstimate16:
        case IR::Opcode::VectorPolynomialMultiplyLong64:
            return false;
        default:
            continue;
        }
    }

    return true;
}
