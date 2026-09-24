#include "ui/Components.h"

#include <cmath>
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

// --------------------------------------------------------------- 可聚焦方框 ----

namespace {

// 流光路径按等弧长采样，保证每个小段都能独立插值透明度/UV。
// 段长越大，光斑的渐变台阶越明显，8px 在 720p~1080p 下已经看不出来。
constexpr float kFlowSegmentLength = 8.0f;
constexpr int kCornerSegments = 8;
// IM_PI/ImCos 都在 imgui_internal.h 里，组件层只用公开头文件，所以自带常量。
constexpr float kPi = 3.14159265358979323846f;

// 顺时针采样的圆角矩形轮廓。
void BuildRoundedRectOutline(const ImVec2& mn, const ImVec2& mx, float rounding,
                             ImVector<ImVec2>& out) {
    out.clear();
    const float max_rounding = (mx.x - mn.x) * 0.5f < (mx.y - mn.y) * 0.5f ? (mx.x - mn.x) * 0.5f
                                                                         : (mx.y - mn.y) * 0.5f;
    float r = rounding;
    if (r > max_rounding) {
        r = max_rounding;
    }
    if (r <= 0.0f) {
        out.push_back(ImVec2(mn.x, mn.y));
        out.push_back(ImVec2(mx.x, mn.y));
        out.push_back(ImVec2(mx.x, mx.y));
        out.push_back(ImVec2(mn.x, mx.y));
        return;
    }

    // 屏幕坐标 y 轴向下：左上角从 180° 转到 270°，四个角依次衔接。
    const ImVec2 centers[4] = {
        ImVec2(mn.x + r, mn.y + r),
        ImVec2(mx.x - r, mn.y + r),
        ImVec2(mx.x - r, mx.y - r),
        ImVec2(mn.x + r, mx.y - r),
    };
    const float start_angle[4] = {kPi, kPi * 1.5f, 0.0f, kPi * 0.5f};
    for (int corner = 0; corner < 4; ++corner) {
        for (int s = 0; s <= kCornerSegments; ++s) {
            const float a = start_angle[corner] +
                            (kPi * 0.5f) * static_cast<float>(s) / static_cast<float>(kCornerSegments);
            out.push_back(ImVec2(centers[corner].x + std::cos(a) * r, centers[corner].y + std::sin(a) * r));
        }
    }
}

// 把轮廓重采样成等弧长闭合路径；total 为周长。
void ResampleClosed(const ImVector<ImVec2>& outline, float step, ImVector<ImVec2>& out, float& total) {
    out.clear();
    total = 0.0f;
    const int count = outline.Size;
    if (count < 2) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        const ImVec2& a = outline[i];
        const ImVec2& b = outline[(i + 1) % count];
        total += std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
    }
    if (total <= 1.0f) {
        return;
    }

    int target = static_cast<int>(total / step);
    if (target < 8) {
        target = 8;
    }
    if (target > 512) {
        target = 512;
    }
    const float spacing = total / static_cast<float>(target);

    // 沿折线按固定间距取点。
    int emitted = 0;
    float segment_start = 0.0f;
    for (int i = 0; i < count && emitted < target; ++i) {
        const ImVec2& a = outline[i];
        const ImVec2& b = outline[(i + 1) % count];
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.0001f) {
            continue;
        }
        const float segment_end = segment_start + len;
        while (emitted < target && static_cast<float>(emitted) * spacing < segment_end) {
            const float s = static_cast<float>(emitted) * spacing;
            const float t = (s - segment_start) / len;
            out.push_back(ImVec2(a.x + dx * t, a.y + dy * t));
            ++emitted;
        }
        segment_start = segment_end;
    }
}

// 沿路径画一条带 UV 的小段（四边形）。col 乘到贴图上。
void AddBand(ImDrawList* draw_list, const ImVec2& p0, const ImVec2& p1, const ImVec2& n,
             float half_width, float u0, float u1, ImTextureRef texture, ImU32 col) {
    const ImVec2 outer0(p0.x + n.x * half_width, p0.y + n.y * half_width);
    const ImVec2 outer1(p1.x + n.x * half_width, p1.y + n.y * half_width);
    const ImVec2 inner0(p0.x - n.x * half_width, p0.y - n.y * half_width);
    const ImVec2 inner1(p1.x - n.x * half_width, p1.y - n.y * half_width);
    draw_list->AddImageQuad(texture, outer0, outer1, inner1, inner0, ImVec2(u0, 0.0f), ImVec2(u1, 0.0f),
                            ImVec2(u1, 1.0f), ImVec2(u0, 1.0f), col);
}

inline float WrapCentered(float x) {
    // 映射到 [-0.5, 0.5)，用于算光斑的余弦包络。
    return x - std::floor(x + 0.5f);
}

