#include "memory_window.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_memory_editor.h>

#include <gameboy.hpp>

#include "application.hpp"
#include "font/fa_icons.h"

using namespace GbcEmulator;

static constexpr size_t instruction_length[256] = {
    1,3,1,1, 1,1,2,1, 3,1,1,1, 1,1,2,1,
    2,3,1,1, 1,1,2,1, 2,1,1,1, 1,1,2,1,
    2,3,1,1, 1,1,2,1, 2,1,1,1, 1,1,2,1,
    2,3,1,1, 1,1,2,1, 2,1,1,1, 1,1,2,1,

    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,

    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,

    1,1,3,3, 3,1,2,1, 1,1,3,2, 3,3,2,1,
    1,1,3,1, 3,1,2,1, 1,1,3,1, 3,1,2,1,
    2,1,1,1, 1,1,2,1, 2,1,3,1, 1,1,2,1,
    2,1,1,1, 1,1,2,1, 2,1,3,1, 1,1,2,1 
};

static ImU8 readGameBoyMemory([[maybe_unused]] const ImU8* mem, size_t offset, void* user_data)
{
    auto& mmu = static_cast<GameBoy*>(user_data)->getMmu();
    return mmu.load(static_cast<Word>(offset));
}

static ImU32 colorGameBoyMemory([[maybe_unused]] const ImU8* mem, size_t offset, void* user_data)
{
    const auto& cpu = static_cast<GameBoy*>(user_data)->getCpu();
    return cpu.hasBreakpoint(static_cast<Word>(offset)) ? IM_COL32(255, 0, 0, 255) : 0;
}

void MemoryWindow::draw()
{
    if (!ImGui::Begin(getName(), &is_opened_, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }

    auto& gb = application_->getEmulator();

    if (!gb.getMmu().hasCartridge())
    {
        ImGui::TextUnformatted("No cartridge loaded.");
        ImGui::End();
        return;
    }

    static MemoryEditor mem_edit;
    mem_edit.UserData = &gb;
    mem_edit.ReadFn = readGameBoyMemory;
    mem_edit.ReadOnly = true;
    mem_edit.BgColorFn = colorGameBoyMemory;

    uint16_t pc = const_cast<GbcEmulator::CpuState&>(gb.getCpu().getState())[Reg16::PC];
    mem_edit.HighlightMin = pc;
    uint8_t current_inst = gb.getMmu().load(pc);
    mem_edit.HighlightMax = mem_edit.HighlightMin + instruction_length[current_inst];
    
    mem_edit.DrawContents(nullptr, gb.getMmu().hasCartridge() ? 0x10000 : 0);

    ImGui::End();
}

REGISTER_WINDOW(MemoryWindow)
