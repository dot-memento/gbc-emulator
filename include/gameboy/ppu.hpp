#pragma once

#include <array>
    
#include "types.hpp"

namespace GbcEmulator {

class Clock;
class InterruptScheduler;

class Ppu {
public:
    Ppu(Clock& clock, InterruptScheduler& interrupt_scheduler)
    : clock_{clock}, interrupt_scheduler_{interrupt_scheduler}
    { reset(); }

    Byte getLcdc() {
        return lcdc;
    }

    Byte getStat() {
        catchUp();
        return stat;
    }

    Byte getLy() {
        catchUp();
        return ly;
    }

    Byte getLyc() {
        return lyc;
    }

    Byte getScx() {
        return scx;
    }

    Byte getScy() {
        return scy;
    }

    void setLcdc(Byte byte) {
        catchUp();
        lcdc = byte;
    }

    void setStat(Byte byte) {
        catchUp();
        stat = byte;
    }

    void setLy(Byte byte) {
        catchUp();
        ly = byte;
    }

    void setLyc(Byte byte) {
        catchUp();
        lyc = byte;
    }

    void setScx(Byte byte) {
        catchUp();
        scx = byte;
    }

    void setScy(Byte byte) {
        catchUp();
        scy = byte;
    }

    void catchUp();
    void reset();

    constexpr static int screen_width  = 160;
    constexpr static int screen_height = 144;
    const std::array<uint16_t, screen_width*screen_height>& getScreenData() const { return pixel_data_; }

private:
    constexpr static int scanline_dot_count = 456;
    constexpr static int scanline_count = 154;
    constexpr static int full_frame_dot_count = scanline_dot_count * scanline_count;

    Clock& clock_;
    InterruptScheduler& interrupt_scheduler_;

    TCycleCount last_timestamp_;

    Byte lcdc, stat, scy, scx, ly, lyc, bgp, obp0, obp1, wy, wx;

    std::array<uint16_t, screen_width*screen_height> pixel_data_;
    uint16_t scanline_x;
};

}  // namespace GbcEmulator
