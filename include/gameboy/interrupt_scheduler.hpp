#pragma once

#include <algorithm>
#include <array>
#include <cassert>

#include "interrupt_type.hpp"
#include "types.hpp"

namespace GbcEmulator {

class Clock;
class GameBoy;

class InterruptScheduler {
public:
    InterruptScheduler(Clock& clock, GameBoy& gb) : clock_{clock}, gb_{gb} { reset(); }
    InterruptScheduler(const InterruptScheduler&) = delete;
    InterruptScheduler& operator=(const InterruptScheduler&) = delete;

    inline void reschedule(InterruptType type, TCycleCount cycle) {
        interrupt_times_[static_cast<std::underlying_type<InterruptType>::type>(type)] = cycle;
        recalculateClosestInterrupt();
    }

    inline Byte getIf() {
        catchUp();
        return if_;
    }

    inline void setIf(Byte value) {
        catchUp();
        if_ = value | 0xE0;
    }

    constexpr const std::array<TCycleCount, 4>& getAllInts() const { return interrupt_times_; }
    
    void catchUp();
    void reset();

private:
    inline static constexpr std::array<Byte, 4> interrupt_masks = {0x01, 0x02, 0x04, 0x08};

    static constexpr std::underlying_type<InterruptType>::type toUnderlying(InterruptType type) {
        return static_cast<std::underlying_type<InterruptType>::type>(type);
    }

    void recalculateClosestInterrupt();

    Clock& clock_;
    GameBoy& gb_;

    Byte if_;
    TCycleCount closest_interrupt_time_ = TCycle_never;
    InterruptType closest_interrupt_type_;
    std::array<TCycleCount, 4> interrupt_times_ = {
        TCycle_never,
        TCycle_never,
        TCycle_never,
        TCycle_never,
    };
};

}  // namespace GbcEmulator
