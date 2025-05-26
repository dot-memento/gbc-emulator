#pragma once

#include "imgui_window.hpp"
#include "opengl_types.hpp"

class Application;

class PpuWindow : public SubWindow
{
public:
    explicit PpuWindow(Application* app);

    const char* getName() const override { return "Ppu"; }
    void draw() override;

private:
    void drawTileset();
    void drawBackground();

    Application* application_;
    GLuint video_memory_texture_;
};
