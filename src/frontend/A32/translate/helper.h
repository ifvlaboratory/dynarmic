/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/types.h"

namespace Dynarmic::A32::Helper {

bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address);
bool STMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address);
void PKHHelper(A32::IREmitter& ir, bool tb, Reg d, IR::U32 n, IR::U32 shifted);
void SSAT16Helper(A32::IREmitter& ir, Reg d, Reg n, size_t saturate_to);
void SBFXHelper(A32::IREmitter& ir, Reg d, Reg n, u32 lsbit, u32 width_num);
void BFCHelper(A32::IREmitter& ir, Reg d, u32 lsbit, u32 msbit);

IR::U32 GetAddress(A32::IREmitter& ir, bool P, bool U, bool W, Reg n, IR::U32 offset);
IR::U32 Pack2x16To1x32(A32::IREmitter& ir, IR::U32 lo, IR::U32 hi);
IR::U16 MostSignificantHalf(A32::IREmitter& ir, IR::U32 value);
}  // namespace Dynarmic::A32::Helper
