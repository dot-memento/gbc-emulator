#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>
#include <array>
#include <string>
#include <span>

namespace GbcEmulator {

class Cartridge {
public:
    Cartridge(std::span<uint8_t> rom, std::span<uint8_t> eram = std::span<uint8_t, 0>{});
    Cartridge(const Cartridge&) = delete;
    Cartridge& operator=(const Cartridge&) = delete;
    Cartridge(Cartridge&&) = default;
    Cartridge& operator=(Cartridge&&) = default;

    constexpr uint8_t loadFromRom(uint16_t address) const {
        return address < 0x4000 ? rom_[address] : selected_rom_bank_[address - 0x4000];
    }

    constexpr uint8_t loadFromExternRam(uint16_t address) const {
        return address < 0xD000 ? eram_[address] : selected_eram_bank_[address - 0xD000];
    }

    // TODO: handle depending on cartridge mapper
    constexpr void storeInRom(uint16_t address, uint8_t value) {}

    constexpr void storeInExternRam(uint16_t address, uint8_t value) {
        selected_eram_bank_[address] = value;
    }

    constexpr const std::string& getName() const { return name_; }
    constexpr bool isRomBootable() const { return is_logo_ok_ && is_header_checksum_ok_; }

private:
    std::vector<uint8_t> rom_;
    std::vector<uint8_t>::iterator selected_rom_bank_;
    std::vector<uint8_t> eram_;
    std::vector<uint8_t>::iterator selected_eram_bank_;

    std::string name_;
    uint8_t cgb_flag_;
    std::array<uint8_t, 2> new_licensee_code_;
    uint8_t sgb_flag_;
    uint8_t destination_code_;
    uint8_t old_licensee_code_;
    uint8_t rom_version_number_;
    bool is_logo_ok_, is_header_checksum_ok_, is_full_checksum_ok_;
};

}  // namespace GbcEmulator