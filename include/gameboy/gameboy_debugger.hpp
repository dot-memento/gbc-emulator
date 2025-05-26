#pragma once

#include "gameboy.hpp"

namespace GbcEmulator {

class GameBoy;

class GameBoyDebugger {
public:
    GameBoyDebugger(GameBoy& gb) : gb_{gb} {}

    CpuState& getCpuState() { return gb_.cpu_.state_; }

private:
    GameBoy& gb_;
};

}  // namespace GbcEmulator
