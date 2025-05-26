#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/ImGuiFileDialog.h>

#include "application.hpp"
#include "windows/imgui_window.hpp"
#include "logging.hpp"

void Application::drawToolBar()
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open..."))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseRom", "Choose File", ".gb,.*", config);
        }

        ImGui::EndMenu();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseRom"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            if (gb_.loadRomFile(filePathName))
                resetEmulator();
            else
                LOG_ERROR << "Couldn't load file game from '" << filePathName << '\'';
        }
        
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::BeginMenu("Window"))
    {
        for (const auto& window_ptr : sub_windows_)
        {
            SubWindow* imgui_window = window_ptr.get();

            bool was_selected = imgui_window->isOpened();
            bool is_selected = was_selected;
            ImGui::MenuItem(imgui_window->getName(), nullptr, &is_selected);

            if (is_selected && !was_selected)
                imgui_window->open();
            else if (was_selected && !is_selected)
                imgui_window->close();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
