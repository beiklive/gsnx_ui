#include "component_view/components/Slider.h"

#include <cmath>
#include <cstdio>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

Slider::Slider() : Widget("slider") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = true;
    focus_frame_offset = 4.0f;
    focus_scale = 1.02f;
    padding = EdgeInsets::Symmetric(10.0f, 7.0f);
    corner_radius = Theme::kRadiusSmall;
}

Slider::Slider(std::string text, float initial, float min_v, float max_v) : Slider() {
    label = std::move(text);
    value = initial;
    min_value = min_v;
    max_value = max_v;
    display_value_ = value;
    focus_on_value_ = value;
}

Slider& Slider::SetRange(float min_v, float max_v) {
    min_value = min_v;
    max_value = max_v;
    value = Clamp(value);
    return *this;
}

Slider& Slider::SetValue(float next, bool notify) {
    const float clamped = Clamp(next);
    if (clamped == value) {
        return *this;
    }
    value = clamped;
    if (notify && on_changed) {
        on_changed(*this, value);
    }
    return *this;
}

Slider& Slider::SetLabel(std::string text) {
    label = std::move(text);
    return *this;
}

Slider& Slider::SetStep(float normal_step, float fast, float page) {
    step = normal_step;
    fast_step = fast;
    page_step = page;
    return *this;
}

void Slider::Nudge(float delta, bool notify) {
    float base = value + delta;
    // 吸附到 step 的整数倍，避免出现 65.00001 这种值
    if (step > 0.0f) {
        base = min_value + std::round((base - min_value) / step) * step;
    }
    SetValue(base, notify);
}

ImVec2 Slider::MeasureContent(const ImVec2& available) {
    (void)available;
    const float size = Theme::kFontBody;
    const ImVec2 text = Draw::MeasureText(nullptr, size, label.c_str(), 0.0f);
    return ImVec2(Maxf(text.x, 200.0f), Maxf(text.y, Theme::kControlHeight * 0.6f));
}

void Slider::OnUpdate(float dt) {
    capture_horizontal = !vertical;
    capture_vertical = vertical;
    display_value_ = SmoothTo(display_value_, value, 14.0f, dt);
    thumb_mix_ = SmoothTo(thumb_mix_, (focused && enabled) ? 1.0f : 0.0f, 16.0f, dt);
    if (!focused) {
        adjusting_ = false;
    }
}

bool Slider::OnPadAction(InputAction action) {
    const float direction = vertical ? 1.0f : 0.0f;
    (void)direction;
    switch (action) {
    case InputAction::Left:
        if (!vertical) {
            Nudge(-step);
            return true;
        }
        return false;
    case InputAction::Right:
        if (!vertical) {
            Nudge(step);
            return true;
        }
        return false;
    case InputAction::Up:
        if (vertical) {
            Nudge(step);
            return true;
        }
        return false;
    case InputAction::Down:
        if (vertical) {
            Nudge(-step);
            return true;
        }
        return false;
    case InputAction::PageLeft:
        Nudge(-fast_step);
        return true;
    case InputAction::PageRight:
        Nudge(fast_step);
        return true;
    case InputAction::TriggerLeft:
        Nudge(-page_step);
        return true;
    case InputAction::TriggerRight:
        Nudge(page_step);
        return true;
    case InputAction::Confirm:
        if (on_changed) {
            on_changed(*this, value);
        }
        adjusting_ = false;
        return true;
    case InputAction::Cancel:
        if (revert_on_cancel && value != focus_on_value_) {
            value = focus_on_value_;
            if (on_changed) {
                on_changed(*this, value);
            }
            return true;
        }
        return false;
    default:
        return false;
    }
}

void Slider::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float size = Theme::kFontBody * scale;
    const float label_room = label_width > 0.0f ? label_width * scale
                                               : (label.empty() ? 0.0f
                                                                : Draw::MeasureText(nullptr, size, label.c_str(), 0.0f).x +
                                                                      16.0f * scale);
    if (!label.empty()) {
        Draw::Text(dl, nullptr, size, ImVec2(content.min.x, content.Center().y - size * 0.5f),
                   Tint(focused ? Theme::kTextBright : label_color), label.c_str());
    }

    char value_text[64];
    float value_width = 0.0f;
    if (show_value) {
        if (show_percent) {
            std::snprintf(value_text, sizeof(value_text), "%d%%",
                          static_cast<int>(std::round(Percent() * 100.0f)));
        } else {
            std::snprintf(value_text, sizeof(value_text), "%.2f", value);
        }
        value_width = Draw::MeasureText(nullptr, size, value_text, 0.0f).x + 20.0f * scale;
    }

    const float track_start = content.min.x + label_room;
    const float track_end = content.max.x - value_width;
    const float center_y = content.Center().y;
    const float thickness = track_thickness * scale;
    const float radius = thickness * 0.5f;
    const float progress = Clampf(Percent(), 0.0f, 1.0f);

    const Rect track = Rect::FromPosSize(ImVec2(track_start, center_y - thickness * 0.5f),
                                        ImVec2(Maxf(track_end - track_start, 10.0f), thickness));
    Draw::RoundedRectFilled(dl, track, Tint(track_color), radius, radius, radius, radius);

    const float thumb_x = track.min.x + track.Width() * Clampf(display_value_ <= max_value ? progress : progress, 0.0f, 1.0f);
    const Rect fill = Rect::FromPosSize(track.min, ImVec2(Maxf(thumb_x - track.min.x, 0.0f), thickness));
    Draw::RoundedRectFilled(dl, fill, Tint(fill_color), radius, radius, radius, radius);

    // 刻度：step 较粗时画出刻度点，方便手柄对齐
    if (step > 0.0f && max_value > min_value) {
        const int ticks = static_cast<int>((max_value - min_value) / step);
        if (ticks > 0 && ticks <= 24) {
            for (int i = 0; i <= ticks; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(ticks);
                const float x = track.min.x + track.Width() * t;
                Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(x - 0.5f * scale, center_y - 4.0f * scale),
                                                             ImVec2(1.0f * scale, 8.0f * scale)),
                                        Tint(Theme::Alpha(Theme::kTextMuted, 0.5f)), 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    const float thumb_r = (thumb_radius * (1.0f + 0.22f * thumb_mix_)) * scale;
    Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(thumb_x - thumb_r, center_y - thumb_r),
                                                 ImVec2(thumb_r * 2.0f, thumb_r * 2.0f)),
                            Tint(thumb_color), thumb_r, thumb_r, thumb_r, thumb_r);
    if (thumb_mix_ > 0.01f) {
        const float ring = thumb_r + 4.0f * scale;
        Draw::RoundedRectOutline(dl, Rect::FromPosSize(ImVec2(thumb_x - ring, center_y - ring),
                                                      ImVec2(ring * 2.0f, ring * 2.0f)),
                                 Theme::Alpha(Theme::kAccent, thumb_mix_), 2.0f * scale, ring, ring, ring, ring);
    }

    if (show_value) {
        const ImVec2 extent = Draw::MeasureText(nullptr, size, value_text, 0.0f);
        Draw::Text(dl, nullptr, size, ImVec2(content.max.x - extent.x, center_y - extent.y * 0.5f), Tint(value_color),
                   value_text);
    }
}

} // namespace gui_dev::cv
