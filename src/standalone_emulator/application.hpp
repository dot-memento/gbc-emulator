#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <chrono>

#include <gameboy.hpp>

#include "logging.hpp"

struct GLFWwindow;
class SubWindow;
class EmulationWindow;

#define REGISTER_WINDOW(Type) \
static_assert(std::is_base_of_v<SubWindow, Type> == true); \
namespace { const bool reg = Application::registerWindow<Type>(); }

class Application
{
public:
    using WindowFactory = std::function<std::unique_ptr<SubWindow>(Application*)>;
    template<class T>
    static bool registerWindow()
    {
        LOG_DEBUG << "Registering window type " << typeid(T).name();
        WindowFactory lambda = [](Application* app) { return std::make_unique<T>(app); };
        getWindowFactories().push_back(lambda);
        return true;
    }

private:
    static std::vector<WindowFactory>& getWindowFactories()
    {
        static std::vector<WindowFactory> window_factories;
        return window_factories;
    }


public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool isRunning();
    void update();
    void draw();

    EmulationWindow* getEmulationWindow() const { return emulation_window_.get(); }
    GbcEmulator::GameBoy& getEmulator() { return gb_; }
    constexpr bool isEmulatorRunning() const { return gb_.isRunning(); }

    void resetEmulator();

    using KeyInterceptHandler = void(*)(int key, int scancode, int mods);
    void interceptNextKey(KeyInterceptHandler handler);
    
private:
    void drawToolBar();
    void changeTitle(const std::string& title);
    
    bool is_toolbar_visible_ = true;

    GLFWwindow* glfw_window_;
    KeyInterceptHandler key_intercept_handler_ = nullptr;

    GbcEmulator::GameBoy gb_;
    std::chrono::steady_clock::time_point last_frame_timepoint_;

    std::unique_ptr<EmulationWindow> emulation_window_;
    std::vector<std::unique_ptr<SubWindow>> sub_windows_;

    friend void glfwApplicationKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    friend void glfwInterceptKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
