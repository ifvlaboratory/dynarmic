/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "backend/x64/block_of_code.h"

#include <array>
#include <cstring>

#include <xbyak.h>

#include "backend/x64/a32_jitstate.h"
#include "backend/x64/abi.h"
#include "backend/x64/perf_map.h"
#include "common/assert.h"

#ifdef _WIN32
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif

namespace Dynarmic::Backend::X64 {

#ifdef _WIN32
const Xbyak::Reg64 BlockOfCode::ABI_RETURN = Xbyak::util::rax;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM1 = Xbyak::util::rcx;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM2 = Xbyak::util::rdx;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM3 = Xbyak::util::r8;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM4 = Xbyak::util::r9;
const std::array<Xbyak::Reg64, 4> BlockOfCode::ABI_PARAMS = {BlockOfCode::ABI_PARAM1, BlockOfCode::ABI_PARAM2, BlockOfCode::ABI_PARAM3, BlockOfCode::ABI_PARAM4};
#else
const Xbyak::Reg64 BlockOfCode::ABI_RETURN = Xbyak::util::rax;
const Xbyak::Reg64 BlockOfCode::ABI_RETURN2 = Xbyak::util::rdx;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM1 = Xbyak::util::rdi;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM2 = Xbyak::util::rsi;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM3 = Xbyak::util::rdx;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM4 = Xbyak::util::rcx;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM5 = Xbyak::util::r8;
const Xbyak::Reg64 BlockOfCode::ABI_PARAM6 = Xbyak::util::r9;
const std::array<Xbyak::Reg64, 6> BlockOfCode::ABI_PARAMS = {BlockOfCode::ABI_PARAM1, BlockOfCode::ABI_PARAM2, BlockOfCode::ABI_PARAM3, BlockOfCode::ABI_PARAM4, BlockOfCode::ABI_PARAM5, BlockOfCode::ABI_PARAM6};
#endif

namespace {

constexpr size_t TOTAL_CODE_SIZE = 128 * 1024 * 1024;
constexpr size_t FAR_CODE_OFFSET = 100 * 1024 * 1024;
constexpr size_t CONSTANT_POOL_SIZE = 2 * 1024 * 1024;

class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    bool useProtect() const override { return false; }
#endif
};

// This is threadsafe as Xbyak::Allocator does not contain any state; it is a pure interface.
CustomXbyakAllocator s_allocator;

#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
void ProtectMemory(const void* base, size_t size, bool is_executable) {
#    ifdef _WIN32
    DWORD oldProtect = 0;
    VirtualProtect(const_cast<void*>(base), size, is_executable ? PAGE_EXECUTE_READ : PAGE_READWRITE, &oldProtect);
#    else
    static const size_t pageSize = sysconf(_SC_PAGESIZE);
    const size_t iaddr = reinterpret_cast<size_t>(base);
    const size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    const int mode = is_executable ? (PROT_READ | PROT_EXEC) : (PROT_READ | PROT_WRITE);
    mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode);
#    endif
}
#endif

}  // anonymous namespace

BlockOfCode::BlockOfCode(RunCodeCallbacks cb, JitStateInfo jsi, std::function<void(BlockOfCode&)> rcp)
        : Xbyak::CodeGenerator(TOTAL_CODE_SIZE, nullptr, &s_allocator)
        , cb(std::move(cb))
        , jsi(jsi)
        , constant_pool(*this, CONSTANT_POOL_SIZE) {
    EnableWriting();
    GenRunCode(rcp);
}

void BlockOfCode::PreludeComplete() {
    prelude_complete = true;
    near_code_begin = getCurr();
    far_code_begin = getCurr() + FAR_CODE_OFFSET;
    ClearCache();
    DisableWriting();
}

void BlockOfCode::EnableWriting() {
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    ProtectMemory(getCode(), maxSize_, false);
#endif
}

void BlockOfCode::DisableWriting() {
#ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
    ProtectMemory(getCode(), maxSize_, true);
#endif
}

void BlockOfCode::ClearCache() {
    ASSERT(prelude_complete);
    in_far_code = false;
    near_code_ptr = near_code_begin;
    far_code_ptr = far_code_begin;
    SetCodePtr(near_code_begin);
}

size_t BlockOfCode::SpaceRemaining() const {
    ASSERT(prelude_complete);
    // This function provides an underestimate of near-code-size but that's okay.
    // (Why? The maximum size of near code should be measured from near_code_begin, not top_.)
    // These are offsets from Xbyak::CodeArray::top_.
    std::size_t far_code_offset, near_code_offset;
    if (in_far_code) {
        near_code_offset = static_cast<const u8*>(near_code_ptr) - getCode();
        far_code_offset = getCurr() - getCode();
    } else {
        near_code_offset = getCurr() - getCode();
        far_code_offset = static_cast<const u8*>(far_code_ptr) - getCode();
    }
    if (far_code_offset > TOTAL_CODE_SIZE)
        return 0;
    if (near_code_offset > FAR_CODE_OFFSET)
        return 0;
    return std::min(TOTAL_CODE_SIZE - far_code_offset, FAR_CODE_OFFSET - near_code_offset);
}

