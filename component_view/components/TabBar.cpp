#include "component_view/components/TabBar.h"

#include <cmath>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

TabBar::TabBar() : Widget("tab_bar") {
    focusable = true;
    focus_on_hover = false;
    overflow = Overflow::Scroll;
    scroll_bar = true;
    scroll_bar_auto_hide = false;
    padding = EdgeInsets::All(4.0f);
}

TabBar::TabBar(std::string widget_name) : TabBar() {
    name = std::move(widget_name);
}

TabBar& TabBar::AddTab(std::string text, std::string icon, bool disabled) {
    Tab tab;
    tab.text = std::move(text);
    tab.icon = std::move(icon);
    tab.disabled = disabled;
    tabs_.push_back(std::move(tab));
    return *this;
}

TabBar& TabBar::SetIndex(int value, bool notify) {
    if (tabs_.empty()) {
        return *this;
    }
    const int count = TabCount();
    const int next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    if (next == index_) {
        return *this;
    }
    index_ = next;
    if (notify) {
        emit currentChanged(index_);
    }
    return *this;
}

TabBar& TabBar::SetCursor(int value, bool ensure_visible) {
    if (tabs_.empty()) {
        return *this;
    }
    const int count = TabCount();
    const int next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    cursor_ = next;
    if (immediate) {
        SetIndex(cursor_, true);
    }
    if (ensure_visible) {
        EnsureRectVisible(TabRect(cursor_));
    }
    return *this;
}

Rect TabBar::TabRect(int index) const {
    const float width = tab_size.x > 0.0f ? tab_size.x : content_rect.Width();
    if (orientation == Orientation::Vertical) {
        const float y = content_rect.min.y + static_cast<float>(index) * (tab_size.y + gap);
        return Rect::FromPosSize(ImVec2(content_rect.min.x, y), ImVec2(width, tab_size.y));
    }
    const float x = content_rect.min.x + static_cast<float>(index) * (width + gap);
    return Rect::FromPosSize(ImVec2(x, content_rect.min.y), ImVec2(width, tab_size.y));
}

int TabBar::VisiblePageSize() const {
    const float span = (orientation == Orientation::Vertical ? tab_size.y : tab_size.x) + gap;
    const float view = orientation == Orientation::Vertical ? content_rect.Height() : content_rect.Width();
    return Maxf(1, static_cast<int>(view / Maxf(span, 1.0f)));
}

ImVec2 TabBar::MeasureContent(const ImVec2& available) {
    (void)available;
    const int count = TabCount();
    const float span = tab_size.y + gap;
    float widest = 0.0f;
    for (const Tab& tab : tabs_) {
        const float text = Draw::MeasureText(nullptr, font_size > 0.0f ? font_size : Theme::kFontBody, tab.text.c_str(), 0.0f).x;
        widest = Maxf(widest, text + 42.0f);
    }
    if (orientation == Orientation::Vertical) {
        return ImVec2(widest, static_cast<float>(count) * span - gap);
    }
    return ImVec2(static_cast<float>(count) * (widest + gap) - gap, tab_size.y);
}

void TabBar::OnAfterLayout() {
    content_extent = MeasureContent(ImVec2(0.0f, 0.0f));
    if (orientation == Orientation::Vertical) {
        content_extent.x = content_rect.Width();
    } else {
        content_extent.y = content_rect.Height();
    }
}

void TabBar::OnUpdate(float dt) {
    capture_vertical = orientation == Orientation::Vertical;
    capture_horizontal = orientation == Orientation::Horizontal;
    cursor_anim_ = SmoothTo(cursor_anim_, static_cast<float>(cursor_), transition_speed, dt);
    focus_mix_local_ = SmoothTo(focus_mix_local_, (focused && enabled) ? 1.0f : 0.0f, 16.0f, dt);
    if (focused) {
        EnsureRectVisible(TabRect(cursor_));
    }
}

