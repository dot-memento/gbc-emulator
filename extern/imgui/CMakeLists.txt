add_library(ImGui
    STATIC
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui.cpp
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.cpp
        misc/cpp/imgui_stdlib.cpp
)

target_link_libraries(ImGui PRIVATE glfw)
target_compile_options(ImGui PRIVATE "$<$<CONFIG:Debug>:;-Og>")

target_include_directories(ImGui PRIVATE include/imgui)
target_include_directories(ImGui SYSTEM INTERFACE include)

set(IMGUI_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui"
    CACHE INTERNAL "IMGUI_INCLUDE_DIRECTORIES")
