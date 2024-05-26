/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "backend/x64/block_of_code.h"
#include "backend/x64/emit_x64.h"
#include "common/crypto/sm4.h"
#include "frontend/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

void EmitX64::EmitSM4AccessSubstitutionBox(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.CallFunction(&Common::Crypto::SM4::AccessSubstitutionBox);
}

}  // namespace Dynarmic::Backend::X64
