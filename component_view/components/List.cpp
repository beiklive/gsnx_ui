#include "component_view/components/List.h"

#include <cmath>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

List::List() : Widget("list") {
    focusable = true;
    focus_on_hover = false;
    focus_frame = true;
    focus_frame_offset = 3.0f;
    overflow = Overflow::Scroll;
    scroll_bar = true;
    padding = EdgeInsets::All(4.0f);
}

List::List(std::string widget_name) : List() {
    name = std::move(widget_name);
}

List& List::AddItem(std::string text, std::string icon, std::string detail) {
    Item item;
    item.text = std::move(text);
    item.icon = std::move(icon);
    item.detail = std::move(detail);
    items_.push_back(std::move(item));
    enter_mix_.push_back(item_enter_animation ? 0.0f : 1.0f);
    return *this;
}

List& List::SetItemDisabled(int index, bool value) {
    if (index >= 0 && index < ItemCount()) {
        items_[static_cast<std::size_t>(index)].disabled = value;
    }
    return *this;
}

List& List::ClearItems() {
    items_.clear();
    enter_mix_.clear();
    index_ = 0;
    selected_ = -1;
    return *this;
}

List& List::SetFocusIndex(int value, bool ensure_visible) {
    if (items_.empty()) {
        index_ = 0;
        return *this;
    }
    const int count = ItemCount();
    int next = value;
    if (loop) {
        next = ((next % count) + count) % count;
    } else {
        next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    }
    if (next != index_) {
        index_ = next;
        emit currentIndexChanged(index_);
    }
    if (ensure_visible) {
        EnsureRectVisible(ItemRect(index_));
    }
    return *this;
}

ImVec2 List::ItemsExtent() const {
    const int count = ItemCount();
    if (count == 0) {
        return ImVec2(0.0f, 0.0f);
    }
    const float row_h = item_size.y;
    if (orientation == Orientation::Horizontal) {
        return ImVec2(static_cast<float>(count) * (item_size.x + item_gap.x) - item_gap.x, row_h);
    }
    if (orientation == Orientation::Grid) {
        const int cols = columns > 0 ? columns : 1;
        const int rows = (count + cols - 1) / cols;
        return ImVec2(static_cast<float>(cols) * (item_size.x + item_gap.x) - item_gap.x,
                      static_cast<float>(rows) * (row_h + item_gap.y) - item_gap.y);
    }
    return ImVec2(item_size.x, static_cast<float>(count) * (row_h + item_gap.y) - item_gap.y);
}

ImVec2 List::MeasureContent(const ImVec2& available) {
    (void)available;
    return ItemsExtent();
}

void List::OnAfterLayout() {
    content_extent = ItemsExtent();
}

Rect List::ItemRect(int index) const {
    const float row_h = item_size.y;
    const float width = item_size.x > 0.0f ? item_size.x : content_rect.Width();
    if (orientation == Orientation::Horizontal) {
        const float x = content_rect.min.x + static_cast<float>(index) * (width + item_gap.x);
        return Rect::FromPosSize(ImVec2(x, content_rect.min.y), ImVec2(width, row_h));
    }
    if (orientation == Orientation::Grid) {
        const int cols = columns > 0 ? columns : 1;
        const int col = index % cols;
        const int row = index / cols;
        const float x = content_rect.min.x + static_cast<float>(col) * (width + item_gap.x);
        const float y = content_rect.min.y + static_cast<float>(row) * (row_h + item_gap.y);
        return Rect::FromPosSize(ImVec2(x, y), ImVec2(width, row_h));
    }
    const float y = content_rect.min.y + static_cast<float>(index) * (row_h + item_gap.y);
    return Rect::FromPosSize(ImVec2(content_rect.min.x, y), ImVec2(width, row_h));
}

Rect List::IndicatorRect(int index) const {
    const Rect row = ItemRect(index);
    const float height = row.Height() * 0.62f;
    return Rect::FromPosSize(ImVec2(row.min.x - focus_indicator_width - 2.0f, row.Center().y - height * 0.5f),
                             ImVec2(focus_indicator_width, height));
}

