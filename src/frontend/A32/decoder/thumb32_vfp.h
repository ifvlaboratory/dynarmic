/* This file is part of the dynarmic project.
 * Copyright (c) 2032 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"
#include "frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template<typename Visitor>
using ThumbVFPMatcher = Decoder::Matcher<Visitor, u32>;

template<typename V>
std::optional<std::reference_wrapper<const ThumbVFPMatcher<V>>> DecodeThumbVFP(u32 instruction) {
    static const std::vector<ThumbVFPMatcher<V>> table = {

#define INST(fn, name, bitstring) Decoder::detail::detail<ThumbVFPMatcher<V>>::GetMatcher(&V::fn, name, bitstring),
#include "thumb32_vfp.inc"
#undef INST

    };

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const ThumbVFPMatcher<V>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A32
