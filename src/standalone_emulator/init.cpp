#include "init.hpp"

#include <array>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "logging.hpp"

#include "font/fa_solid_font.h"
#include "font/fa_icons.h"

#include "constants.hpp"
#include "build_config.hpp"

namespace {

void stdoutLoggingCallback(const LogEntry& entry)
{
    static constexpr const char* color_red =      "\033[31m";
    static constexpr const char* color_yellow =   "\033[33m";
    static constexpr const char* color_white =    "\033[37m";
    static constexpr const char* color_dim_white = "\033[37;2m";
    static constexpr std::array<const char*, logLevel_count> colors =
    { color_red, color_red, color_yellow, color_white, color_white, color_dim_white };
    
    std::cerr << colors.at(entry.level);
    
    // TODO: that's not portable
    if (tm* time = std::localtime(&(entry.timestamp)))
        std::cerr << std::put_time(time, "%H:%M:%S");
    else
        std::cerr << "??:??:??";

    std::cerr << " | " << logLevelToString(entry.level) << " | "
        << entry.message << "\033[0m\n";
    std::flush(std::cerr);
}

void glfwErrorCallback(int error, const char* description)
{
    LOG_ERROR << "[GLFW] Error#" << error << ": " << description;
}

void GLAPIENTRY
openglMessageCallback(GLenum source,
                GLenum type,
                [[maybe_unused]] GLuint id,
                GLenum severity,
                [[maybe_unused]] GLsizei length,
                const GLchar* message,
                [[maybe_unused]] const void* user_param )
{
    std::ostringstream os;
    os << "[OpenGL] ";
    
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:               os << "[API] "; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     os << "[WINDOW] "; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:   os << "[SHADER] "; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:       os << "[3rd PARTY] "; break;
    case GL_DEBUG_SOURCE_APPLICATION:       os << "[APP] "; break;
    //case GL_DEBUG_SOURCE_OTHER:             break;
    default:                                break;
    }
    
    switch (type)
    {
    //case GL_DEBUG_TYPE_ERROR:               break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: os << "[DEPRECTAED] "; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  os << "[UB] "; break;
    case GL_DEBUG_TYPE_PORTABILITY:         os << "[PORTABILITY] "; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         os << "[PERF] "; break;
    case GL_DEBUG_TYPE_MARKER:              os << "[MARKER] "; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          os << "[PUSH] "; break;
    case GL_DEBUG_TYPE_POP_GROUP:           os << "[POP] "; break;
    //case GL_DEBUG_TYPE_OTHER:               break;
    default:                                break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:            LOG_ERROR << os.str() << message; break;
    default:                                LOG_WARN << os.str() << message; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:    LOG_DEBUG << os.str() << message; break;
    }
}

}


void setupLogging()
{
    for (const auto& entry : Log::getMessageLog())
        stdoutLoggingCallback(entry);
    Log::log_callbacks.push_back(stdoutLoggingCallback);

    LOG_INFO << Project::project_name
        << " v" << Project::version_major << '.' << Project::version_minor;
}

GLFWwindow* initializeGlfw()
{
    // Initialize GLFW
    glfwSetErrorCallback(glfwErrorCallback);

    LOG_INFO << "GLFW v" << glfwGetVersionString();

    if (glfwInit() == GLFW_FALSE)
    {
        LOG_FATAL << "Failed to initialize GLFW";
        exit(EXIT_FAILURE);
    }

    // Create GLFW window (and context)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    constexpr static int window_width  = 640;
    constexpr static int window_height = 480;

    GLFWwindow* gflw_window = glfwCreateWindow(window_width, window_height,
                                                Constants::main_window_title, nullptr, nullptr);
    if (!gflw_window)
    {
        LOG_FATAL << "Failed to create GLFW context";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glfwMakeContextCurrent(gflw_window);
    
    // Load OpenGL function through GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_FATAL << "Failed to initialize GLAD";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    LOG_INFO << "OpenGL v" << glGetString(GL_VERSION)
        << " (" << glGetString(GL_RENDERER) << ")";

    if (!GLAD_GL_VERSION_3_2)
    {
        LOG_FATAL << "Failed to find OpenGL 3.2 through GLAD";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    glDebugMessageCallback(openglMessageCallback, nullptr);

    return gflw_window;
}

void initializeImGui()
{
    GLFWwindow* gflw_window = glfwGetCurrentContext();
    if (!gflw_window)
    {
        LOG_ERROR << "Failed to get current GLFW context";
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    LOG_INFO << "ImGui version " << ImGui::GetVersion();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(gflw_window, true))
    {
        LOG_ERROR << "Failed to initialize ImGui";
        ImGui::DestroyContext();
        return;
    }

    if (!ImGui_ImplOpenGL3_Init())
    {
        LOG_ERROR << "Failed to initialize ImGui";
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return;
    }
    
    // Setup ImGui fonts
    io.Fonts->AddFontDefault();

    constexpr static const std::array<ImWchar, 3> icon_ranges = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    constexpr static const float fontSize = 13.0f;
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = fontSize;
    io.Fonts->AddFontFromMemoryCompressedTTF(static_cast<const void*>(fa_solid_compressed_data),
        fa_solid_compressed_size, fontSize, &config, icon_ranges.data());
}

void prepareImGuiFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void renderImGuiFrame()
{
    ImGui::Render();

    const ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void terminateImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void terminateGlfw()
{
    glfwTerminate();
}
