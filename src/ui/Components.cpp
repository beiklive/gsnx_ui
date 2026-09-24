#include "ui/Components.h"

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::Components {
namespace {

// 统一的选中/悬浮色堆栈，避免每个组件各写一套。
void PushSelectedStyle() {
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(Theme::kAccentDim));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(Theme::kAccentDim));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(Theme::kAccent));
}

void PopSelectedStyle() { ImGui::PopStyleColor(3); }

// 根画布：铺满整个屏幕，无标题栏/边框/圆角/阴影/拖动/缩放。
// 这是「直接画在屏幕上」而不是「浮在屏幕上的一扇窗」——所有页面都画在它里面，
// 因此不需要也没有 ImGui 的窗口外壳。
constexpr ImGuiWindowFlags kRootWindowFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

// 设 GUI_DEV_DEBUG_LAYOUT=1 时打印根画布矩形。Switch 上拿不到截图，
// 只能靠这些数字核对布局是否真的铺满屏幕。
void DebugLogRootCanvas() {
    static const bool enabled = std::getenv("GUI_DEV_DEBUG_LAYOUT") != nullptr;
    if (!enabled) {
        return;
    }
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    static ImVec2 last_pos(-1.0f, -1.0f);
    static ImVec2 last_size(-1.0f, -1.0f);
    if (pos.x == last_pos.x && pos.y == last_pos.y && size.x == last_size.x && size.y == last_size.y) {
        return;
    }
    last_pos = pos;
    last_size = size;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    std::fprintf(stderr, "[gui_dev] root canvas pos=(%.0f,%.0f) size=(%.0f,%.0f) display=(%.0f,%.0f)\n",
                 static_cast<double>(pos.x), static_cast<double>(pos.y), static_cast<double>(size.x),
                 static_cast<double>(size.y), static_cast<double>(display.x),
                 static_cast<double>(display.y));
}

} // namespace

bool BeginPanel(UiContext& ui, const char* title, const char* subtitle) {
    (void)ui;
    // 尺寸必须每帧显式指定：不指定的话 ImGui 会按内容自动撑成一个浮窗。
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const std::string root_id = std::string("##panel_") + (title ? title : "root");
    const bool open = ImGui::Begin(root_id.c_str(), nullptr, kRootWindowFlags);
    ImGui::PopStyleVar(3);
    DebugLogRootCanvas();

    // ---- 页头（顶到画布最上方，因此用 Group 压掉窗口 padding）----------------
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
    ImGui::BeginChild("##panel_header", ImVec2(0.0f, Theme::kHeaderHeight), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(Theme::kGapLarge, Theme::kGap * 0.5f));
    ImGui::BeginGroup();
    ImGui::TextUnformatted(title ? title : "");
    if (subtitle && subtitle[0] != '\0') {
        ImGui::TextDisabled("%s", subtitle);
    }
    ImGui::EndGroup();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Separator();

    // ---- 内容区（自动滚动）：负高度 = 剩余空间 - 页脚预留 --------------------
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Theme::kGapLarge, Theme::kGapLarge));
    ImGui::BeginChild("##panel_body", ImVec2(0.0f, -(Theme::kGap + Theme::kFooterHeight)),
                      ImGuiChildFlags_None, ImGuiWindowFlags_None);
    return open;
}

void EndPanel(UiContext& ui, const std::vector<std::pair<const char*, const char*>>& footer_hints) {
    (void)ui;
    ImGui::EndChild();    // panel_body
    ImGui::PopStyleVar(); // panel_body padding
    ImGui::EndGroup();

    // 页脚在画布内绝对定位到最底部。必须在这里（仍处于画布内）绘制：
    // 如果在 ImGui::End() 之后再画，会落到 ImGui 的隐藏 fallback 窗口上，
    // 表现为屏幕角落多出一块浮动的方块。
    const float footer_y = ImGui::GetWindowSize().y - Theme::kFooterHeight;
    ImGui::SetCursorPos(ImVec2(0.0f, footer_y > 0.0f ? footer_y : 0.0f));
    ImGui::BeginChild("##panel_footer", ImVec2(0.0f, Theme::kFooterHeight), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
    // key 通常是 Icons::Glyph(...) 字形，也可以是 "F11" 这类文本按键。
    ImGui::SetCursorPos(ImVec2(Theme::kGapLarge,
                               (Theme::kFooterHeight - ImGui::GetTextLineHeight()) * 0.5f));
    for (std::size_t i = 0; i < footer_hints.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, Theme::kGapLarge);
        }
        ImGui::TextColored(Theme::ToVec4(Theme::kAccent), "%s",
                           footer_hints[i].first ? footer_hints[i].first : "");
        ImGui::SameLine(0.0f, Theme::kGapSmall);
        ImGui::TextDisabled("%s", footer_hints[i].second ? footer_hints[i].second : "");
    }
    ImGui::EndChild();

    ImGui::End();
}

void SectionHeader(UiContext& ui, const char* label) {
    (void)ui;
    ImGui::Spacing();
    ImGui::TextColored(Theme::ToVec4(Theme::kAccent), "%s", label ? label : "");
    ImGui::Separator();
}

void LabeledRow(UiContext& ui, const char* label, const char* value) {
    (void)ui;
    ImGui::TextUnformatted(label ? label : "");
    ImGui::SameLine();
    if (value && value[0] != '\0') {
        const float w = ImGui::CalcTextSize(value).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             (ImGui::GetContentRegionAvail().x - w > 0.0f ? ImGui::GetContentRegionAvail().x - w : 0.0f));
    }
    ImGui::TextDisabled("%s", (value && value[0] != '\0') ? value : "-");
}

