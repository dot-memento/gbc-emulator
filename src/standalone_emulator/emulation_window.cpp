#include "emulation_window.hpp"

#include <cstdint>
#include <array>
#include <cassert>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader/shader.hpp"
#include "shader/shader_bank.hpp"
#include "logging.hpp"

namespace {

constexpr const GLchar* shader_texture_uniform_name = "fbo_texture";

constexpr std::array<GLfloat, 6> fbo_vertices = {
    -1, -1,
    3, -1,
    -1,  3
};

}

EmulationWindow::EmulationWindow(
    GLFWwindow* glfw_window, const std::array<uint16_t, 160*144>& screen_data)
: glfw_window_{glfw_window}, screen_data_{screen_data}
{
    glGenTextures(1, &screen_texture_);
    glBindTexture(GL_TEXTURE_2D, screen_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Create Vertex Buffer Object
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_); 
    glBufferData(GL_ARRAY_BUFFER, fbo_vertices.size() * sizeof(GLfloat), fbo_vertices.data(), GL_STATIC_DRAW);

    // Create Vertex Array Object
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Selecting default shader
    useShader(OpenGL::defaultShader());
}

EmulationWindow::~EmulationWindow()
{
    glDeleteTextures(1, &screen_texture_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void EmulationWindow::draw()
{
    int width, height;
    glfwGetFramebufferSize(glfw_window_, &width, &height);

    glBindTexture(GL_TEXTURE_2D, screen_texture_);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        160,
        144,
        0,
        GL_RGBA,
        GL_UNSIGNED_SHORT_5_5_5_1,
        screen_data_.data()
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindVertexArray(vao_);
    
    current_shader_->useShader();
    texture_uniform_->setValue(screen_texture_);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void EmulationWindow::useShader(const OpenGL::Shader* shader)
{
    assert(shader != nullptr);

    LOG_DEBUG << "Switching shader to '" << shader->getName() << "'.";
    current_shader_ = shader;
    texture_uniform_ = shader->getUniform<OpenGL::Texture2DUniform>(shader_texture_uniform_name);
}
