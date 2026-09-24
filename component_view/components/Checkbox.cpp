#include "component_view/components/Checkbox.h"

#include <cmath>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

// ------------------------------------------------------------- Checkbox ----

Checkbox::Checkbox() : Widget("checkbox") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = true;
    focus_frame_offset = 3.0f;
    focus_scale = 1.02f;
    padding = EdgeInsets::Symmetric(9.0f, 6.0f);
    corner_radius = Theme::kRadiusSmall;
}

Checkbox::Checkbox(std::string text, bool value) : Checkbox() {
    label = std::move(text);
    checked = value;
}

Checkbox& Checkbox::SetLabel(std::string value) {
    label = std::move(value);
    return *this;
}

Checkbox& Checkbox::SetChecked(bool value, bool notify) {
    Apply(value, notify);
    return *this;
}

Checkbox& Checkbox::Toggle(bool notify) {
    Apply(!checked, notify);
    return *this;
}

void Checkbox::Apply(bool value, bool notify) {
    if (checked == value) {
        return;
    }
    checked = value;
    if (notify) {
        emit toggled(checked);
        emit stateChanged(checked ? 2 : 0);
    }
}

ImVec2 Checkbox::MeasureContent(const ImVec2& available) {
    (void)available;
    const float size = font_size > 0.0f ? font_size : Theme::kFontBody;
    const ImVec2 text = Draw::MeasureText(nullptr, size, label.c_str(), 0.0f);
    return ImVec2(box_size + (label.empty() ? 0.0f : label_gap + text.x), Maxf(box_size, text.y));
}

void Checkbox::OnUpdate(float dt) {
    check_mix_ = SmoothTo(check_mix_, checked ? 1.0f : 0.0f, transition_speed, dt);
    hover_mix_ = SmoothTo(hover_mix_, (hovered || focused) && enabled ? 1.0f : 0.0f, transition_speed, dt);
    // 聚焦时轻微右移，给手柄焦点一个「抬起来」的感觉
    visual_translate = ImVec2(6.0f * focus_mix, 0.0f);
}

bool Checkbox::OnPadAction(InputAction action) {
    if (action == InputAction::ActionX) {
        Toggle(true);
        return true;
    }
    return false;
}

void Checkbox::Activate() {
    Toggle(true);
}

void Checkbox::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float box = box_size * scale;
    const Rect indicator = Rect::FromPosSize(ImVec2(content.min.x, content.Center().y - box * 0.5f), ImVec2(box, box));
    const float radius = 5.0f * scale;

    const ImU32 fill = Theme::Mix(box_bg, box_bg_checked, check_mix_);
    Draw::RoundedRectFilled(dl, indicator, Tint(fill), radius, radius, radius, radius);
    Draw::RoundedRectOutline(dl, indicator,
                             Theme::Alpha(Theme::Mix(box_border, box_bg_checked, check_mix_),
                                          EffectiveOpacity() * (0.7f + 0.3f * hover_mix_)),
                             1.5f * scale, radius, radius, radius, radius);
    Draw::CheckMark(dl, indicator, Tint(check_color), 2.0f * scale, check_mix_);

    if (!label.empty()) {
        const float size = (font_size > 0.0f ? font_size : Theme::kFontBody) * scale;
        const ImU32 color = focused ? label_color_focus : label_color;
        Draw::Text(dl, nullptr, size, ImVec2(indicator.max.x + label_gap * scale, content.Center().y - size * 0.5f),
                   Tint(color), label.c_str());
    }
}

// ----------------------------------------------------------- RadioGroup ----

RadioGroup::RadioGroup() : Widget("radio_group") {
    focusable = true;
    focus_on_hover = false;
    focus_frame = true;
    focus_frame_offset = 4.0f;
    overflow = Overflow::Scroll;
    padding = EdgeInsets::All(4.0f);
}

RadioGroup::RadioGroup(std::string widget_name) : RadioGroup() {
    name = std::move(widget_name);
}

RadioGroup& RadioGroup::AddOption(std::string text, std::string detail, bool disabled) {
    Option option;
    option.text = std::move(text);
    option.detail = std::move(detail);
    option.disabled = disabled;
    options_.push_back(std::move(option));
    select_mix_.push_back(0.0f);
    return *this;
}

RadioGroup& RadioGroup::SetValue(int value, bool notify) {
    if (options_.empty()) {
        return *this;
    }
    const int count = OptionCount();
    const int next = ((value % count) + count) % count;
    if (next == value_) {
        return *this;
    }
    value_ = next;
    cursor_ = next;
    if (notify) {
        emit currentChanged(value_);
        emit toggled(value_);
    }
    return *this;
}

RadioGroup& RadioGroup::SetCursor(int value, bool ensure_visible) {
    if (options_.empty()) {
        return *this;
    }
    const int count = OptionCount();
    int next = value;
    if (loop) {
        next = ((next % count) + count) % count;
    } else {
        next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    }
    cursor_ = next;
    if (ensure_visible) {
        EnsureRectVisible(OptionRect(cursor_));
    }
    return *this;
}

