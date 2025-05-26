# Had to slightly change the includes of the sources to fit the current ImGui CMake setup

add_library(ImGuiFileDialog STATIC ImGuiFileDialog.cpp)

target_link_libraries(ImGuiFileDialog PRIVATE ImGui)

target_include_directories(ImGuiFileDialog PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui")
target_include_directories(ImGuiFileDialog SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(UNIX)
    target_compile_options(ImGuiFileDialog PRIVATE -Wno-unknown-pragmas)
endif()
