/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/ir/cond.h"

namespace Dynarmic::A32 {

class ITState final {
public:
    ITState() = default;
    explicit ITState(u8 data)
            : value(data) {}

    ITState& operator=(u8 data) {
        value = data;
        return *this;
    }

    [[nodiscard]] IR::Cond Cond() const {
        return static_cast<IR::Cond>(Common::Bits<4, 7>(value));
    }
    void Cond(IR::Cond cond) {
        value = Common::ModifyBits<4, 7>(value, static_cast<u8>(cond));
    }

    [[nodiscard]] u8 Mask() const {
        return Common::Bits<0, 3>(value);
    }
    void Mask(u8 mask) {
        value = Common::ModifyBits<0, 3>(value, mask);
    }

    [[nodiscard]] bool IsInITBlock() const {
        return Mask() != 0b0000;
    }

    [[nodiscard]] bool IsLastInITBlock() const {
        return Mask() == 0b1000;
    }

    [[nodiscard]] ITState Advance() const {
        ITState result{*this};
        const u8 mask = result.Mask() << 1U;
        result.value = Common::ModifyBits<0, 4>(result.value, mask);
        if (result.Mask() == 0b0000) {
            return ITState{0};
        }
        return result;
    }

    [[nodiscard]] u8 Value() const {
        return value;
    }

private:
    u8 value;
};

inline bool operator==(ITState lhs, ITState rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(ITState lhs, ITState rhs) {
    return !operator==(lhs, rhs);
}

}  // namespace Dynarmic::A32