void BlockOfCode::RunCode(void* jit_state, CodePtr code_ptr) const {
    run_code(jit_state, code_ptr);
}

void BlockOfCode::StepCode(void* jit_state, CodePtr code_ptr) const {
    step_code(jit_state, code_ptr);
}

void BlockOfCode::ReturnFromRunCode(bool mxcsr_already_exited) {
    size_t index = 0;
    if (mxcsr_already_exited)
        index |= MXCSR_ALREADY_EXITED;
    jmp(return_from_run_code[index]);
}

void BlockOfCode::ForceReturnFromRunCode(bool mxcsr_already_exited) {
    size_t index = FORCE_RETURN;
    if (mxcsr_already_exited)
        index |= MXCSR_ALREADY_EXITED;
    jmp(return_from_run_code[index]);
}

void BlockOfCode::GenRunCode(std::function<void(BlockOfCode&)> rcp) {
    align();
    run_code = getCurr<RunCodeFuncType>();

    // This serves two purposes:
    // 1. It saves all the registers we as a callee need to save.
    // 2. It aligns the stack so that the code the JIT emits can assume
    //    that the stack is appropriately aligned for CALLs.
    ABI_PushCalleeSaveRegistersAndAdjustStack(*this);

    mov(r15, ABI_PARAM1);
    mov(rbx, ABI_PARAM2);  // save temporarily in non-volatile register

    cb.GetTicksRemaining->EmitCall(*this);
    mov(qword[r15 + jsi.offsetof_cycles_to_run], ABI_RETURN);
    mov(qword[r15 + jsi.offsetof_cycles_remaining], ABI_RETURN);

    rcp(*this);

    SwitchMxcsrOnEntry();
    jmp(rbx);

    align();
    step_code = getCurr<RunCodeFuncType>();

    ABI_PushCalleeSaveRegistersAndAdjustStack(*this);

    mov(r15, ABI_PARAM1);

    mov(qword[r15 + jsi.offsetof_cycles_to_run], 1);
    mov(qword[r15 + jsi.offsetof_cycles_remaining], 1);

    rcp(*this);

    SwitchMxcsrOnEntry();
    jmp(ABI_PARAM2);

    align();

    // Dispatcher loop

    Xbyak::Label return_to_caller, return_to_caller_mxcsr_already_exited;

    align();
    return_from_run_code[0] = getCurr<const void*>();

    cmp(qword[r15 + jsi.offsetof_cycles_remaining], 0);
    jng(return_to_caller);
    cb.LookupBlock->EmitCall(*this);
    jmp(ABI_RETURN);

    align();
    return_from_run_code[MXCSR_ALREADY_EXITED] = getCurr<const void*>();

    cmp(qword[r15 + jsi.offsetof_cycles_remaining], 0);
    jng(return_to_caller_mxcsr_already_exited);
    SwitchMxcsrOnEntry();
    cb.LookupBlock->EmitCall(*this);
    jmp(ABI_RETURN);

    align();
    return_from_run_code[FORCE_RETURN] = getCurr<const void*>();
    L(return_to_caller);

    SwitchMxcsrOnExit();
    // fallthrough

    return_from_run_code[MXCSR_ALREADY_EXITED | FORCE_RETURN] = getCurr<const void*>();
    L(return_to_caller_mxcsr_already_exited);

    cb.AddTicks->EmitCall(*this, [this](RegList param) {
        mov(param[0], qword[r15 + jsi.offsetof_cycles_to_run]);
        sub(param[0], qword[r15 + jsi.offsetof_cycles_remaining]);
    });

    ABI_PopCalleeSaveRegistersAndAdjustStack(*this);
    ret();

    PerfMapRegister(run_code, getCurr(), "dynarmic_dispatcher");
}

void BlockOfCode::SwitchMxcsrOnEntry() {
    stmxcsr(dword[r15 + jsi.offsetof_save_host_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
}

void BlockOfCode::SwitchMxcsrOnExit() {
    stmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_save_host_MXCSR]);
}

void BlockOfCode::EnterStandardASIMD() {
    stmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_asimd_MXCSR]);
}

void BlockOfCode::LeaveStandardASIMD() {
    stmxcsr(dword[r15 + jsi.offsetof_asimd_MXCSR]);
    ldmxcsr(dword[r15 + jsi.offsetof_guest_MXCSR]);
}