Rect RadioGroup::OptionRect(int index) const {
    const float row_h = row_height;
    const float width = content_rect.Width();
    if (vertical) {
        const float y = content_rect.min.y + static_cast<float>(index) * row_h;
        return Rect::FromPosSize(ImVec2(content_rect.min.x, y), ImVec2(width, row_h));
    }
    const float x = content_rect.min.x + static_cast<float>(index) * width / static_cast<float>(Maxf(1.0f, OptionCount()));
    return Rect::FromPosSize(ImVec2(x, content_rect.min.y), ImVec2(width / static_cast<float>(Maxf(1.0f, OptionCount())), row_h));
}

ImVec2 RadioGroup::MeasureContent(const ImVec2& available) {
    (void)available;
    const int count = OptionCount();
    if (count == 0) {
        return ImVec2(0.0f, 0.0f);
    }
    if (vertical) {
        return ImVec2(dot_size + 200.0f, static_cast<float>(count) * row_height);
    }
    return ImVec2(static_cast<float>(count) * 140.0f, row_height);
}

void RadioGroup::OnAfterLayout() {
    const int count = OptionCount();
    if (count == 0) {
        content_extent = ImVec2(0.0f, 0.0f);
        return;
    }
    content_extent = vertical ? ImVec2(content_rect.Width(), static_cast<float>(count) * row_height)
                              : ImVec2(static_cast<float>(count) * 140.0f, row_height);
}

void RadioGroup::OnUpdate(float dt) {
    capture_vertical = vertical;
    capture_horizontal = !vertical;
    highlight_mix_ = SmoothTo(highlight_mix_, (focused && enabled) ? 1.0f : 0.0f, 16.0f, dt);
    cursor_anim_ = SmoothTo(cursor_anim_, static_cast<float>(cursor_), 20.0f, dt);
    for (std::size_t i = 0; i < select_mix_.size(); ++i) {
        const float target = (static_cast<int>(i) == value_) ? 1.0f : 0.0f;
        select_mix_[i] = SmoothTo(select_mix_[i], target, 18.0f, dt);
    }
    if (focused) {
        EnsureRectVisible(OptionRect(cursor_));
    }
}

bool RadioGroup::OnPadAction(InputAction action) {
    if (options_.empty()) {
        return false;
    }
    switch (action) {
    case InputAction::Up:
        if (vertical) {
            SetCursor(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Down:
        if (vertical) {
            SetCursor(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Left:
        if (!vertical) {
            SetCursor(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Right:
        if (!vertical) {
            SetCursor(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Confirm:
        SetValue(cursor_, true);
        return true;
    default:
        return false;
    }
}

void RadioGroup::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)content;
    const float scale = DrawScale();
    const int count = OptionCount();
    for (int i = 0; i < count; ++i) {
        const Option& option = options_[static_cast<std::size_t>(i)];
        const Rect row = OptionRect(i);
        const bool is_cursor = (i == cursor_);
        const float radius = Theme::kRadiusSmall * scale;

        if (is_cursor && highlight_mix_ > 0.01f) {
            Draw::RoundedRectFilled(dl, row.Expanded(-1.0f * scale),
                                    Theme::Alpha(highlight_bg, highlight_mix_ * EffectiveOpacity()), radius, radius,
                                    radius, radius);
        }

        const float dot = dot_size * scale;
        const Rect indicator = Rect::FromPosSize(
            ImVec2(row.min.x + 12.0f * scale, row.Center().y - dot * 0.5f), ImVec2(dot, dot));
        const float mix = select_mix_[static_cast<std::size_t>(i)];
        Draw::RoundedRectFilled(dl, indicator, Tint(Theme::Mix(dot_bg, dot_selected, mix * 0.25f)), dot * 0.5f, dot * 0.5f,
                                dot * 0.5f, dot * 0.5f);
        Draw::RoundedRectOutline(dl, indicator, Theme::Alpha(Theme::Mix(dot_border, dot_selected, mix), EffectiveOpacity()),
                                 2.0f * scale, dot * 0.5f, dot * 0.5f, dot * 0.5f, dot * 0.5f);
        if (mix > 0.01f) {
            const float inner = dot * 0.5f * mix;
            const Rect core = Rect::FromPosSize(ImVec2(indicator.Center().x - inner, indicator.Center().y - inner),
                                               ImVec2(inner * 2.0f, inner * 2.0f));
            Draw::RoundedRectFilled(dl, core, Tint(dot_selected), inner, inner, inner, inner);
        }

        const float size = (font_size > 0.0f ? font_size : Theme::kFontBody) * scale;
        const ImU32 color = option.disabled ? Theme::kTextDisabled
                                            : (is_cursor && highlight_mix_ > 0.4f ? label_color_focus : label_color);
        Draw::Text(dl, nullptr, size, ImVec2(indicator.max.x + label_gap * scale, row.Center().y - size * 0.5f),
                   Tint(color), option.text.c_str());
        if (!option.detail.empty()) {
            const float detail_size = Theme::kFontSmall * scale;
            const ImVec2 extent = Draw::MeasureText(nullptr, detail_size, option.detail.c_str(), 0.0f);
            Draw::Text(dl, nullptr, detail_size,
                       ImVec2(row.max.x - 14.0f * scale - extent.x, row.Center().y - extent.y * 0.5f),
                       Tint(Theme::kTextMuted), option.detail.c_str());
        }
    }
}

} // namespace gui_dev::cv
