#pragma once

#include <vector>

#include "clock.hpp"
#include "types.hpp"

namespace GbcEmulator {

class InterruptScheduler;

class SerialConnection {
public:
    SerialConnection(Clock& clock, InterruptScheduler& interrupt_scheduler)
        : clock_{clock}, interrupt_scheduler_{interrupt_scheduler} { reset(); }
    SerialConnection(const SerialConnection&) = delete;
    SerialConnection& operator=(const SerialConnection&) = delete;

    constexpr Byte getSc() const { return sc_; }
    constexpr Byte getSb() { return sb_; }

    constexpr void setSc(Byte value) { sc_ = value; }

    inline void setSb(Byte value) {
        sb_ = value;
        if (serial_connection_buffer_.size() < max_buffer_size)
            serial_connection_buffer_.push_back(value);
    }

    constexpr TCycleCount getNextInterruptTime() { return TCycle_never; }

    constexpr const std::vector<Byte>& getSerialBuffer() const { return serial_connection_buffer_; }

    void reset() {
        sb_ = 0;
        sc_ = 0;
        serial_connection_buffer_.resize(0);
    }

private:
    inline static constexpr size_t max_buffer_size = 4092;

    Clock& clock_;
    InterruptScheduler& interrupt_scheduler_;

    Byte sb_, sc_;
    std::vector<Byte> serial_connection_buffer_;
};

}  // namespace GbcEmulator