bool TabBar::OnPadAction(InputAction action) {
    if (tabs_.empty()) {
        return false;
    }
    const int page = VisiblePageSize();
    switch (action) {
    case InputAction::Up:
        if (orientation == Orientation::Vertical) {
            SetCursor(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Down:
        if (orientation == Orientation::Vertical) {
            SetCursor(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Left:
        if (orientation == Orientation::Horizontal) {
            SetCursor(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Right:
        if (orientation == Orientation::Horizontal) {
            SetCursor(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Confirm:
        SetIndex(cursor_, true);
        emit tabBarClicked(cursor_);
        return true;
    case InputAction::PageLeft:
        if (wrap_pages) {
            ScrollPage(-1, 1.0f);
            SetCursor(cursor_ - page);
            return true;
        }
        return false;
    case InputAction::PageRight:
        if (wrap_pages) {
            ScrollPage(1, 1.0f);
            SetCursor(cursor_ + page);
            return true;
        }
        return false;
    case InputAction::TriggerLeft:
        SetCursor(0);
        scroll_target = ImVec2(0.0f, 0.0f);
        return true;
    case InputAction::TriggerRight:
        SetCursor(TabCount() - 1);
        return true;
    default:
        return false;
    }
}

void TabBar::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)content;
    const float scale = DrawScale();
    const int count = TabCount();
    const float font = (font_size > 0.0f ? font_size : Theme::kFontBody) * scale;
    const float radius = Theme::kRadiusSmall * scale;

    // 指示条：在 cursor_anim_ 的整数部分之间插值，得到滑动动画
    const int from = static_cast<int>(cursor_anim_);
    const int to = Minf(static_cast<float>(count - 1), static_cast<float>(from + 1));
    const float t = Clampf(cursor_anim_ - static_cast<float>(from), 0.0f, 1.0f);
    if (count > 0) {
        const Rect a = TabRect(from);
        const Rect b = TabRect(to);
        const Rect blended = Rect{ImVec2(a.min.x + (b.min.x - a.min.x) * t, a.min.y + (b.min.y - a.min.y) * t),
                                  ImVec2(a.max.x + (b.max.x - a.max.x) * t, a.max.y + (b.max.y - a.max.y) * t)};
        const float mix = focus_mix_local_;
        if (mix > 0.01f) {
            const Rect indicator =
                orientation == Orientation::Vertical
                    ? Rect::FromPosSize(ImVec2(blended.min.x - indicator_width * scale - 2.0f * scale,
                                               blended.Center().y - blended.Height() * 0.3f),
                                        ImVec2(indicator_width * scale, blended.Height() * 0.6f))
                    : Rect::FromPosSize(ImVec2(blended.Center().x - blended.Width() * 0.3f,
                                               blended.max.y + 1.0f * scale),
                                        ImVec2(blended.Width() * 0.6f, indicator_width * scale));
            Draw::RoundedRectFilled(dl, indicator, Theme::Alpha(indicator_color, mix), indicator_width * scale * 0.5f,
                                    indicator_width * scale * 0.5f, indicator_width * scale * 0.5f,
                                    indicator_width * scale * 0.5f);
        }
    }

    for (int i = 0; i < count; ++i) {
        const Tab& tab = tabs_[static_cast<std::size_t>(i)];
        const Rect row = TabRect(i);
        const bool is_cursor = (i == cursor_);
        const bool is_active = (i == index_);
        const float appear = is_cursor ? focus_mix_local_ : 0.0f;

        if (is_active) {
            Draw::RoundedRectFilled(dl, row, Tint(tab_active_color), radius, radius, radius, radius);
        }
        if (is_cursor && appear > 0.01f) {
            Draw::RoundedRectFilled(dl, row, Theme::Alpha(tab_cursor_color, appear * 0.9f), radius, radius, radius,
                                    radius);
        }

        float cursor_x = row.min.x + 10.0f * scale;
        if (!tab.icon.empty()) {
            const float icon_size = font * 1.05f;
            const ImVec2 extent = Draw::MeasureText(nullptr, icon_size, tab.icon.c_str(), 0.0f);
            Draw::Text(dl, nullptr, icon_size, ImVec2(cursor_x, row.Center().y - extent.y * 0.5f),
                       Tint(tab.disabled ? Theme::kTextDisabled
                                         : (is_active || is_cursor ? text_color_active : text_color)),
                       tab.icon.c_str());
            cursor_x += extent.x + icon_gap * scale;
        }
        const ImU32 color = tab.disabled ? Theme::kTextDisabled
                                         : (is_active ? text_color_active : (is_cursor ? text_color_focus : text_color));
        Draw::Text(dl, nullptr, font, ImVec2(cursor_x, row.Center().y - font * 0.5f), Tint(color), tab.text.c_str());
    }
}

} // namespace gui_dev::cv
