#pragma once

#include "imgui_window.hpp"

class Application;

class SettingsWindow : public SubWindow
{
public:
    explicit SettingsWindow(Application* app) : application_{app} {}

    const char* getName() const override { return "Settings"; }
    void draw() override;

private:
    void drawShaderSettings();
    void drawControlsSettings();

    Application* application_;

    bool was_shader_list_opened_last_frame_ = false;
    char test_message_input_[256] = "This is a test message.";
};
