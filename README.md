# GBC Emulator (Work in Progress)

This project is a work-in-progress emulator for the Nintendo Game Boy and Game Boy Color (GBC), written in C++20. The goal is to provide an accurate and user-friendly emulator with a modern standalone GUI.

## Features

- **Game Boy & Game Boy Color emulation**
- **Standalone GUI** using GLFW and ImGui
- **PPU, CPU, MMU, Timer, Interrupts**: Modular and testable components
- **Shader support** for enhanced visuals
- **Integrated tests** using Catch2
- **Built-in debugger support**

## Status

The project has lost momentum and is currently on hiatus.

- CPU opcodes (implemented and tested)
- Cycle accurate CPU instruction timing (implemented and tested)
- OpenGL shader for display (implemented)
- Hardware timers (implemented but not accurate)
- Display and PPU (partially implemented, background tiles in debug view)
- GUI debugging interface (partially implemented)

## Project Structure

```
include/ # Emulator C++ headers
src/     # Emulator C++ source files
extern/  # External dependencies (glfw, glad, imgui, etc.)
shaders/ # Custom GLSL shaders for rendering
tests/   # Unit and integration tests
```

## Building

This project uses CMake. You will need a C++20 compiler, CMake, and development libraries for OpenGL.
```sh
cd gbc-emulator
mkdir build && cd build
cmake ..
make
```
After this, launch the emulator with:
```sh
./build/bin/StandaloneEmulator
```

## Testing

Unit tests are located in the tests/ directory and use Catch2. To run tests (after building):
```sh
./build/bin/gameboy_test
```
The executable must be run from the root directory, as the test roms are in the `./tests/roms/` directory.

## Licence

[MIT License](LICENSE)