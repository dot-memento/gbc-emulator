#pragma once

#include "mmu.hpp"
#include "cpu.hpp"
#include "interrupt_scheduler.hpp"
#include "serial_connection.hpp"
#include "timer.hpp"
#include "ppu.hpp"

namespace GbcEmulator {

class GameBoy {
public:
    GameBoy();
    GameBoy(const GameBoy&) = delete;
    GameBoy& operator=(const GameBoy&) = delete;

    constexpr MemoryManagmentUnit& getMmu() noexcept { return mmu_; }
    constexpr Cpu& getCpu() noexcept { return cpu_; }
    constexpr InterruptScheduler& getInterrupt() noexcept { return interrupt_; }
    constexpr Timer& getTimer() noexcept { return timer_; }
    constexpr SerialConnection& getSerial() noexcept { return serial_; }
    constexpr Ppu& getPpu() noexcept { return ppu_; }

    constexpr bool isRunning() const
    {
        return !cpu_.getState().paused;
    }

    bool loadRomFile(const std::string& path);
    void reset();

    void setPause(bool is_paused = true) { cpu_.setPause(is_paused); }
    void runFor(TCycleCount t_cycles);
    
private:
    MemoryManagmentUnit mmu_;
    Cpu cpu_;
    InterruptScheduler interrupt_;
    Timer timer_;
    SerialConnection serial_;
    Ppu ppu_;

    friend class GameBoyDebugger;
};

}  // namespace GbcEmulator
