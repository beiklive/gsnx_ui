// Button：按钮组件。悬停/按下/禁用/焦点四态 + 平滑过渡 + 图标 + 焦点环。
#pragma once

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Button : public Widget {
public:
    Button();
    explicit Button(std::string text);

    std::string text;
    std::string icon;      // 图标字形（Icons::Glyph / Material 图标字符），可为空
    float font_size = 0.0f;
    float icon_size = 0.0f; // 0 = font_size * 1.15
    float icon_gap = 8.0f;

    // 四态颜色
    ImU32 color_normal = Theme::kButton;
    ImU32 color_hover = Theme::kAccentHover;
    ImU32 color_pressed = Theme::kButtonActive;
    ImU32 color_disabled = Theme::kBgWidget;
    ImU32 text_color = Theme::kTextBright;
    ImU32 text_color_disabled = Theme::kTextDisabled;

    // 焦点环
    bool show_focus_ring = true;
    float focus_ring_width = 2.0f;
    float focus_ring_offset = 4.0f;
    ImU32 focus_ring_color = Theme::kAccent;

    float transition_speed = 16.0f; // 状态过渡速度（越大越快）

    // ---- 预设 --------------------------------------------------------------
    Button& Primary();   // VSCode 蓝底主按钮
    Button& Secondary(); // 灰底次按钮
    Button& Ghost();     // 透明底 + 描边
    Button& Danger();    // 红底危险操作

    Button& SetText(std::string value);
    Button& SetIcon(std::string glyph);
    Button& SetFontSize(float value);
    Button& SetIconGap(float value);
    Button& FitContent(float horizontal_padding = 18.0f, float height = 44.0f);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;

private:
    float hover_mix_ = 0.0f; // 0 = normal，1 = hover
    float press_mix_ = 0.0f; // 0 = 未按下，1 = 按下
    float ResolvedIconSize() const { return icon_size > 0.0f ? icon_size : ResolvedFontSize() * 1.15f; }
    float ResolvedFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }
    ImVec2 ContentExtent() const;
};

} // namespace gui_dev::cv
