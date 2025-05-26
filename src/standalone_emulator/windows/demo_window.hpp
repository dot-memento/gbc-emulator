#pragma once

#include "imgui_window.hpp"

class Application;

class DemoWindow : public SubWindow
{
public:
    explicit DemoWindow(Application* app) : application_{app} {}

    const char* getName() const override { return "Demo"; }
    void draw() override;

private:
    Application* application_;
};
