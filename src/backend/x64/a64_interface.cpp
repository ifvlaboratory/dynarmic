/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstring>
#include <memory>

#include <boost/icl/interval_set.hpp>
#include <dynarmic/A64/a64.h>

#include "backend/x64/a64_emit_x64.h"
#include "backend/x64/a64_jitstate.h"
#include "backend/x64/block_of_code.h"
#include "backend/x64/devirtualize.h"
#include "backend/x64/jitstate_info.h"
#include "common/assert.h"
#include "common/llvm_disassemble.h"
#include "common/scope_exit.h"
#include "frontend/A64/translate/translate.h"
#include "frontend/ir/basic_block.h"
#include "ir_opt/passes.h"

namespace Dynarmic::A64 {

using namespace Backend::X64;

static RunCodeCallbacks GenRunCodeCallbacks(A64::UserCallbacks* cb, CodePtr (*LookupBlock)(void* lookup_block_arg), void* arg) {
    return RunCodeCallbacks{
        std::make_unique<ArgCallback>(LookupBlock, reinterpret_cast<u64>(arg)),
        std::make_unique<ArgCallback>(Devirtualize<&A64::UserCallbacks::AddTicks>(cb)),
        std::make_unique<ArgCallback>(Devirtualize<&A64::UserCallbacks::GetTicksRemaining>(cb)),
    };
}

static std::function<void(BlockOfCode&)> GenRCP(const A64::UserConfig& conf) {
    return [conf](BlockOfCode& code) {
        if (conf.page_table) {
            code.mov(code.r14, Common::BitCast<u64>(conf.page_table));
        }
    };
}

struct Jit::Impl final {
public:
    Impl(Jit* jit, UserConfig conf)
            : conf(conf)
            , block_of_code(GenRunCodeCallbacks(conf.callbacks, &GetCurrentBlockThunk, this), JitStateInfo{jit_state}, GenRCP(conf))
            , emitter(block_of_code, conf, jit) {
        ASSERT(conf.page_table_address_space_bits >= 12 && conf.page_table_address_space_bits <= 64);
    }

    ~Impl() = default;

    void Run() {
        ASSERT(!is_executing);
        is_executing = true;
        SCOPE_EXIT {
            this->is_executing = false;
        };
        jit_state.halt_requested = false;

        // TODO: Check code alignment

        const CodePtr current_code_ptr = [this] {
            // RSB optimization
            const u32 new_rsb_ptr = (jit_state.rsb_ptr - 1) & A64JitState::RSBPtrMask;
            if (jit_state.GetUniqueHash() == jit_state.rsb_location_descriptors[new_rsb_ptr]) {
                jit_state.rsb_ptr = new_rsb_ptr;
                return reinterpret_cast<CodePtr>(jit_state.rsb_codeptrs[new_rsb_ptr]);
            }

            return GetCurrentBlock();
        }();
        block_of_code.RunCode(&jit_state, current_code_ptr);

        PerformRequestedCacheInvalidation();
    }

    void Step() {
        ASSERT(!is_executing);
        is_executing = true;
        SCOPE_EXIT {
            this->is_executing = false;
        };
        jit_state.halt_requested = true;

        block_of_code.StepCode(&jit_state, GetCurrentSingleStep());

        PerformRequestedCacheInvalidation();
    }

    void ExceptionalExit() {
        if (!conf.wall_clock_cntpct) {
            const s64 ticks = jit_state.cycles_to_run - jit_state.cycles_remaining;
            conf.callbacks->AddTicks(ticks);
        }
        PerformRequestedCacheInvalidation();
        is_executing = false;
    }

    void ChangeProcessorID(size_t value) {
        conf.processor_id = value;
        emitter.ChangeProcessorID(value);
    }

    void ClearCache() {
        invalidate_entire_cache = true;
        RequestCacheInvalidation();
    }

    void InvalidateCacheRange(u64 start_address, size_t length) {
        const auto end_address = static_cast<u64>(start_address + length - 1);
        const auto range = boost::icl::discrete_interval<u64>::closed(start_address, end_address);
        invalid_cache_ranges.add(range);
        RequestCacheInvalidation();
    }