void BlockOfCode::UpdateTicks() {
    cb.AddTicks->EmitCall(*this, [this](RegList param) {
        mov(param[0], qword[r15 + jsi.offsetof_cycles_to_run]);
        sub(param[0], qword[r15 + jsi.offsetof_cycles_remaining]);
    });

    cb.GetTicksRemaining->EmitCall(*this);
    mov(qword[r15 + jsi.offsetof_cycles_to_run], ABI_RETURN);
    mov(qword[r15 + jsi.offsetof_cycles_remaining], ABI_RETURN);
}

void BlockOfCode::LookupBlock() {
    cb.LookupBlock->EmitCall(*this);
}

Xbyak::Address BlockOfCode::MConst(const Xbyak::AddressFrame& frame, u64 lower, u64 upper) {
    return constant_pool.GetConstant(frame, lower, upper);
}

void BlockOfCode::SwitchToFarCode() {
    ASSERT(prelude_complete);
    ASSERT(!in_far_code);
    in_far_code = true;
    near_code_ptr = getCurr();
    SetCodePtr(far_code_ptr);

    ASSERT_MSG(near_code_ptr < far_code_begin, "Near code has overwritten far code!");
}

void BlockOfCode::SwitchToNearCode() {
    ASSERT(prelude_complete);
    ASSERT(in_far_code);
    in_far_code = false;
    far_code_ptr = getCurr();
    SetCodePtr(near_code_ptr);
}

CodePtr BlockOfCode::GetCodeBegin() const {
    return near_code_begin;
}

size_t BlockOfCode::GetTotalCodeSize() const {
    return maxSize_;
}

void* BlockOfCode::AllocateFromCodeSpace(size_t alloc_size) {
    if (size_ + alloc_size >= maxSize_) {
        throw Xbyak::Error(Xbyak::ERR_CODE_IS_TOO_BIG);
    }

    void* ret = getCurr<void*>();
    size_ += alloc_size;
    memset(ret, 0, alloc_size);
    return ret;
}

void BlockOfCode::SetCodePtr(CodePtr code_ptr) {
    // The "size" defines where top_, the insertion point, is.
    size_t required_size = reinterpret_cast<const u8*>(code_ptr) - getCode();
    setSize(required_size);
}

void BlockOfCode::EnsurePatchLocationSize(CodePtr begin, size_t size) {
    size_t current_size = getCurr<const u8*>() - reinterpret_cast<const u8*>(begin);
    ASSERT(current_size <= size);
    nop(size - current_size);
}

bool BlockOfCode::HasSSSE3() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tSSSE3);
}

bool BlockOfCode::HasSSE41() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tSSE41);
}

bool BlockOfCode::HasSSE42() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tSSE42);
}

bool BlockOfCode::HasPCLMULQDQ() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tPCLMULQDQ);
}

bool BlockOfCode::HasAVX() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tAVX);
}

bool BlockOfCode::HasF16C() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tF16C);
}

bool BlockOfCode::HasAESNI() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tAESNI);
}

bool BlockOfCode::HasLZCNT() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tLZCNT);
}

bool BlockOfCode::HasBMI1() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tBMI1);
}

bool BlockOfCode::HasBMI2() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tBMI2);
}

bool BlockOfCode::HasFastBMI2() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tBMI2) && !DoesCpuSupport(Xbyak::util::Cpu::tAMD);
}

bool BlockOfCode::HasFMA() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tFMA);
}

bool BlockOfCode::HasAVX2() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tAVX2);
}

bool BlockOfCode::HasAVX512_Skylake() const {
    // The feature set formerly known as AVX3.2. (Introduced with Skylake.)
    return DoesCpuSupport(Xbyak::util::Cpu::tAVX512F)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512CD)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512BW)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512DQ)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512VL);
}

bool BlockOfCode::HasAVX512_Icelake() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tAVX512F)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512CD)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512BW)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512DQ)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512VL)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512_VPOPCNTDQ)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512_VNNI)
        && DoesCpuSupport(Xbyak::util::Cpu::tGFNI)
        && DoesCpuSupport(Xbyak::util::Cpu::tVAES)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512_VBMI2)
        && DoesCpuSupport(Xbyak::util::Cpu::tAVX512_BITALG)
        && DoesCpuSupport(Xbyak::util::Cpu::tVPCLMULQDQ);
}

bool BlockOfCode::HasAVX512_BITALG() const {
    return DoesCpuSupport(Xbyak::util::Cpu::tAVX512_BITALG);
}

bool BlockOfCode::DoesCpuSupport([[maybe_unused]] Xbyak::util::Cpu::Type type) const {
#ifdef DYNARMIC_ENABLE_CPU_FEATURE_DETECTION
    return cpu_info.has(type);
#else
    return false;
#endif
}

}  // namespace Dynarmic::Backend::X64
