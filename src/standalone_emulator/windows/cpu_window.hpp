#pragma once

#include <vector>

#include <types.hpp>

#include "imgui_window.hpp"

class Application;

class CpuWindow : public SubWindow
{
public:
    explicit CpuWindow(Application* app) : application_{app} {}

    const char* getName() const override { return "Cpu"; }
    void draw() override;

private:
    void drawDebugControl();
    void drawCpuState();
    void drawInterrupts();
    void drawTimer();
    void drawSerialOutput();

    Application* application_;
    std::vector<GbcEmulator::Word> breakpoints_;
};
