#include "ppu.hpp"

#include "clock.hpp"

namespace GbcEmulator {

void Ppu::catchUp()
{
    TCycleCount now = clock_.get();
    TCycleCount delta = now - last_timestamp_;
    last_timestamp_ = now;

    while (delta >= scanline_dot_count)
    {
        delta -= scanline_dot_count;
        if (++ly >= scanline_count)
            ly -= scanline_count;
    }
    
    scanline_x += delta;
    if (scanline_x >= scanline_dot_count)
    {
        scanline_x -= scanline_dot_count;
        if (++ly >= scanline_count)
            ly -= scanline_count;
    }
}

void Ppu::reset()
{
    last_timestamp_ = 0;
    pixel_data_.fill(0);
    scanline_x = 0;
    lyc = 0;
    ly = 0;
}

}  // namespace GbcEmulator
