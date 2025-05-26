#include "demo_window.hpp"

#include <imgui/imgui.h>

#include "application.hpp"

void DemoWindow::draw()
{
    ImGui::ShowDemoWindow();
}

REGISTER_WINDOW(DemoWindow)
