#include "course/CourseUi.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <imgui.h>

#include "ui/Components.h"
#include "ui/Icons.h"

namespace gui_dev::course {
namespace {

int CountLines(const char* text) {
    int count = 1;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '\n') {
            ++count;
        }
    }
    return count;
}

} // namespace

void Section(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.89f, 0.11f, 0.15f, 1.0f));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void Note(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.64f, 0.68f, 1.0f));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void Code(const char* lines) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    const float height =
        ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(CountLines(lines)) + 8.0f;
    ImGui::BeginChild(lines, ImVec2(0.0f, height), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.85f, 0.72f, 1.0f));
    ImGui::TextUnformatted(lines);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void Bullet(const char* fmt, ...) {
    ImGui::Bullet();
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

void KeyValueW(float key_width, const char* key, const char* fmt, ...) {
    ImGui::TextUnformatted(key);
    ImGui::SameLine(key_width);
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.93f, 0.96f, 1.0f));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void KeyValue(const char* key, const char* fmt, ...) {
    ImGui::TextUnformatted(key);
    ImGui::SameLine(230.0f);
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.93f, 0.96f, 1.0f));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

const std::vector<std::pair<const char*, const char*>>& StandardFooter() {
    static std::vector<std::pair<const char*, const char*>> hints;
    hints.clear();
    // Icons::Glyph 返回轮转缓冲里的指针，所以每帧重建（容量复用，无堆分配）
    hints.emplace_back(Icons::Glyph(Icons::Button::L), "上一课");
    hints.emplace_back(Icons::Glyph(Icons::Button::R), "下一课");
    hints.emplace_back(Icons::Glyph(Icons::Button::B), "退出");
    return hints;
}

void EndLesson(UiContext& ui) { Components::EndPanel(ui, StandardFooter()); }

} // namespace gui_dev::course
