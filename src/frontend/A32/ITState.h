/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/common_types.h"
#include "common/bit_util.h"
#include "frontend/ir/cond.h"

namespace Dynarmic::A32 {

class ITState final {
public:
    ITState() = default;
    explicit ITState(u8 data) : value(data) {}

    ITState& operator=(u8 data) {
        value = data;
        return *this;
    }

    IR::Cond Cond() const {
        auto new_cond = Common::Bits<4, 7>(value);
        new_cond = Common::ModifyBit<0>(new_cond, Common::Bit<4>(Flags()));
        return static_cast<IR::Cond>(new_cond);
    }

    // IT<5:7>
    IR::Cond Base() const {
        return static_cast<IR::Cond>(Common::Bits<5, 7>(value));
    }

    void Base(u8 base) {
        value = Common::ModifyBits<5, 7>(value, Common::Bits<0, 2>(base));
    }

    // IT<0:4>
    u8 Flags() const {
        return Common::Bits<0, 4>(value);
    }
    void Flags(u8 flags) {
        value = Common::ModifyBits<0, 4>(value, flags);
    }

    bool IsInITBlock() const {
        return Flags() != 0b00000;
    }
    bool IsLastInITBlock() const {
        return Common::Bits<1,4>(Flags()) == 0b1000;
    }

    ITState Advance() const {
        ITState result{*this};
        result.Flags(result.Flags() << 1);
        if (result.Flags() == 0b10000) {
            return ITState{0};
        }
        return result;
    }

    u8 Value() const {
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

} // namespace Dynarmic::A32
