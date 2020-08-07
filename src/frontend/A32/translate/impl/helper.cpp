/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/helper.h"

namespace Dynarmic::A32::Helper {

bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
	auto address = start_address;
	for (size_t i = 0; i <= 14; i++) {
		if (Common::Bit(i, list)) {
			ir.SetRegister(static_cast<Reg>(i), ir.ReadMemory32(address));
			address = ir.Add(address, ir.Imm32(4));
		}
	}
	if (W && !Common::Bit(RegNumber(n), list)) {
		ir.SetRegister(n, writeback_address);
	}
	if (Common::Bit<15>(list)) {
		ir.LoadWritePC(ir.ReadMemory32(address));
		if (n == Reg::R13)
			ir.SetTerm(IR::Term::PopRSBHint{});
		else
			ir.SetTerm(IR::Term::FastDispatchHint{});
		return false;
	}
	return true;
}

bool STMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
	auto address = start_address;
	for (size_t i = 0; i <= 14; i++) {
		if (Common::Bit(i, list)) {
			ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
			address = ir.Add(address, ir.Imm32(4));
		}
	}
	if (W) {
		ir.SetRegister(n, writeback_address);
	}
	if (Common::Bit<15>(list)) {
		ir.WriteMemory32(address, ir.Imm32(ir.PC()));
	}
	return true;
}

} // namespace Dynarmic::A32