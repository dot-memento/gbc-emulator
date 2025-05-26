#include "cpu_window.hpp"

#include <algorithm>

#include <imgui/imgui.h>
#include <gameboy.hpp>

#include "application.hpp"
#include "font/fa_icons.h"
#include "logging.hpp"

using namespace GbcEmulator;

static void imgui_InputRegister8(const char* name, Byte* reg)
{
    ImGui::InputScalar(name, ImGuiDataType_U8, static_cast<void*>(reg), nullptr, nullptr,
            "%02X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
}

static void imgui_InputRegister16(const char* name, Word* reg)
{
    ImGui::InputScalar(name, ImGuiDataType_U16, static_cast<void*>(reg), nullptr, nullptr,
            "%04X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
}

void CpuWindow::drawDebugControl()
{
    auto& gb = application_->getEmulator();
    auto& cpu = gb.getCpu();
    auto& cpu_state = const_cast<CpuState&>(cpu.getState());

    bool is_emulator_running = application_->isEmulatorRunning();
    bool has_cartridge = gb.getMmu().hasCartridge();


    ImGui::BeginDisabled(!has_cartridge);
    
    ImGui::BeginDisabled(is_emulator_running);
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {
        gb.setPause(false);
        gb.runFor(1);
        gb.setPause(true);
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();

    if (ImGui::Button(is_emulator_running ? ICON_FA_PAUSE : ICON_FA_PLAY))
        gb.setPause(is_emulator_running);

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_ARROWS_ROTATE))
        application_->resetEmulator();

    ImGui::EndDisabled();

    if (ImGui::SmallButton(ICON_FA_PLUS))
    {
        breakpoints_.push_back(0);
        cpu.setBreakpoint(0);
    }

    if (ImGui::BeginListBox("Breakpoint"))
    {
        for (int id = breakpoints_.size()-1; id >= 0; --id)
        {
            GbcEmulator::Word& bp = breakpoints_[id];
            ImGui::PushID(id);
            GbcEmulator::Word old_value = bp;
            if (ImGui::InputScalar("", ImGuiDataType_U16, &bp, nullptr, nullptr, "%04X"))
            {
                cpu.clearBreakpoint(old_value);
                cpu.setBreakpoint(bp);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_FA_TRASH_CAN))
            {
                breakpoints_.erase(breakpoints_.begin()+id);
                if (std::find(breakpoints_.cbegin(), breakpoints_.cend(), bp) == breakpoints_.cend())
                    cpu.clearBreakpoint(bp);
            }
            ImGui::PopID();
        }
        
        ImGui::EndListBox();
    }

    const char* mode_str;
    if (is_emulator_running)
    {
        switch (cpu_state.mode)
        {
        case CpuState::Mode::Normal: mode_str = "Normal"; break;
        case CpuState::Mode::Stopped: mode_str = "Stopped"; break;
        case CpuState::Mode::Halted: mode_str = "Halted"; break;
        default: mode_str = "Unknown mode"; break;
        }
    }
    else
        mode_str = "Paused";

    ImGui::Text("State: %s", mode_str);
    ImGui::Text("Ran for %llu (T-cycles)", gb.getCpu().getClock().get());
}

