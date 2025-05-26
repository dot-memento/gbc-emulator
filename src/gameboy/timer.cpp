#include "timer.hpp"

#include "clock.hpp"
#include "interrupt_scheduler.hpp"

namespace GbcEmulator {

Byte Timer::getDiv() {
    catchUp();
    return (full_div_t_clock_ >> 8) & 0xFF;
}

Byte Timer::getTima() {
    catchUp();
    return tima_;
}

void Timer::setDiv([[maybe_unused]] Byte value) {
    catchUp();

    Byte tac_freq = tac_ & 0x3;
    tima_ += (tac_ & 0x4) && (full_div_t_clock_ & tac_bit_mask[tac_freq]);

    full_div_t_clock_ = 0;

    interrupt_scheduler_.reschedule(InterruptType::Timer, getNextInterruptTime());
}

void Timer::setTima(Byte value) {
    catchUp();
    tima_ = value;

    interrupt_scheduler_.reschedule(InterruptType::Timer, getNextInterruptTime());
}

void Timer::setTma(Byte value) {
    catchUp();
    tma_ = value;
    tima_ = value;

    interrupt_scheduler_.reschedule(InterruptType::Timer, getNextInterruptTime());
}

void Timer::setTac(Byte value) {
    catchUp();

    if (tac_ & 0x4) {
        Byte old_tac_freq = tac_ & 0x3;
        Byte new_tac_freq = value & 0x3;

        bool was_tima_bit_high = (full_div_t_clock_ & tac_bit_mask[old_tac_freq]);
        bool is_tima_bit_high = (full_div_t_clock_ & tac_bit_mask[new_tac_freq]);
        tima_ += !(is_tima_bit_high && (value & 0x4)) && was_tima_bit_high;
    }

    tac_ = value;

    interrupt_scheduler_.reschedule(InterruptType::Timer, getNextInterruptTime());
}

void Timer::catchUp() {
    // depends on cpu mode (Double speed mode)
    TCycleCount delta_t_clock = (clock_.get() - last_timestamp_)
                                << static_cast<unsigned int>(clock_.isDoubleSpeed());
    last_timestamp_ = clock_.get();

    if (tac_ & 0x4) {
        Byte selected_freq = tac_ & 0x3;
        uint8_t freq_shift = tac_freq_shift[selected_freq];

        // calculate how much tima should be increased
        TCycleCount old_tima_clock = full_div_t_clock_ >> freq_shift;
        full_div_t_clock_ += delta_t_clock;
        TCycleCount new_tima_clock = full_div_t_clock_ >> freq_shift;
        TCycleCount delta_tima = new_tima_clock - old_tima_clock;

        // add delta tima to tima
        delta_tima += tima_ - tma_;
        tima_ = (tma_ + delta_tima % (0x100 - tma_)) & 0xFF;

        return;
    }

    full_div_t_clock_ += delta_t_clock;

    return;
}

TCycleCount Timer::getNextInterruptTime() {
    if (!(tac_ & 0x4)) return TCycle_never;

    catchUp();
    Byte selected_freq = tac_ & 0x3;
    TCycleCount tima_period = tac_period[selected_freq];
    return clock_.get() + tima_period * (0x100 - tima_) - (full_div_t_clock_ & (tima_period - 1));
}

void Timer::reset()
{
    last_timestamp_ = 0;
    full_div_t_clock_ = 0;
    tima_ = 0;
    tma_ = 0;
    tac_ = 0xF8;
}

}  // namespace GbcEmulator
