#include "ppu_window.hpp"

#include <glad/glad.h>
#include <imgui/imgui.h>

#include "application.hpp"
#include "gameboy.hpp"

using namespace GbcEmulator;

PpuWindow::PpuWindow(Application* app)
: application_{app}
{
    glGenTextures(1, &video_memory_texture_);
    glBindTexture(GL_TEXTURE_2D, video_memory_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void PpuWindow::drawTileset()
{
    auto& gb = application_->getEmulator();
    auto& mmu = gb.getMmu();

    static constexpr size_t tile_per_row = 16;
    static constexpr uint16_t palette[4] = { 0x0001, 0x5295, 0xAD6B, 0xFFFF };

    std::array<uint16_t, 64*128*3> texture_data;
    texture_data.fill(0xF0FF);
    for (unsigned int tile_index = 0; tile_index < 128*3; ++tile_index)
    {
        unsigned int tile_x = tile_index % tile_per_row;
        unsigned int tile_y = tile_index / tile_per_row;
        for (unsigned int tile_line = 0; tile_line < 8; ++tile_line)
        {
            size_t offset = (tile_x + (8*tile_y + tile_line) * tile_per_row) * 8;
            Byte first_byte = mmu.load(0x8000 + 16*tile_index + 2*tile_line);
            Byte second_byte = mmu.load(0x8001 + 16*tile_index + 2*tile_line);

            texture_data[offset]     = palette[((first_byte >> 7) & 1) | ((second_byte >> 7) & 1) << 1];
            texture_data[offset + 1] = palette[((first_byte >> 6) & 1) | ((second_byte >> 6) & 1) << 1];
            texture_data[offset + 2] = palette[((first_byte >> 5) & 1) | ((second_byte >> 5) & 1) << 1];
            texture_data[offset + 3] = palette[((first_byte >> 4) & 1) | ((second_byte >> 4) & 1) << 1];
            texture_data[offset + 4] = palette[((first_byte >> 3) & 1) | ((second_byte >> 3) & 1) << 1];
            texture_data[offset + 5] = palette[((first_byte >> 2) & 1) | ((second_byte >> 2) & 1) << 1];
            texture_data[offset + 6] = palette[((first_byte >> 1) & 1) | ((second_byte >> 1) & 1) << 1];
            texture_data[offset + 7] = palette[((first_byte >> 0) & 1) | ((second_byte >> 0) & 1) << 1];
        }
    }

    glBindTexture(GL_TEXTURE_2D, video_memory_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8*tile_per_row, 8*128*3/tile_per_row, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texture_data.data());

    ImVec2 uv_min = ImVec2(0.0f, 0.0f);
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

    ImGui::Image(reinterpret_cast<ImTextureID>(video_memory_texture_), ImVec2(8*tile_per_row*2, 8*128*3/tile_per_row*2), uv_min, uv_max, tint_col, border_col);
}

void PpuWindow::drawBackground()
{
    auto& gb = application_->getEmulator();
    auto& mmu = gb.getMmu();
    auto& ppu = gb.getPpu();

    static constexpr uint16_t palette[4] = { 0x0001, 0x5295, 0xAD6B, 0xFFFF };

    bool use_alt_tileset = !(ppu.getLcdc() & 0x10);

    std::array<uint16_t, 256*256> texture_data;
    texture_data.fill(0xF0FF);
    for (unsigned int i = 0; i < 32*32; ++i)
    {
        unsigned int tile_x = i % 32;
        unsigned int tile_y = i / 32;
        
        Byte tile_index = mmu.load(0x9800 + i);
        size_t offset = tile_x * 8 + tile_y * 8 * 32 * 8;
        for (Word tile_line = 0; tile_line < 8; ++tile_line, offset += 32 * 8)
        {
            Byte first_byte = mmu.load(use_alt_tileset ?
                0x9000 + 16*static_cast<int8_t>(tile_index) + 2*tile_line :
                0x8000 + 16*tile_index + 2*tile_line);
            Byte second_byte = mmu.load(use_alt_tileset ?
                0x9001 + 16*static_cast<int8_t>(tile_index) + 2*tile_line :
                0x8001 + 16*tile_index + 2*tile_line);

            for (size_t pixel = 0; pixel < 8; ++pixel)
                texture_data[offset + pixel] =
                    palette[((first_byte >> (7-pixel)) & 1) | ((second_byte >> (7-pixel)) & 1) << 1];
        }
    }

    const uint16_t window_color = 0xF801;

    uint8_t left = ppu.getScx();
    uint8_t right = left + 159;
    uint8_t top = ppu.getScy();
    uint8_t bottom = top + 143;
    for (uint8_t x = left; x != right+1; ++x)
    {
        texture_data[top * 256u + x] = window_color;
        texture_data[bottom * 256u + x] = window_color;
    }

    for (uint8_t y = top+1; y != bottom; ++y)
    {
        texture_data[y * 256u + left] = window_color;
        texture_data[y * 256u + right] = window_color;
    }

    glBindTexture(GL_TEXTURE_2D, video_memory_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32*8, 32*8, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texture_data.data());

    ImVec2 uv_min = ImVec2(0.0f, 0.0f);
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

    ImGui::Image(reinterpret_cast<ImTextureID>(video_memory_texture_), ImVec2(32*8*2, 32*8*2), uv_min, uv_max, tint_col, border_col);
}

void PpuWindow::draw()
{
    if (!ImGui::Begin(getName(), &is_opened_))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginTabBar("DebugTabBar");
    if (ImGui::BeginTabItem("Tileset"))
    {
        drawTileset();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Background"))
    {
        drawBackground();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();

    ImGui::End();
}

REGISTER_WINDOW(PpuWindow)
