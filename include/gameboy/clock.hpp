#pragma once

#include "types.hpp"

namespace GbcEmulator {

class Clock {
public:
    Clock() = default;
    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

    constexpr TCycleCount get() const noexcept { return clock_; }
    constexpr void add(TCycleCount amount) noexcept { clock_ += amount; }

    constexpr bool isDoubleSpeed() const noexcept { return isDoubleSpeed_; }
    constexpr void setDoubleSpeed(bool is_double_speed) noexcept {
        isDoubleSpeed_ = is_double_speed;
    }

    constexpr void reset() {
        clock_ = 0;
        isDoubleSpeed_ = false;
    }

private:
    TCycleCount clock_ = 0;
    bool isDoubleSpeed_ = false;
};

}  // namespace GbcEmulator
