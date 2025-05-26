add_executable(StandaloneEmulator)

target_link_libraries(StandaloneEmulator
    PRIVATE
        gbc_compiler_flags
        GameBoy
        glad
        glfw
        ImGui
        ImGuiFileDialog
)

configure_file(build_config.hpp.in build_config.hpp @ONLY ESCAPE_QUOTES)

target_include_directories(StandaloneEmulator
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include/standalone_emulator"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}" # to include configured file
)

target_sources(StandaloneEmulator
    PRIVATE
        main.cpp
        init.cpp
        application.cpp
        application_toolbar.cpp
        emulation_window.cpp

        shader/shader_part.cpp
        shader/shader.cpp
        shader/shader_bank.cpp

        windows/demo_window.cpp
        windows/settings_window.cpp
        windows/log_window.cpp
        windows/cpu_window.cpp
        windows/memory_window.cpp
        windows/ppu_window.cpp
)