    void Reset() {
        ASSERT(!is_executing);
        jit_state = {};
    }

    void HaltExecution() {
        jit_state.halt_requested = true;
    }

    u64 GetSP() const {
        return jit_state.sp;
    }

    void SetSP(u64 value) {
        jit_state.sp = value;
    }

    u64 GetPC() const {
        return jit_state.pc;
    }

    void SetPC(u64 value) {
        jit_state.pc = value;
    }

    u64 GetRegister(size_t index) const {
        if (index == 31)
            return GetSP();
        return jit_state.reg.at(index);
    }

    void SetRegister(size_t index, u64 value) {
        if (index == 31)
            return SetSP(value);
        jit_state.reg.at(index) = value;
    }

    std::array<u64, 31> GetRegisters() const {
        return jit_state.reg;
    }

    void SetRegisters(const std::array<u64, 31>& value) {
        jit_state.reg = value;
    }

    Vector GetVector(size_t index) const {
        return {jit_state.vec.at(index * 2), jit_state.vec.at(index * 2 + 1)};
    }

    void SetVector(size_t index, Vector value) {
        jit_state.vec.at(index * 2) = value[0];
        jit_state.vec.at(index * 2 + 1) = value[1];
    }

    std::array<Vector, 32> GetVectors() const {
        std::array<Vector, 32> ret;
        static_assert(sizeof(ret) == sizeof(jit_state.vec));
        std::memcpy(ret.data(), jit_state.vec.data(), sizeof(jit_state.vec));
        return ret;
    }

    void SetVectors(const std::array<Vector, 32>& value) {
        static_assert(sizeof(value) == sizeof(jit_state.vec));
        std::memcpy(jit_state.vec.data(), value.data(), sizeof(jit_state.vec));
    }

    u32 GetFpcr() const {
        return jit_state.GetFpcr();
    }

    void SetFpcr(u32 value) {
        jit_state.SetFpcr(value);
    }

    u32 GetFpsr() const {
        return jit_state.GetFpsr();
    }

    void SetFpsr(u32 value) {
        jit_state.SetFpsr(value);
    }

    u32 GetPstate() const {
        return jit_state.GetPstate();
    }

    void SetPstate(u32 value) {
        jit_state.SetPstate(value);
    }

    void ClearExclusiveState() {
        jit_state.exclusive_state = 0;
    }

    bool IsExecuting() const {
        return is_executing;
    }

    std::string Disassemble() const {
        return Common::DisassembleX64(block_of_code.GetCodeBegin(), block_of_code.getCurr());
    }

private:
    static CodePtr GetCurrentBlockThunk(void* thisptr) {
        Jit::Impl* this_ = static_cast<Jit::Impl*>(thisptr);
        return this_->GetCurrentBlock();
    }

    IR::LocationDescriptor GetCurrentLocation() const {
        return IR::LocationDescriptor{jit_state.GetUniqueHash()};
    }

    CodePtr GetCurrentBlock() {
        return GetBlock(GetCurrentLocation());
    }

    CodePtr GetCurrentSingleStep() {
        return GetBlock(A64::LocationDescriptor{GetCurrentLocation()}.SetSingleStepping(true));
    }

    CodePtr GetBlock(IR::LocationDescriptor current_location) {
        if (auto block = emitter.GetBasicBlock(current_location))
            return block->entrypoint;

        constexpr size_t MINIMUM_REMAINING_CODESIZE = 1 * 1024 * 1024;
        if (block_of_code.SpaceRemaining() < MINIMUM_REMAINING_CODESIZE) {
            // Immediately evacuate cache
            invalidate_entire_cache = true;
            PerformRequestedCacheInvalidation();
        }

        // JIT Compile
        const auto get_code = [this](u64 vaddr) { return conf.callbacks->MemoryReadCode(vaddr); };
        IR::Block ir_block = A64::Translate(A64::LocationDescriptor{current_location}, get_code,
                                            {conf.define_unpredictable_behaviour, conf.wall_clock_cntpct});
        Optimization::A64CallbackConfigPass(ir_block, conf);
        if (conf.HasOptimization(OptimizationFlag::GetSetElimination)) {
            Optimization::A64GetSetElimination(ir_block);
            Optimization::DeadCodeElimination(ir_block);
        }
        if (conf.HasOptimization(OptimizationFlag::ConstProp)) {
            Optimization::ConstantPropagation(ir_block);
            Optimization::DeadCodeElimination(ir_block);
        }
        if (conf.HasOptimization(OptimizationFlag::MiscIROpt)) {
            Optimization::A64MergeInterpretBlocksPass(ir_block, conf.callbacks);
        }
        Optimization::VerificationPass(ir_block);
        return emitter.Emit(ir_block).entrypoint;
    }

