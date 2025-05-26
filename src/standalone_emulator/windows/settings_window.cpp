#include "settings_window.hpp"

#include <filesystem>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include "font/fa_icons.h"

#include "application.hpp"
#include "emulation_window.hpp"
#include "shader/shader.hpp"
#include "shader/shader_bank.hpp"

#include "logging.hpp"
#include "constants.hpp"

// Listing shader
static std::vector<std::string> available_shaders;
static void searchAvailableShaders()
{
    LOG_TRACE << "Refreshing available shader list";

    available_shaders.clear();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(Constants::shader_directory))
    {
        if (!entry.is_regular_file())
            continue;
        available_shaders.push_back(entry.path().lexically_proximate(Constants::shader_directory).generic_string());
    }
}

void SettingsWindow::drawShaderSettings()
{
    EmulationWindow* emulation_window = application_->getEmulationWindow();
    if (ImGui::Button(ICON_FA_CUBE))
    {
        std::string shader_name = emulation_window->getShader()->getName();
        OpenGL::dropShaderCache();
        
        const OpenGL::Shader* shader = (shader_name == OpenGL::default_shader_name)
        ? OpenGL::defaultShader()
        : OpenGL::getScreenShaderFromName(shader_name);
        emulation_window->useShader(shader);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("Drop shader cache and recompile sahders");

    ImGui::SameLine();

    const std::string& current_shader_name = emulation_window->getShader()->getName();
    if (ImGui::BeginCombo("##shaderCombo", current_shader_name.c_str()))
    {
        if (!was_shader_list_opened_last_frame_)
        {
            was_shader_list_opened_last_frame_ = true;
            searchAvailableShaders();
        }

        {
            bool is_selected = (current_shader_name == OpenGL::default_shader_name);
            if (ImGui::Selectable(OpenGL::default_shader_name.c_str(), is_selected))
                emulation_window->useShader(OpenGL::defaultShader());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }

        for (const auto& shader_name : available_shaders)
        {
            bool is_selected = (current_shader_name == shader_name);
            if (ImGui::Selectable(shader_name.c_str(), is_selected))
                emulation_window->useShader(OpenGL::getScreenShaderFromName(shader_name));
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
    else
        was_shader_list_opened_last_frame_ = false;
}

static void keyInterceptor(int key, int scancode, [[maybe_unused]] int mods)
{
    LOG_DEBUG << "Key intercepted: '" << glfwGetKeyName(key, scancode) << "'";
}

void SettingsWindow::drawControlsSettings()
{
    if (ImGui::Button("Intercept next key"))
    {
        application_->interceptNextKey(keyInterceptor);
    }
}

void SettingsWindow::draw()
{
    if (!ImGui::Begin(getName(), &is_opened_))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("Tabs"))
    {
        if (ImGui::BeginTabItem("General"))
        {
            ImGui::Text("Smoothed FPS: %f", ImGui::GetIO().Framerate);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shader"))
        {
            drawShaderSettings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Controls"))
        {
            drawControlsSettings();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

REGISTER_WINDOW(SettingsWindow)
