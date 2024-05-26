/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"
#include "frontend/decoder/matcher.h"

namespace Dynarmic::A64 {

template<typename Visitor>
using Matcher = Decoder::Matcher<Visitor, u32>;

template<typename Visitor>
using DecodeTable = std::array<std::vector<Matcher<Visitor>>, 0x1000>;

namespace detail {
inline size_t ToFastLookupIndex(u32 instruction) {
    return ((instruction >> 10U) & 0x00FU) | ((instruction >> 18U) & 0xFF0U);
}
}  // namespace detail

template<typename Visitor>
DecodeTable<Visitor> GetDecodeTable() {
    std::vector<Matcher<Visitor>> list = {
#define INST(fn, name, bitstring) Decoder::detail::detail<Matcher<Visitor>>::GetMatcher(&Visitor::fn, name, bitstring),
#include "a64.inc"
#undef INST
    };

    std::stable_sort(list.begin(), list.end(), [](const auto& matcher1, const auto& matcher2) {
        // If a matcher has more bits in its mask it is more specific, so it should come first.
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    // Exceptions to the above rule of thumb.
    const std::set<std::string> comes_first{
        "MOVI, MVNI, ORR, BIC (vector, immediate)",
        "FMOV (vector, immediate)",
        "Unallocated SIMD modified immediate",
    };

    std::stable_partition(list.begin(), list.end(), [&](const auto& matcher) {
        return comes_first.count(matcher.GetName()) > 0;
    });

    DecodeTable<Visitor> table{};
    for (size_t i = 0; i < table.size(); ++i) {
        for (auto matcher : list) {
            const auto expect = detail::ToFastLookupIndex(matcher.GetExpected());
            const auto mask = detail::ToFastLookupIndex(matcher.GetMask());
            if ((i & mask) == expect) {
                table[i].push_back(matcher);
            }
        }
    }
    return table;
}

template<typename Visitor>
std::optional<std::reference_wrapper<const Matcher<Visitor>>> Decode(u32 instruction) {
    static const auto table = GetDecodeTable<Visitor>();

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    const auto& subtable = table[detail::ToFastLookupIndex(instruction)];
    auto iter = std::find_if(subtable.begin(), subtable.end(), matches_instruction);
    return iter != subtable.end() ? std::optional<std::reference_wrapper<const Matcher<Visitor>>>(*iter) : std::nullopt;
}

}  // namespace Dynarmic::A64
