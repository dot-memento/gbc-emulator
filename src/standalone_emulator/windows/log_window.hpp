#pragma once

#include <array>

#include "imgui_window.hpp"
#include "logging.hpp"

class Application;

class LogWindow : public SubWindow
{
public:
    explicit LogWindow(Application* app) : application_{app} {}

    const char* getName() const override { return "Logs"; }
    void draw() override;

private:
    void drawShaderSettings();

    Application* application_;

    std::array<char, 256> test_message_input_{"This is a test message."};
};