void CpuWindow::drawCpuState()
{
    auto& gb = application_->getEmulator();
    auto& cpu_state = const_cast<CpuState&>(gb.getCpu().getState());

    ImGui::TextUnformatted("Registers");
    {
        ImGui::PushItemWidth(36);
        uint16_t pc_buffer_ = cpu_state[Reg16::PC];
        imgui_InputRegister16("PC", &pc_buffer_);
        cpu_state[Reg16::PC] = pc_buffer_;
        ImGui::PushItemWidth(22);
        imgui_InputRegister8("A", &cpu_state[Reg8::A]); ImGui::SameLine();
        imgui_InputRegister8("F", &cpu_state[Reg8::F]);
        imgui_InputRegister8("B", &cpu_state[Reg8::B]); ImGui::SameLine();
        imgui_InputRegister8("C", &cpu_state[Reg8::C]);
        imgui_InputRegister8("D", &cpu_state[Reg8::D]); ImGui::SameLine();
        imgui_InputRegister8("E", &cpu_state[Reg8::E]);
        imgui_InputRegister8("H", &cpu_state[Reg8::H]); ImGui::SameLine();
        imgui_InputRegister8("L", &cpu_state[Reg8::L]);
        ImGui::PopItemWidth();
        uint16_t sp_buffer_ = cpu_state[Reg16::SP];
        imgui_InputRegister16("SP", &sp_buffer_);
        cpu_state[Reg16::SP] = sp_buffer_;
        ImGui::PopItemWidth();
    }

    ImGui::TextUnformatted("Flags");
    { 
        bool c= cpu_state[RegFlag::C],
            h = cpu_state[RegFlag::H],
            n = cpu_state[RegFlag::N],
            z = cpu_state[RegFlag::Z];
        ImGui::Checkbox("c", &c); ImGui::SameLine();
        ImGui::Checkbox("h", &h); ImGui::SameLine();
        ImGui::Checkbox("n", &n); ImGui::SameLine();
        ImGui::Checkbox("z", &z);
        cpu_state[RegFlag::C] = c;
        cpu_state[RegFlag::H] = h;
        cpu_state[RegFlag::N] = n;
        cpu_state[RegFlag::Z] = z;
    }
}

void CpuWindow::drawInterrupts()
{
    auto& gb = application_->getEmulator();
    auto& interrupts = gb.getInterrupt();
    interrupts.catchUp();

    auto& all_interrupts = interrupts.getAllInts();

    ImGui::Text("IF: %02X", interrupts.getIf());
    ImGui::Text("Vblank: %llu", all_interrupts[static_cast<std::underlying_type<InterruptType>::type>(InterruptType::VBlank)]);
    ImGui::Text("LCD:    %llu", all_interrupts[static_cast<std::underlying_type<InterruptType>::type>(InterruptType::LCD)]);
    ImGui::Text("Timer:  %llu", all_interrupts[static_cast<std::underlying_type<InterruptType>::type>(InterruptType::Timer)]);
    ImGui::Text("Serial: %llu", all_interrupts[static_cast<std::underlying_type<InterruptType>::type>(InterruptType::Serial)]);
}

void CpuWindow::drawTimer()
{
    auto& gb = application_->getEmulator();
    auto& timer = gb.getTimer();

    ImGui::Text("DIV:  %02X", timer.getDiv());
    ImGui::Text("TIMA: %02X", timer.getTima());
    ImGui::Text("TMA:  %02X", timer.getTma());
    Byte tac = timer.getTac();
    ImGui::Text("TAC:  %02X", tac);
    ImGui::TextUnformatted(tac & 0x4 ? "Enabled: True" : "Enabled: False");
    ImGui::Text("Speed: %u", tac & 0x3);
}

void CpuWindow::drawSerialOutput()
{
    auto& gb = application_->getEmulator();

    const auto& serial_buffer = gb.getSerial().getSerialBuffer();
    std::string serial_str{serial_buffer.cbegin(), serial_buffer.cend()};

    ImGui::TextUnformatted("Output");

    ImGui::BeginChild("SerialOutput", ImVec2(0, 0),
                    ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextWrapped("%s", serial_str.c_str());
    ImGui::EndChild();
}

void CpuWindow::draw()
{
    if (!ImGui::Begin(getName(), &is_opened_))
    {
        ImGui::End();
        return;
    }

    drawDebugControl();

    ImGui::BeginTabBar("DebugTabBar");
    if (ImGui::BeginTabItem("CPU"))
    {
        drawCpuState();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Interrupts"))
    {
        drawInterrupts();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Timer"))
    {
        drawTimer();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Serial"))
    {
        drawSerialOutput();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();

    ImGui::End();
}

REGISTER_WINDOW(CpuWindow)
