#pragma once

#include <array>
#include <memory>

#include "types.hpp"

namespace GbcEmulator {

class GameBoy;
class Cartridge;

class MemoryManagmentUnit {
public:
    explicit MemoryManagmentUnit(GameBoy& gb);
    MemoryManagmentUnit(const MemoryManagmentUnit&) = delete;
    MemoryManagmentUnit& operator=(const MemoryManagmentUnit&) = delete;
    ~MemoryManagmentUnit();

    Byte load(Word address);
    void store(Word address, Byte value);

    void loadCartridge(Cartridge&& cartridge);
    bool hasCartridge() const { return static_cast<bool>(cartridge_); }

    void reset();

private:
    GameBoy& gb_;

    std::unique_ptr<Cartridge> cartridge_;

    std::array<Byte, 0x4000> vram_;
    std::array<Byte, 0x4000>::iterator selected_vram_bank_;

    std::array<Byte, 0x8000> wram_;
    std::array<Byte, 0x8000>::iterator selected_wram_bank_;

    std::array<Byte, 0xA0> oam_;

    std::array<Byte, 0x80> io_;

    std::array<Byte, 0x7F> hram_;

    Byte ie_;

    friend class GameBoyDebugger;
};

}  // namespace GbcEmulator