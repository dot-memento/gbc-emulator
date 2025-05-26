#include "application.hpp"

#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "init.hpp"
#include "emulation_window.hpp"
#include "shader/shader_bank.hpp"

#include "windows/imgui_window.hpp"

#include "constants.hpp"

void glfwApplicationKeyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            return;
        
        case GLFW_KEY_LEFT_ALT:
            app->is_toolbar_visible_ = !app->is_toolbar_visible_;
            return;
        
        default:
            return;
        }
    }
}

void glfwInterceptKeyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, [[maybe_unused]] int action, [[maybe_unused]] int mods)
{
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    Application::KeyInterceptHandler handler = app->key_intercept_handler_;
    app->key_intercept_handler_ = nullptr;
    handler(key, scancode, mods);

    if (!app->key_intercept_handler_)
        glfwSetKeyCallback(window, glfwApplicationKeyCallback);
}

Application::Application()
{
    setupLogging();

    glfw_window_ = initializeGlfw();

    glfwSetWindowUserPointer(glfw_window_, this);
    glfwSetKeyCallback(glfw_window_, glfwApplicationKeyCallback);

    emulation_window_ = std::make_unique<EmulationWindow>(glfw_window_, gb_.getPpu().getScreenData());

    initializeImGui();

    for (const auto& window_factory : getWindowFactories())
        sub_windows_.push_back(window_factory(this));
}

Application::~Application()
{
    // Drop shader cache first because shader destructors need OpenGL functions
    OpenGL::dropShaderCache();

    terminateImGui();
    terminateGlfw();
}

bool Application::isRunning()
{
    return !glfwWindowShouldClose(glfw_window_);
}

void Application::update()
{
    glfwPollEvents();

    using FpMilliseconds = std::chrono::duration<float, std::chrono::seconds::period>;
    auto now = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<FpMilliseconds>(now - last_frame_timepoint_);
    gb_.runFor(static_cast<GbcEmulator::TCycleCount>(delta.count() * 4194304.f));
    last_frame_timepoint_ = now;

    prepareImGuiFrame();

    if (is_toolbar_visible_)
        drawToolBar();
    
    for (const auto& window_ptr : sub_windows_)
    {
        if (window_ptr->isOpened())
            window_ptr->draw();
    }
}

void Application::draw()
{
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    emulation_window_->draw();

    renderImGuiFrame();
    
    glfwSwapBuffers(glfw_window_);
}

void Application::resetEmulator()
{
    gb_.reset();
}

void Application::interceptNextKey(KeyInterceptHandler handler)
{
    key_intercept_handler_ = handler;
    glfwSetKeyCallback(glfw_window_, glfwInterceptKeyCallback);
}

void Application::changeTitle(const std::string& title)
{
    std::string full_title = Constants::main_window_title;
    full_title += " - " + title;
    glfwSetWindowTitle(glfw_window_, full_title.c_str());
}
