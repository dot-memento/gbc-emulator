#pragma once

#include <array>

#include "opengl_types.hpp"

struct GLFWwindow;

namespace OpenGL {
    class Shader;
    class Texture2DUniform;
}

class EmulationWindow
{
public:
    EmulationWindow(GLFWwindow* glfw_window, const std::array<uint16_t, 160*144>& screen_data);
    ~EmulationWindow();
    EmulationWindow(const EmulationWindow&) = delete;
    EmulationWindow& operator=(const EmulationWindow&) = delete;

    void draw();

    void useShader(const OpenGL::Shader* shader);
    const OpenGL::Shader* getShader() { return current_shader_; }

private:
    GLFWwindow* glfw_window_;
    const std::array<uint16_t, 160*144>& screen_data_;

    GLuint screen_texture_, vao_, vbo_;

    const OpenGL::Shader* current_shader_;
    const OpenGL::Texture2DUniform* texture_uniform_;
};
