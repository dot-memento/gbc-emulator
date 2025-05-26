#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <string>

#include <gameboy.hpp>

static std::vector<uint8_t> test_rom(const char* path, unsigned long long t_cycle)
{
    GbcEmulator::GameBoy gb;
    gb.loadRomFile(path);
    gb.setPause(false);
    gb.runFor(t_cycle);
    return gb.getSerial().getSerialBuffer();
}

static std::string test_rom_string_output(const char* path, unsigned long long t_cycle)
{
    const auto serialBuffer = test_rom(path, t_cycle);
    return std::string(serialBuffer.cbegin(), serialBuffer.cend());
}

static constexpr GbcEmulator::TCycleCount timeout_limit = 100000000;

TEST_CASE( "Cpu instructions correctness (blargg)", "[cpu][integrated]" )
{
    const char* path = GENERATE(
        "tests/roms/cpu/instr/01-special.gb",
        "tests/roms/cpu/instr/02-interrupts.gb",
        "tests/roms/cpu/instr/03-op sp,hl.gb",
        "tests/roms/cpu/instr/04-op r,imm.gb",
        "tests/roms/cpu/instr/05-op rp.gb",
        "tests/roms/cpu/instr/06-ld r,r.gb",
        "tests/roms/cpu/instr/07-jr,jp,call,ret,rst.gb",
        "tests/roms/cpu/instr/08-misc instrs.gb",
        "tests/roms/cpu/instr/09-op r,r.gb",
        "tests/roms/cpu/instr/10-bit ops.gb",
        "tests/roms/cpu/instr/11-op a,(hl).gb"
    );
    SECTION( path )
    {
        REQUIRE_THAT( test_rom_string_output(path, timeout_limit), Catch::Matchers::EndsWith("Passed\n") );
    }
}

TEST_CASE( "Cpu instruction timing (blargg)", "[cpu][timer][integrated]" )
{
    const char* path = GENERATE(
        "tests/roms/cpu/timing/instr_timing.gb",
        "tests/roms/cpu/timing/01-read_timing.gb",
        "tests/roms/cpu/timing/02-write_timing.gb",
        "tests/roms/cpu/timing/03-modify_timing.gb"
    );
    SECTION( path )
    {
        REQUIRE_THAT( test_rom_string_output(path, timeout_limit), Catch::Matchers::EndsWith("Passed\n") );
    }
}

static constexpr std::array<uint8_t, 6> mooneye_magic_numbers = {3, 5, 8, 13, 21, 34};

TEST_CASE( "Cpu instructions correctness (mooneye)", "[cpu][integrated]" )
{
    const char* path = GENERATE(
        "tests/roms/cpu/instr/daa.gb",
        "tests/roms/cpu/instr/reg_f.gb"
    );
    SECTION( path )
    {
        REQUIRE_THAT( test_rom(path, timeout_limit), Catch::Matchers::RangeEquals(mooneye_magic_numbers) );
    }
}

TEST_CASE( "Timer (mooneye)", "[timer][integrated]" )
{
    const char* path = GENERATE(
        "tests/roms/timer/div_write.gb",
        "tests/roms/timer/rapid_toggle.gb",
        "tests/roms/timer/tim00_div_trigger.gb",
        "tests/roms/timer/tim00.gb",
        "tests/roms/timer/tim01_div_trigger.gb",
        "tests/roms/timer/tim01.gb",
        "tests/roms/timer/tim10_div_trigger.gb",
        "tests/roms/timer/tim10.gb",
        "tests/roms/timer/tim11_div_trigger.gb",
        "tests/roms/timer/tim11.gb",
        "tests/roms/timer/tima_reload.gb",
        "tests/roms/timer/tima_write_reloading.gb",
        "tests/roms/timer/tma_write_reloading.gb"
    );
    SECTION( path )
    {
        REQUIRE_THAT( test_rom(path, timeout_limit), Catch::Matchers::RangeEquals(mooneye_magic_numbers) );
    }
}

TEST_CASE( "Interrupt (mooneye)", "[interrupt][integrated]" )
{
    const char* path = GENERATE(
        "tests/roms/interrupt/ei_sequence.gb",
        "tests/roms/interrupt/ei_timing.gb",
        "tests/roms/interrupt/if_ie_registers.gb",
        "tests/roms/interrupt/intr_timing.gb",
        "tests/roms/interrupt/rapid_di_ei.gb"
    );
    SECTION( path )
    {
        REQUIRE_THAT( test_rom(path, timeout_limit), Catch::Matchers::RangeEquals(mooneye_magic_numbers) );
    }
}
