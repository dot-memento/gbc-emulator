add_library(GameBoy STATIC)

target_link_libraries(GameBoy PRIVATE gbc_compiler_flags)

target_include_directories(GameBoy
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include/gameboy"
)

target_sources(GameBoy
    PRIVATE
        gameboy.cpp

        cartridge.cpp
        
        mmu.cpp
        cpu.cpp
        interrupt_scheduler.cpp
        timer.cpp
        ppu.cpp
)