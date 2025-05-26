#include "log_window.hpp"

#include <sstream>

#include <imgui/imgui.h>
#include "font/fa_icons.h"

#include "logging.hpp"
#include "application.hpp"

static constexpr std::array<ImVec4, logLevel_count> log_colors =
{
    ImVec4{ 0.75, 0,    0,    1 }, // Magenta
    ImVec4{ 1,    0,    0,    1 }, // Red
    ImVec4{ 1,    1,    0.5,  1 }, // Yellow
    ImVec4{ 1,    1,    1,    1 }, // White
    ImVec4{ 0.75, 0.75, 0.75, 1 }, // Light Gray
    ImVec4{ 0.5,  0.5,  0.5,  1 }  // Gray
};

void LogWindow::draw()
{
    ImGui::SetNextWindowSize(ImVec2(750, 350), ImGuiCond_Once);
    if (!ImGui::Begin(getName(), &is_opened_))
    {
        ImGui::End();
        return;
    }

    ImGui::InputText("##test_message", test_message_input_.data(), test_message_input_.size()-1);
    ImGui::SameLine();
    if (ImGui::BeginCombo("##combo", "Send as...", ImGuiComboFlags_WidthFitPreview))
    {
        for (int i = 0; i < logLevel_count; ++i)
        {
            if (ImGui::Selectable(logLevelToString(static_cast<TLogLevel>(i))))
            {
                LOG(static_cast<TLogLevel>(i)) << test_message_input_.data();
                break;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Text("Current logging level: %s", logLevelToString(Log::reporting_level));

    if (ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
    {
        ImGuiListClipper clipper;
        const auto& message_queue = Log::getMessageLog();
        clipper.Begin(static_cast<int>(message_queue.size()));
        while (clipper.Step())
            for (size_t i = static_cast<size_t>(clipper.DisplayStart); i < static_cast<size_t>(clipper.DisplayEnd); ++i)
            {
                const LogEntry& entry = message_queue[i];

                std::ostringstream os;
                // TODO: that's not portable
                if (tm* time = std::localtime(&(entry.timestamp)))
                    os << std::put_time(time, "%H:%M:%S");
                else
                    os << "??:??:??";
                    
                ImGui::TextColored(log_colors[entry.level], "%s | %s | %s", os.str().c_str(), logLevelToString(entry.level), entry.message.c_str());
            }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

REGISTER_WINDOW(LogWindow)