int List::VisibleItemCount() const {
    if (orientation == Orientation::Vertical) {
        const float row = item_size.y + item_gap.y;
        return Maxf(1, static_cast<int>(content_rect.Height() / Maxf(row, 1.0f)));
    }
    const float col = item_size.x + item_gap.x;
    const int cols = columns > 0 ? columns : 1;
    return Maxf(1, static_cast<int>(content_rect.Width() / Maxf(col, 1.0f)) * cols);
}

void List::MoveFocus(int delta) {
    if (items_.empty() || delta == 0) {
        return;
    }
    SetFocusIndex(index_ + delta);
}

void List::OnUpdate(float dt) {
    capture_vertical = orientation != Orientation::Horizontal;
    capture_horizontal = orientation != Orientation::Vertical;
    indicator_mix_ = SmoothTo(indicator_mix_, (focused && enabled) ? 1.0f : 0.0f, 18.0f, dt);
    index_anim_ = SmoothTo(index_anim_, static_cast<float>(index_), 22.0f, dt);
    if (item_enter_animation) {
        for (std::size_t i = 0; i < enter_mix_.size(); ++i) {
            if (enter_mix_[i] < 1.0f) {
                enter_mix_[i] = SmoothTo(enter_mix_[i], 1.0f, enter_speed, dt);
            }
        }
    }
    // 键盘焦点在 List 上时，把当前项保持在可见区
    if (focused) {
        EnsureRectVisible(ItemRect(index_));
    }
}

bool List::OnPadAction(InputAction action) {
    if (items_.empty()) {
        return false;
    }
    switch (action) {
    case InputAction::Up:
        if (orientation == Orientation::Grid) {
            MoveFocus(-columns);
        } else if (orientation == Orientation::Vertical) {
            MoveFocus(-1);
        }
        return true;
    case InputAction::Down:
        if (orientation == Orientation::Grid) {
            MoveFocus(columns);
        } else if (orientation == Orientation::Vertical) {
            MoveFocus(1);
        }
        return true;
    case InputAction::Left:
        if (orientation == Orientation::Horizontal) {
            MoveFocus(-1);
            return true;
        }
        if (orientation == Orientation::Grid) {
            MoveFocus(-1);
            return true;
        }
        return false; // 竖直列表：← 交给页面导航到别的控件
    case InputAction::Right:
        if (orientation == Orientation::Horizontal) {
            MoveFocus(1);
            return true;
        }
        if (orientation == Orientation::Grid) {
            MoveFocus(1);
            return true;
        }
        return false;
    case InputAction::PageLeft:
        MoveFocus(-VisibleItemCount());
        return true;
    case InputAction::PageRight:
        MoveFocus(VisibleItemCount());
        return true;
    case InputAction::TriggerLeft:
        MoveFocus(-fast_scroll_items);
        return true;
    case InputAction::TriggerRight:
        MoveFocus(fast_scroll_items);
        return true;
    case InputAction::ActionX:
        SetFocusIndex(0);
        return true;
    case InputAction::ActionY:
        SetFocusIndex(ItemCount() - 1);
        return true;
    case InputAction::Confirm:
        if (index_ >= 0 && index_ < ItemCount() && !items_[static_cast<std::size_t>(index_)].disabled) {
            selected_ = index_;
            emit itemActivated(index_);
            emit itemClicked(index_);
            return true;
        }
        return true;
    default:
        return false;
    }
}