    void RequestCacheInvalidation() {
        if (is_executing) {
            //            jit_state.halt_requested = true;
            return;
        }

        PerformRequestedCacheInvalidation();
    }

    void PerformRequestedCacheInvalidation() {
        if (!invalidate_entire_cache && invalid_cache_ranges.empty()) {
            return;
        }

        jit_state.ResetRSB();
        if (invalidate_entire_cache) {
            block_of_code.ClearCache();
            emitter.ClearCache();
        } else {
            emitter.InvalidateCacheRanges(invalid_cache_ranges);
        }
        invalid_cache_ranges.clear();
        invalidate_entire_cache = false;
    }

    bool is_executing = false;

    UserConfig conf;
    A64JitState jit_state;
    BlockOfCode block_of_code;
    A64EmitX64 emitter;

    bool invalidate_entire_cache = false;
    boost::icl::interval_set<u64> invalid_cache_ranges;
};

Jit::Jit(UserConfig conf)
        : impl(std::make_unique<Jit::Impl>(this, conf)) {}

Jit::~Jit() = default;

void Jit::Run() {
    impl->Run();
}

void Jit::Step() {
    impl->Step();
}

void Jit::ClearCache() {
    impl->ClearCache();
}

void Jit::InvalidateCacheRange(u64 start_address, size_t length) {
    impl->InvalidateCacheRange(start_address, length);
}

void Jit::Reset() {
    impl->Reset();
}

void Jit::HaltExecution() {
    impl->HaltExecution();
}

void Jit::ExceptionalExit() {
    impl->ExceptionalExit();
}

void Jit::ChangeProcessorID(size_t new_processor) {
    impl->ChangeProcessorID(new_processor);
}

u64 Jit::GetSP() const {
    return impl->GetSP();
}

void Jit::SetSP(u64 value) {
    impl->SetSP(value);
}

u64 Jit::GetPC() const {
    return impl->GetPC();
}

void Jit::SetPC(u64 value) {
    impl->SetPC(value);
}

u64 Jit::GetRegister(size_t index) const {
    return impl->GetRegister(index);
}

void Jit::SetRegister(size_t index, u64 value) {
    impl->SetRegister(index, value);
}

std::array<u64, 31> Jit::GetRegisters() const {
    return impl->GetRegisters();
}

void Jit::SetRegisters(const std::array<u64, 31>& value) {
    impl->SetRegisters(value);
}

Vector Jit::GetVector(size_t index) const {
    return impl->GetVector(index);
}

void Jit::SetVector(size_t index, Vector value) {
    impl->SetVector(index, value);
}

std::array<Vector, 32> Jit::GetVectors() const {
    return impl->GetVectors();
}

void Jit::SetVectors(const std::array<Vector, 32>& value) {
    impl->SetVectors(value);
}

u32 Jit::GetFpcr() const {
    return impl->GetFpcr();
}

void Jit::SetFpcr(u32 value) {
    impl->SetFpcr(value);
}

u32 Jit::GetFpsr() const {
    return impl->GetFpsr();
}

void Jit::SetFpsr(u32 value) {
    impl->SetFpsr(value);
}

u32 Jit::GetPstate() const {
    return impl->GetPstate();
}

void Jit::SetPstate(u32 value) {
    impl->SetPstate(value);
}

void Jit::ClearExclusiveState() {
    impl->ClearExclusiveState();
}

bool Jit::IsExecuting() const {
    return impl->IsExecuting();
}

std::string Jit::Disassemble() const {
    return impl->Disassemble();
}

}  // namespace Dynarmic::A64
