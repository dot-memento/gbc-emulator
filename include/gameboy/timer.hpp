#pragma once

#include "types.hpp"

namespace GbcEmulator {

class Clock;
class InterruptScheduler;

class Timer {
public:
    Timer(Clock& clock, InterruptScheduler& interrupt_scheduler)
    : clock_{clock}
    , interrupt_scheduler_{interrupt_scheduler} { reset(); }
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    Byte getDiv();
    Byte getTima();
    constexpr Byte getTma() const { return tma_; }
    constexpr Byte getTac() const { return tac_; }

    void setDiv(Byte value);
    void setTima(Byte value);
    void setTma(Byte value);
    void setTac(Byte value);

    TCycleCount getNextInterruptTime();

    void reset();

private:
    void catchUp();
    void checkFallingEdgeTimaTrigger();

    Clock& clock_;
    InterruptScheduler& interrupt_scheduler_;

    TCycleCount last_timestamp_;

    // in T-cycles
    TCycleCount full_div_t_clock_;

    Byte tima_, tma_, tac_;

    // in T-cycles
    inline static constexpr TCycleCount tac_period[4] = {256 << 2, 4 << 2, 16 << 2, 64 << 2};

    // in T-cycles
    inline static constexpr uint8_t tac_freq_shift[4] = {10, 4, 6, 8};

    // in T-cycles
    inline static constexpr TCycleCount tac_bit_mask[4] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};
};

}  // namespace GbcEmulator
