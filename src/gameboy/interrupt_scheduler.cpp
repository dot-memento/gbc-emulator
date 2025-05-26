#include "interrupt_scheduler.hpp"

#include "clock.hpp"
#include "gameboy.hpp"

namespace GbcEmulator {

void InterruptScheduler::recalculateClosestInterrupt() {
    auto min = std::min_element(interrupt_times_.cbegin(), interrupt_times_.cend());
    assert(*min > clock_.get());
    closest_interrupt_type_ =
        static_cast<InterruptType>(std::distance(interrupt_times_.cbegin(), min));
    closest_interrupt_time_ = *min;
}

void InterruptScheduler::catchUp() {
    TCycleCount current_cycle = clock_.get();
    while (closest_interrupt_time_ < current_cycle) {
        auto casted_type = toUnderlying(closest_interrupt_type_);
        if_ |= interrupt_masks[casted_type];

        switch (closest_interrupt_type_) {
            case InterruptType::Joypad:
                break;
            case InterruptType::LCD:
                break;
            case InterruptType::Serial:
                break;

            case InterruptType::Timer:
                interrupt_times_[toUnderlying(InterruptType::Timer)] =
                    gb_.getTimer().getNextInterruptTime();
                break;

            case InterruptType::VBlank:
                break;
        }

        recalculateClosestInterrupt();
    }
}

void InterruptScheduler::reset()
{
    if_ = 0xE0;
    closest_interrupt_time_ = TCycle_never;
    interrupt_times_.fill(TCycle_never);
}

}  // namespace GbcEmulator