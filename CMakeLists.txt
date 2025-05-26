cmake_minimum_required(VERSION 3.22)

cmake_policy(SET CMP0135 NEW)

# ---- Project ----

project(
    GBCEmulator
    VERSION 0.1
    DESCRIPTION "A simple GameBoy Color emulator."
    LANGUAGES C CXX
)

# ---- In-source guard ----
# In-source builds are messy, I don't see any reason someone might want this

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds not allowed. Please make a new directory and run CMake from there."
    )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Create compiler flag interface

add_library(gbc_compiler_flags INTERFACE)
target_compile_features(gbc_compiler_flags INTERFACE cxx_std_20)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(gbc_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wconversion;-Wsign-conversion;-Wformat=2>>" #$<$<CONFIG:Debug>:;-Og>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:/W4>>"
)

# ---- Create the gameboy core ----

add_subdirectory(src/gameboy)

# ---- Create the standalone executable ----

add_subdirectory(extern/glad)
add_subdirectory(extern/glfw)
add_subdirectory(extern/imgui)
add_subdirectory(extern/imgui_file_dialog)

add_subdirectory(src/embed_file)

add_subdirectory(src/standalone_emulator)

# ---- Testing ----

include(CTest)

if(BUILD_TESTING)
    enable_testing()

    add_subdirectory(extern/catch2 SYSTEM)
    add_subdirectory(tests)
endif()
