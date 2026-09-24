#include "component_view/components/Progress.h"

#include <cmath>
#include <cstdio>

#include "component_view/Draw.h"

namespace gui_dev::cv {

Progress::Progress() : Widget("progress") {
    interactive = false;
    padding = EdgeInsets{};
}

Progress::Progress(float initial, float min_v, float max_v) : Progress() {
    value = initial;
    min_value = min_v;
    max_value = max_v;
    display_percent_ = Percent();
}

Progress& Progress::SetValue(float next) {
    const float clamped = Clampf(next, Minf(min_value, max_value), Maxf(min_value, max_value));
    if (clamped != value) {
        value = clamped;
        emit valueChanged(Percent());
    }
    return *this;
}

Progress& Progress::SetRange(float min_v, float max_v) {
    min_value = min_v;
    max_value = max_v;
    value = Clampf(value, Minf(min_value, max_value), Maxf(min_value, max_value));
    return *this;
}

Progress& Progress::SetLabel(std::string text) {
    label = std::move(text);
    return *this;
}

ImVec2 Progress::MeasureContent(const ImVec2& available) {
    (void)available;
    const float size = Theme::kFontBody;
    const ImVec2 text = Draw::MeasureText(nullptr, size, label.c_str(), 0.0f);
    const float bar = thickness;
    if (vertical) {
        return ImVec2(Maxf(text.x, bar), 160.0f);
    }
    return ImVec2(Maxf(text.x, 240.0f), bar + (label.empty() ? 0.0f : text.y + label_gap));
}

void Progress::OnUpdate(float dt) {
    if (indeterminate) {
        phase_ += dt * indeterminate_speed;
        if (phase_ > 1.0f) {
            phase_ -= 1.0f;
        }
    }
    const float target = Percent();
    display_percent_ += (target - display_percent_) * (1.0f - std::exp(-animation_speed * dt));
}

void Progress::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const ImU32 fill_color_now = fill_color;
    const float radius = rounded ? (vertical ? content.Width() * 0.5f : thickness * scale * 0.5f) : 0.0f;

    Rect bar = content;
    if (!label.empty() && !vertical) {
        const float size = Theme::kFontBody * scale;
        Draw::Text(dl, nullptr, size, ImVec2(content.min.x, content.min.y), Tint(label_color), label.c_str());
        bar.min.y = content.min.y + size + label_gap * scale;
        bar = Rect{bar.min, content.max};
    }

    const ImU32 track = Tint(track_color);
    Draw::RoundedRectFilled(dl, bar, track, radius, radius, radius, radius);

    if (indeterminate) {
        const float width = bar.Width() * 0.3f;
        const float travel = bar.Width() - width;
        const float x = bar.min.x + travel * Clampf(phase_, 0.0f, 1.0f);
        const Rect chunk = Rect::FromPosSize(ImVec2(x, bar.min.y), ImVec2(width, bar.Height()));
        Draw::RoundedRectFilled(dl, chunk, Tint(fill_color_now), radius, radius, radius, radius);
    } else {
        const float percent = Clampf(display_percent_, 0.0f, 1.0f);
        if (vertical) {
            const float height = bar.Height() * percent;
            const Rect fill = Rect::FromPosSize(ImVec2(bar.min.x, bar.max.y - height), ImVec2(bar.Width(), height));
            Draw::RoundedRectFilled(dl, fill, Tint(fill_color_now), radius, radius, radius, radius);
        } else {
            const Rect fill = Rect::FromPosSize(bar.min, ImVec2(bar.Width() * percent, bar.Height()));
            Draw::RoundedRectFilled(dl, fill, Tint(fill_color_now), radius, radius, radius, radius);
        }
    }

    char buffer[32] = {};
    if (show_percentage) {
        std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(std::round(Percent() * 100.0f)));
    } else if (show_value_text) {
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    }
    if (buffer[0] != '\0') {
        const float size = Theme::kFontSmall * scale;
        const ImVec2 extent = Draw::MeasureText(nullptr, size, buffer, 0.0f);
        if (vertical) {
            Draw::Text(dl, nullptr, size, ImVec2(bar.Center().x - extent.x * 0.5f, bar.max.y + 8.0f * scale),
                       Tint(text_color), buffer);
        } else {
            Draw::Text(dl, nullptr, size,
                       ImVec2(bar.max.x - extent.x, bar.Center().y - extent.y * 0.5f), Tint(text_color), buffer);
        }
    }
}

} // namespace gui_dev::cv
