#include "component_view/components/Button.h"

#include <cmath>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

// 指数平滑：target 为 0/1 时不会过冲，且与帧率无关。
float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

Button::Button() : Widget("button") {
    focusable = true;
    focus_on_hover = true;
    Primary();
}

Button::Button(std::string value) : Widget("button") {
    text = std::move(value);
    focusable = true;
    focus_on_hover = true;
    Primary();
}

Button& Button::Primary() {
    color_normal = Theme::kButton;
    color_hover = Theme::kAccentHover;
    color_pressed = Theme::kButtonActive;
    color_disabled = Theme::kBgWidget;
    text_color = Theme::kTextBright;
    text_color_disabled = Theme::kTextDisabled;
    background = color_normal;
    border = BorderStyle{0.0f, 0, 0.0f};
    corner_radius = Theme::kRadiusSmall;
    padding = EdgeInsets::Symmetric(18.0f, 10.0f);
    return *this;
}

Button& Button::Secondary() {
    color_normal = Theme::kBgWidget;
    color_hover = Theme::kBgWidgetHi;
    color_pressed = Theme::kBgInput;
    color_disabled = Theme::kBgWidget;
    text_color = Theme::kTextPrimary;
    text_color_disabled = Theme::kTextDisabled;
    background = color_normal;
    border = BorderStyle{1.0f, Theme::kBorder, 0.0f};
    corner_radius = Theme::kRadiusSmall;
    padding = EdgeInsets::Symmetric(18.0f, 10.0f);
    return *this;
}

Button& Button::Ghost() {
    color_normal = 0;
    color_hover = Theme::Alpha(Theme::kTextPrimary, 0.10f);
    color_pressed = Theme::Alpha(Theme::kTextPrimary, 0.18f);
    color_disabled = 0;
    text_color = Theme::kTextPrimary;
    text_color_disabled = Theme::kTextDisabled;
    background = color_normal;
    border = BorderStyle{1.0f, Theme::kBorder, 0.0f};
    corner_radius = Theme::kRadiusSmall;
    padding = EdgeInsets::Symmetric(18.0f, 10.0f);
    return *this;
}

Button& Button::Danger() {
    color_normal = Theme::kError;
    color_hover = Theme::Mix(Theme::kError, IM_COL32(0xFF, 0xFF, 0xFF, 0xFF), 0.18f);
    color_pressed = Theme::Mix(Theme::kError, IM_COL32(0x00, 0x00, 0x00, 0xFF), 0.25f);
    color_disabled = Theme::kBgWidget;
    text_color = Theme::kTextBright;
    text_color_disabled = Theme::kTextDisabled;
    background = color_normal;
    border = BorderStyle{0.0f, 0, 0.0f};
    corner_radius = Theme::kRadiusSmall;
    padding = EdgeInsets::Symmetric(18.0f, 10.0f);
    return *this;
}

Button& Button::SetText(std::string value) {
    text = std::move(value);
    return *this;
}

Button& Button::SetIcon(std::string glyph) {
    icon = std::move(glyph);
    return *this;
}

Button& Button::SetFontSize(float value) {
    font_size = value;
    return *this;
}

Button& Button::SetIconGap(float value) {
    icon_gap = value;
    return *this;
}

Button& Button::FitContent(float horizontal_padding, float height) {
    padding = EdgeInsets::Symmetric(horizontal_padding, padding.top);
    size.y = height;
    return *this;
}

ImVec2 Button::ContentExtent() const {
    const float size = ResolvedFontSize();
    ImVec2 extent = Draw::MeasureText(nullptr, size, text.c_str(), 0.0f);
    if (!icon.empty()) {
        const ImVec2 icon_extent = Draw::MeasureText(nullptr, ResolvedIconSize(), icon.c_str(), 0.0f);
        extent.x += icon_gap + icon_extent.x;
        extent.y = Maxf(extent.y, icon_extent.y);
    }
    return extent;
}

ImVec2 Button::MeasureContent(const ImVec2& available) {
    (void)available;
    return ContentExtent();
}

void Button::OnUpdate(float dt) {
    // 鼠标悬停或手柄焦点都算「热」状态，只是焦点态更弱一些。
    float hover_target = 0.0f;
    if (enabled) {
        hover_target = hovered ? 1.0f : (focused ? 0.35f : 0.0f);
    }
    hover_mix_ = SmoothTo(hover_mix_, hover_target, transition_speed, dt);
    press_mix_ = SmoothTo(press_mix_, (pressed && enabled) ? 1.0f : 0.0f, transition_speed * 1.4f, dt);

    if (!enabled) {
        background = color_disabled;
        return;
    }
    ImU32 fill = Theme::Mix(color_normal, color_hover, hover_mix_);
    fill = Theme::Mix(fill, color_pressed, press_mix_);
    background = fill;
}

void Button::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float size = ResolvedFontSize();
    const ImVec2 text_extent = Draw::MeasureText(nullptr, size, text.c_str(), 0.0f);
    const bool has_icon = !icon.empty();
    const float icon_size = ResolvedIconSize();
    const ImVec2 icon_extent = has_icon ? Draw::MeasureText(nullptr, icon_size, icon.c_str(), 0.0f) : ImVec2(0.0f, 0.0f);
    const float total_width = text_extent.x + (has_icon ? icon_gap + icon_extent.x : 0.0f);

    float cursor_x = content.Center().x - total_width * 0.5f;
    const ImU32 foreground = Tint(enabled ? text_color : text_color_disabled);
    if (has_icon) {
        Draw::Text(dl, nullptr, icon_size, ImVec2(cursor_x, content.Center().y - icon_extent.y * 0.5f), foreground,
                   icon.c_str());
        cursor_x += icon_extent.x + icon_gap;
    }
    Draw::Text(dl, nullptr, size, ImVec2(cursor_x, content.Center().y - text_extent.y * 0.5f), foreground, text.c_str());
}

void Button::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    (void)content;
    if (!show_focus_ring || !focused || !enabled) {
        return;
    }
    const Rect ring = rect.Expanded(focus_ring_offset);
    Draw::RoundedRectOutline(dl, ring, Tint(focus_ring_color), focus_ring_width, CornerTL() + focus_ring_offset,
                             CornerTR() + focus_ring_offset, CornerBL() + focus_ring_offset,
                             CornerBR() + focus_ring_offset);
}

} // namespace gui_dev::cv
