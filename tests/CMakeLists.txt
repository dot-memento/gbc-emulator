# ---- Create gameboy_test target ----

add_executable(gameboy_test)

target_link_libraries(gameboy_test
    PRIVATE
        gbc_compiler_flags
        Catch2::Catch2WithMain
        GameBoy
)

target_include_directories(gameboy_test
    PRIVATE
        $<TARGET_PROPERTY:GameBoy,INCLUDE_DIRECTORIES>
)

target_sources(gameboy_test
    PRIVATE
        integrated_test.cpp
)

# ---- Add tests ----

catch_discover_tests(gameboy_test WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")