#include "gameboy.hpp"

#include <fstream>

#include "cartridge.hpp"

namespace GbcEmulator {

static constexpr CpuState createPostBootState()
{
    CpuState state;
    state[Reg16::PC] = 0x0100u;
    state[Reg16::AF] = 0x1180u;
    state[Reg16::BC] = 0x0000u;
    state[Reg16::DE] = 0xFF56u;
    state[Reg16::HL] = 0x000Du;
    state[Reg16::SP] = 0xFFFEu;
    state.ime = false;
    state.next_ime = false;
    state.mode = CpuState::Mode::Normal;
    return state;
}

GameBoy::GameBoy()
: mmu_{*this}
, cpu_{mmu_}
, interrupt_{cpu_.getClock(), *this}
, timer_{cpu_.getClock(), interrupt_}
, serial_{cpu_.getClock(), interrupt_}
, ppu_{cpu_.getClock(), interrupt_}
{
    reset();
}

void GameBoy::runFor(TCycleCount t_cycles) { cpu_.stepTCycles(t_cycles); }

bool GameBoy::loadRomFile(const std::string& path) {
    std::ifstream test_rom_file(path, std::ios::binary);
    if (!test_rom_file.is_open())
        return false;

    test_rom_file.seekg(0, std::ios::end);
    std::streampos file_size = test_rom_file.tellg();
    test_rom_file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data;
    data.reserve(static_cast<std::size_t>(file_size));
    data.insert(data.cbegin(),
                (std::istreambuf_iterator<char>(test_rom_file)),
                std::istreambuf_iterator<char>());

    if (test_rom_file.fail())
        return false;
    test_rom_file.close();

    Cartridge cartridge{data};
    if (!cartridge.isRomBootable())
        return false;

    mmu_.loadCartridge(std::move(cartridge));
    return true;
}

void GameBoy::reset()
{
    mmu_.reset();
    cpu_.reset();
    interrupt_.reset();
    timer_.reset();
    serial_.reset();
    ppu_.reset();
    
    cpu_.restoreStateSnapshot(createPostBootState());
    setPause(true);
}

}  // namespace GbcEmulator