void List::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)content;
    const float scale = DrawScale();
    const int count = ItemCount();
    for (int i = 0; i < count; ++i) {
        const Rect row = ItemRect(i);
        if (row.max.y < DrawRect().min.y - 4.0f || row.min.y > DrawRect().max.y + 4.0f) {
            continue;
        }
        const Item& item = items_[static_cast<std::size_t>(i)];
        const float appear = item_enter_animation ? Clampf(enter_mix_[static_cast<std::size_t>(i)], 0.0f, 1.0f) : 1.0f;
        const bool is_focus = (i == index_);
        const bool is_selected = (i == selected_);
        const float radius = item_radius * scale;

        ImU32 background = ((i % 2) == 0 || !zebra) ? row_color : row_alt_color;
        if (is_selected) {
            background = Theme::Mix(background, row_selected_color, 0.9f);
        }
        if (is_focus && indicator_mix_ > 0.01f) {
            background = Theme::Mix(background, row_focus_color, indicator_mix_);
        }
        const ImU32 painted = Theme::Alpha(background, EffectiveOpacity() * appear);
        Draw::RoundedRectFilled(dl, row, painted, radius, radius, radius, radius);

        if (is_focus && indicator_mix_ > 0.02f) {
            const Rect indicator = IndicatorRect(i);
            Draw::RoundedRectFilled(dl, indicator, Theme::Alpha(row_focus_color == 0 ? Theme::kAccent : Theme::kAccent,
                                                                indicator_mix_ * EffectiveOpacity()),
                                    2.0f * scale, 2.0f * scale, 2.0f * scale, 2.0f * scale);
            Draw::RoundedRectOutline(dl, row.Expanded(-1.0f * scale), Theme::Alpha(Theme::kAccent, indicator_mix_ * 0.7f),
                                     1.5f * scale, radius, radius, radius, radius);
        }

        const float text_size = Theme::kFontBody * scale;
        float cursor_x = row.min.x + 10.0f * scale;
        if (show_index) {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%02d", i + 1);
            Draw::Text(dl, nullptr, Theme::kFontSmall * scale, ImVec2(cursor_x, row.Center().y - text_size * 0.4f),
                       Theme::Alpha(Theme::kTextMuted, EffectiveOpacity() * appear), buffer);
            cursor_x += 26.0f * scale;
        }
        if (!item.icon.empty()) {
            const float icon_size = Theme::kFontBody * 1.25f * scale;
            const ImVec2 extent = Draw::MeasureText(nullptr, icon_size, item.icon.c_str(), 0.0f);
            Draw::Text(dl, nullptr, icon_size, ImVec2(cursor_x, row.Center().y - extent.y * 0.5f),
                       Theme::Alpha(item.disabled ? Theme::kTextDisabled : Theme::kTextPrimary,
                                    EffectiveOpacity() * appear),
                       item.icon.c_str());
            cursor_x += extent.x + 8.0f * scale;
        }

        const float detail_size = Theme::kFontSmall * scale;
        float detail_width = 0.0f;
        if (!item.detail.empty()) {
            detail_width = Draw::MeasureText(nullptr, detail_size, item.detail.c_str(), 0.0f).x + 12.0f * scale;
        }
        const float text_room = row.max.x - detail_width - cursor_x - 12.0f * scale;
        const ImU32 text_color = item.disabled
                                     ? Theme::kTextDisabled
                                     : (is_focus ? Theme::kTextBright : Theme::kTextPrimary);
        const char* shown = Draw::Ellipsize(nullptr, text_size, item.text.c_str(), text_room);
        Draw::Text(dl, nullptr, text_size,
                   ImVec2(cursor_x, row.Center().y - text_size * 0.5f),
                   Theme::Alpha(text_color, EffectiveOpacity() * appear), shown);

        if (!item.detail.empty()) {
            const ImVec2 extent = Draw::MeasureText(nullptr, detail_size, item.detail.c_str(), 0.0f);
            Draw::Text(dl, nullptr, detail_size,
                       ImVec2(row.max.x - 10.0f * scale - extent.x, row.Center().y - extent.y * 0.5f),
                       Theme::Alpha(Theme::kTextMuted, EffectiveOpacity() * appear), item.detail.c_str());
        }
    }
}

} // namespace gui_dev::cv