bool ToggleRow(UiContext& ui, const char* label, bool* value, const char* hint) {
    (void)ui;
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox("##toggle", value);
    ImGui::SameLine();
    ImGui::TextUnformatted(label ? label : "");
    if (hint && hint[0] != '\0') {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", hint);
    }
    ImGui::PopID();
    return changed;
}

int SelectableList(UiContext& ui, const char* id, const std::vector<std::pair<std::string, std::string>>& items,
                   int selected, float item_height) {
    (void)ui;
    int clicked = -1;
    const float h = item_height > 0.0f ? item_height : (ImGui::GetTextLineHeightWithSpacing() * 2.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, Theme::kGapSmall));
    ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const bool is_selected = (i == selected);
        ImGui::PushID(i);
        if (is_selected) {
            PushSelectedStyle();
        }
        // 文字画在 Selectable 内部：先占位，再在同一行区域覆盖绘制，
        // 这样选中/悬浮背景天然对齐，且不需要手工算行高。
        const bool hit = ImGui::Selectable("##row", is_selected, ImGuiSelectableFlags_AllowDoubleClick,
                                           ImVec2(0.0f, h));
        const ImVec2 text_pos = ImGui::GetItemRectMin();
        const ImVec2 text_size = ImGui::GetItemRectSize();
        const bool hovered = ImGui::IsItemHovered();
        if (is_selected) {
            PopSelectedStyle();
        }
        ImGui::PopID();
        if (hit) {
            clicked = i;
        }

        ImGui::SetCursorScreenPos(ImVec2(text_pos.x + Theme::kGap,
                                         text_pos.y + (text_size.y - ImGui::GetTextLineHeightWithSpacing() * 2.0f) * 0.5f));
        ImGui::BeginGroup();
        ImGui::TextUnformatted(items[i].first.c_str());
        if (items[i].second.empty()) {
            ImGui::TextDisabled(" ");
        } else {
            ImGui::TextDisabled("%s", items[i].second.c_str());
        }
        ImGui::EndGroup();
        if (hovered && !is_selected) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                text_pos, ImVec2(text_pos.x + text_size.x, text_pos.y + text_size.y),
                IM_COL32(0xFF, 0xFF, 0xFF, 5), Theme::kPanelRounding);
        }
        ImGui::SetCursorScreenPos(ImVec2(text_pos.x, text_pos.y + text_size.y));
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    return clicked;
}

int Toolbar(UiContext& ui, const char* id, const std::vector<const char*>& labels, bool compact) {
    (void)ui;
    int clicked = -1;
    const ImVec2 pad = ImGui::GetStyle().FramePadding;
    const float h = compact ? (ImGui::GetTextLineHeight() + pad.y * 2.0f) : Theme::kRowHeight;

    ImGui::PushID(id);
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        ImGui::PushID(i);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Theme::kPanelRounding);
        if (ImGui::Button(labels[i], ImVec2(0.0f, h))) {
            clicked = i;
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
        if (i + 1 < static_cast<int>(labels.size())) {
            ImGui::SameLine();
        }
    }
    ImGui::PopID();
    return clicked;
}

void ProgressRow(UiContext& ui, const char* label, float progress01, const char* detail) {
    (void)ui;
    const float p = progress01 < 0.0f ? 0.0f : (progress01 > 1.0f ? 1.0f : progress01);
    ImGui::TextUnformatted(label ? label : "");
    ImGui::SameLine();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%3.0f%%", static_cast<double>(p * 100.0f));
    ImGui::TextDisabled("%s", buf);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::ToVec4(Theme::kAccent));
    ImGui::ProgressBar(p, ImVec2(-1.0f, Theme::kGap), detail ? detail : "");
    ImGui::PopStyleColor();
}

void EmptyState(UiContext& ui, const char* message, const char* hint) {
    (void)ui;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float block = ImGui::GetTextLineHeightWithSpacing() * (hint && hint[0] != '\0' ? 2.0f : 1.0f);
    const float y = avail.y > block ? (avail.y - block) * 0.5f : 0.0f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y);
    const float w = ImGui::CalcTextSize(message ? message : "").x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - w > 0.0f ? (avail.x - w) * 0.5f : 0.0f));
    ImGui::TextDisabled("%s", message ? message : "");
    if (hint && hint[0] != '\0') {
        const float hw = ImGui::CalcTextSize(hint).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hw > 0.0f ? (avail.x - hw) * 0.5f : 0.0f));
        ImGui::TextDisabled("%s", hint);
    }
}

bool ConfirmModal(UiContext& ui, const char* id, const char* title, const char* message) {
    (void)ui;
    bool result = false;
    bool open = true;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::BeginPopupModal(id, &open, flags)) {
        ImGui::TextUnformatted(title ? title : "");
        ImGui::Separator();
        if (message && message[0] != '\0') {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 360.0f);
            ImGui::TextUnformatted(message);
            ImGui::PopTextWrapPos();
        }
        ImGui::Spacing();
        const float button_w = 120.0f;
        if (ImGui::Button("确定", ImVec2(button_w, Theme::kRowHeight))) {
            result = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(button_w, Theme::kRowHeight))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return result;
}

void StatusBanner(UiContext& ui, const char* message, bool is_error) {
    (void)ui;
    const ImU32 color = is_error ? Theme::kDanger : Theme::kSuccess;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::ToVec4(is_error ? IM_COL32(0x3A, 0x1E, 0x1E, 0xFF)
                                                                  : IM_COL32(0x16, 0x2C, 0x22, 0xFF)));
    ImGui::BeginChild(message ? message : "##banner", ImVec2(0.0f, Theme::kFooterHeight), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(Theme::kGap, (Theme::kFooterHeight - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::TextColored(Theme::ToVec4(color), "%s", message ? message : "");
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace gui_dev::Components
