/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/assert.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/types.h"

namespace Dynarmic::A32::Helper {

bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address);
bool STMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address);
void PKHHelper(A32::IREmitter& ir, bool tb, Reg d, IR::U32 n, IR::U32 shifted);

} // namespace Dynarmic::A32