// 流光边框：贴图 UV 沿周长滚动 + 一个绕框跑的光斑。
void DrawFlowBorder(ImDrawList* draw_list, const ImVec2& mn, const ImVec2& mx, const BoxStyle& style) {
    static ImVector<ImVec2> outline;
    static ImVector<ImVec2> path;
    // 路径直接落在填充矩形的边界上（与 AddRectFilled 同一套 rect/rounding）。
    // 若按 border_width/2 内缩又同步改小 rounding，角落处会与填充之间露出缝隙。
    BuildRoundedRectOutline(mn, mx, style.rounding, outline);

    float total = 0.0f;
    ResampleClosed(outline, kFlowSegmentLength, path, total);
    const int count = path.Size;
    if (count < 4 || total <= 1.0f) {
        draw_list->AddRect(mn, mx, style.focus_fallback_color, style.rounding, 0, style.border_width);
        return;
    }

    const float phase = static_cast<float>(std::fmod(ImGui::GetTime() * style.flow_speed, 1.0));
    const float half_core = style.border_width * 0.5f;
    const float half_glow = (style.border_width + style.glow_width) * 0.5f;

    float arc = 0.0f;
    for (int i = 0; i < count; ++i) {
        const ImVec2& p0 = path[i];
        const ImVec2& p1 = path[(i + 1) % count];
        const float dx = p1.x - p0.x;
        const float dy = p1.y - p0.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len <= 0.0001f) {
            continue;
        }
        // 顺时针路径的外法线
        const ImVec2 n(dy / len, -dx / len);

        const float t0 = arc / total;
        const float t1 = (arc + len) / total;
        const float u0 = t0 * style.flow_cycles + phase;
        const float u1 = t1 * style.flow_cycles + phase;

        // 光斑：余弦包络，峰在 phase 处（与 UV 用同一个相位，视觉上是同一道光）。
        const float mid = (t0 + t1) * 0.5f;
        const float rel = WrapCentered(mid - phase);
        const float lobe = std::cos(rel * 2.0f * kPi);
        const float lobe3 = lobe > 0.0f ? lobe * lobe * lobe : 0.0f;
        const float alpha = style.flow_dim_alpha + (style.flow_peak_alpha - style.flow_dim_alpha) * lobe3;

        // 外发光（宽、淡）+ 本体（细、亮）
        AddBand(draw_list, p0, p1, n, half_glow, u0, u1, style.flow_texture,
                IM_COL32(255, 255, 255, static_cast<int>(alpha * 60.0f)));
        AddBand(draw_list, p0, p1, n, half_core, u0, u1, style.flow_texture,
                IM_COL32(255, 255, 255, static_cast<int>(alpha * 255.0f)));

        arc += len;
    }
}

} // namespace

BoxResult FocusableBox(const char* id, bool focused, const BoxStyle& style,
                       const BoxContentFn& draw_content) {
    BoxResult result;

    ImGui::PushID(id);

    ImVec2 box_size = style.size;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (box_size.x <= 0.0f) {
        box_size.x = available.x;
    }
    if (box_size.y <= 0.0f) {
        box_size.y = style.height;
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 box_max(origin.x + box_size.x, origin.y + box_size.y);

    result.clicked = ImGui::InvisibleButton("##hit", box_size, ImGuiButtonFlags_MouseButtonLeft);
    result.hovered = ImGui::IsItemHovered();
    // InvisibleButton 已经把布局光标推到框下方，内容要画回框内再复位。
    const ImVec2 after_box = ImGui::GetCursorScreenPos();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(origin, box_max, style.fill_color, style.rounding);

    if (focused) {
        if (style.flow_texture.GetTexID() != ImTextureID_Invalid) {
            DrawFlowBorder(draw_list, origin, box_max, style);
        } else {
            draw_list->AddRect(origin, box_max, style.focus_fallback_color, style.rounding, 0,
                               style.border_width);
        }
    } else {
        draw_list->AddRect(origin, box_max,
                           result.hovered ? style.hover_border_color : style.idle_border_color,
                           style.rounding, 0, 1.5f);
    }

    if (draw_content) {
        ImGui::SetCursorScreenPos(ImVec2(origin.x + style.padding, origin.y + style.padding));
        ImGui::BeginGroup();
        draw_content(ImVec2(box_size.x - style.padding * 2.0f, box_size.y - style.padding * 2.0f));
        ImGui::EndGroup();
    }

    // 光标复位到框下方。1.92 起 SetCursorPos 后必须紧跟一个 item，
    // 否则会触发 ErrorCheckUsingSetCursorPosToExtendParentBoundaries 断言。
    ImGui::SetCursorScreenPos(after_box);
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::PopID();
    return result;
}

} // namespace gui_dev::Components
