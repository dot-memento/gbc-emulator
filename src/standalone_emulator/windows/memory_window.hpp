#pragma once

#include "imgui_window.hpp"

class Application;

class MemoryWindow : public SubWindow
{
public:
    explicit MemoryWindow(Application* app) : application_{app} {}

    const char* getName() const override { return "Memory"; }
    void draw() override;

private:
    Application* application_;
};
