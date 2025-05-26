#include "mmu.hpp"

#include "cartridge.hpp"
#include "gameboy.hpp"

namespace GbcEmulator {

MemoryManagmentUnit::MemoryManagmentUnit(GameBoy& gb) : gb_(gb)
{
    reset();
}

MemoryManagmentUnit::~MemoryManagmentUnit() = default;

uint8_t MemoryManagmentUnit::load(uint16_t address) {
    if (address < 0x8000) return cartridge_->loadFromRom(address);
    if (address < 0xA000) return selected_vram_bank_[address - 0x8000];
    if (address < 0xC000) return cartridge_->loadFromExternRam(address - 0xA000);
    if (address < 0xD000) return wram_[address - 0xC000];
    if (address < 0xE000) return selected_wram_bank_[address - 0xD000];
    if (address < 0xFE00) return wram_[address - 0xE000];
    if (address < 0xFEA0) return oam_[address - 0xFE00];
    // [CGB rev E]: 0xFEA0-0xFEFF: returns high nibble of the lower address twice
    if (address < 0xFF00) return ((address >> 4) & 0xF) * 0x11;
    if (address == 0xFFFF) return ie_;
    if (address > 0xFF7F) return hram_[address - 0xFF80];

    switch (address & 0xFF) {
        case 0x01:
            return gb_.getSerial().getSb();
        case 0x02:
            return gb_.getSerial().getSc();

        case 0x04:
            return gb_.getTimer().getDiv();
        case 0x05:
            return gb_.getTimer().getTima();
        case 0x06:
            return gb_.getTimer().getTma();
        case 0x07:
            return gb_.getTimer().getTac();

        case 0x0F:
            return gb_.getInterrupt().getIf();

        case 0x40:
            return gb_.getPpu().getLcdc();
        case 0x41:
            return gb_.getPpu().getStat();
        case 0x42:
            return gb_.getPpu().getScy();
        case 0x43:
            return gb_.getPpu().getScx();
        case 0x44:
            return gb_.getPpu().getLy();
        case 0x45:
            return gb_.getPpu().getLyc();
    }

    return 0xFF;
}

void MemoryManagmentUnit::store(uint16_t address, uint8_t value) {
    if (address < 0x8000) {
        cartridge_->storeInRom(address, value);
        return;
    }
    if (address < 0xA000) {
        selected_vram_bank_[address - 0x8000] = value;
        return;
    }
    if (address < 0xC000) {
        cartridge_->storeInExternRam(address - 0xA000, value);
        return;
    }
    if (address < 0xD000) {
        wram_[address - 0xC000] = value;
        return;
    }
    if (address < 0xE000) {
        selected_wram_bank_[address - 0xD000] = value;
        return;
    }
    if (address < 0xFE00) {
        wram_[address - 0xE000] = value;
        return;
    }
    if (address < 0xFEA0) {
        oam_[address - 0xFE00] = value;
        return;
    }
    if (address < 0xFF00) return;
    if (address == 0xFFFF) {
        ie_ = value;
        return;
    }
    if (address > 0xFF7F) {
        hram_[address - 0xFF80] = value;
        return;
    }

    switch (address & 0xFF) {
        case 0x01:
            gb_.getSerial().setSb(value);
            return;
        case 0x02:
            gb_.getSerial().setSc(value);
            return;

        case 0x04:
            gb_.getTimer().setDiv(value);
            return;
        case 0x05:
            gb_.getTimer().setTima(value);
            return;
        case 0x06:
            gb_.getTimer().setTma(value);
            return;
        case 0x07:
            gb_.getTimer().setTac(value);
            return;

        case 0x0F:
            gb_.getInterrupt().setIf(value);
            return;

        case 0x40:
            gb_.getPpu().setLcdc(value);
            return;
        case 0x41:
            gb_.getPpu().setStat(value);
            return;
        case 0x42:
            gb_.getPpu().setScy(value);
            return;
        case 0x43:
            gb_.getPpu().setScx(value);
            return;
        case 0x44:
            gb_.getPpu().setLy(value);
            return;
        case 0x45:
            gb_.getPpu().setLyc(value);
            return;

        default:
            return;
    }
}

void MemoryManagmentUnit::loadCartridge(Cartridge&& cartridge) {
    cartridge_ = std::make_unique<Cartridge>(std::move(cartridge));
}

void MemoryManagmentUnit::reset()
{
    vram_.fill(0);
    selected_vram_bank_ = vram_.begin();

    wram_.fill(0);
    selected_wram_bank_ = wram_.begin() + 0x1000;

    oam_.fill(0);
    io_.fill(0);
    hram_.fill(0);

    ie_ = 0;
}

}  // namespace GbcEmulator
