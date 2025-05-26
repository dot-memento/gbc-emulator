#include "cartridge.hpp"

#include <stdexcept>
#include <algorithm>
#include <numeric>

namespace GbcEmulator {

static constexpr std::array<uint8_t, 48> nintendo_logo = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

static constexpr std::array<size_t, 9> rom_sizes = {
    16384 * 2,  16384 * 4,   16384 * 8,   16384 * 16,  16384 * 32,
    16384 * 64, 16384 * 128, 16384 * 256, 16384 * 512
};

static constexpr std::array<size_t, 6> eram_sizes = {
    0, 0, 8192 * 1, 8192 * 4, 8192 * 16, 8192 * 8
};

Cartridge::Cartridge(std::span<uint8_t> rom, std::span<uint8_t> eram)
{
    // Check nintendo logo
    is_logo_ok_ = std::equal(nintendo_logo.cbegin(), nintendo_logo.cend(), rom.begin() + 0x0104);

    // Check if has a CGB flag (not part of title), or it could be an ASCII character in the title
    uint16_t title_end = rom[0x0143] > 0x7F ? 0x143 : 0x144;
    name_ = std::string{rom.begin() + 0x134, rom.begin() + title_end};
    
    // New licensee code
    new_licensee_code_ = { rom[0x0144], rom[0x0145] };

    // SGB flag
    sgb_flag_ = rom[0x0146];

    // Setting up ROM
    uint8_t rom_size_flag = rom[0x0148];
    if (rom_size_flag >= rom_sizes.size())
        throw std::invalid_argument("Cartridge ROM size flag invalid");
    if (rom.size() != rom_sizes[rom_size_flag])
        throw std::invalid_argument("Cartridge ROM size not matching size flag");
    rom_.assign(rom.begin(), rom.end());
    selected_rom_bank_ = rom_.begin() + 0x4000;

    // Setting up RAM
    // TODO: Handle no external RAM (currently using 8KiB minimum)
    uint8_t eram_size_flag = rom[0x0149];
    if (eram_size_flag >= eram_sizes.size())
        throw std::invalid_argument("Cartridge external RAM size flag invalid");
    if (eram.size() != eram_sizes[eram_size_flag])
        throw std::invalid_argument("Cartridge external RAM size not matching size flag");
    if (eram_.size())
        eram_.assign(eram.begin(), eram.end());
    else
        eram_ = std::vector<uint8_t>(0x2000, 0xFF); // Dummy eRAM
    selected_eram_bank_ = eram_.begin() + 0x1000;

    // Destination code
    destination_code_ = rom[0x014A];

    // Old licensee code
    old_licensee_code_ = rom[0x014B];

    // ROM version number
    rom_version_number_ = rom[0x014C];

    // Check header checksum (25 bytes)
    uint8_t header_checksum = -std::accumulate(rom.begin() + 0x0134, rom.begin() + 0x014D, static_cast<uint8_t>(25));
    is_header_checksum_ok_ = (header_checksum == rom[0x014D]);

    // Check full ROM checksum
    uint16_t rom_checksum = std::accumulate(rom.begin(), rom.end(), 0) - rom[0x014E] - rom[0x014F];
    uint16_t expected_rom_checksum = rom[0x014E] << 8 | rom[0x014F];
    is_full_checksum_ok_ = (rom_checksum == expected_rom_checksum);
}

}