#include "component_view/components/Menu.h"

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

Menu::Menu() : Widget("menu") {
    focusable = true;
    focus_on_hover = false;
    focus_frame = true;
    focus_frame_offset = 4.0f;
    overflow = Overflow::Scroll;
    scroll_bar = true;
    padding = EdgeInsets::All(8.0f);
    sections_.emplace_back();
    stack_.push_back(&sections_.front());
}

Menu::Menu(std::string widget_name) : Menu() {
    name = std::move(widget_name);
}

Menu::Node& Menu::CurrentNode() {
    return *stack_.back();
}

const Menu::Node& Menu::CurrentNode() const {
    return *stack_.back();
}

std::vector<Menu::Entry>& Menu::CurrentEntries() {
    return CurrentNode().entries;
}

Menu& Menu::SetTitle(std::string value) {
    title = std::move(value);
    CurrentNode().title = title;
    return *this;
}

Menu& Menu::AddEntry(std::string label, std::string icon, std::string shortcut, std::string value) {
    Entry entry;
    entry.label = std::move(label);
    entry.icon = std::move(icon);
    entry.shortcut = std::move(shortcut);
    entry.value = std::move(value);
    CurrentEntries().push_back(std::move(entry));
    return *this;
}

Menu& Menu::AddSeparator() {
    Entry entry;
    entry.separator = true;
    CurrentEntries().push_back(std::move(entry));
    return *this;
}

Menu& Menu::SetEntryDisabled(int index, bool disabled) {
    if (index >= 0 && index < EntryCount()) {
        CurrentEntries()[static_cast<std::size_t>(index)].disabled = disabled;
    }
    return *this;
}

Menu& Menu::SetEntrySelected(int index, bool selected) {
    if (index >= 0 && index < EntryCount()) {
        CurrentEntries()[static_cast<std::size_t>(index)].selected = selected;
    }
    return *this;
}

Menu& Menu::EnterSubmenu(int entry_index, std::string submenu_title) {
    if (entry_index < 0 || entry_index >= EntryCount()) {
        return *this;
    }
    Node& node = CurrentNode();
    auto child = std::make_unique<Node>();
    child->title = std::move(submenu_title);
    node.children.push_back(std::move(child));
    node.entries[static_cast<std::size_t>(entry_index)].submenu = static_cast<int>(node.children.size()) - 1;
    stack_.push_back(node.children.back().get());
    cursor_ = 0;
    return *this;
}

Menu& Menu::LeaveSubmenu() {
    if (stack_.size() > 1) {
        stack_.pop_back();
    }
    return *this;
}

Menu& Menu::SetCursor(int value, bool notify) {
    const int count = EntryCount();
    if (count == 0) {
        cursor_ = 0;
        return *this;
    }
    int next = value;
    if (loop) {
        next = ((next % count) + count) % count;
    } else {
        next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    }
    // 跳过分隔符
    int guard = 0;
    while (guard < count && CurrentEntries()[static_cast<std::size_t>(next)].separator) {
        next = loop ? ((next + 1) % count) : Minf(static_cast<float>(count - 1), static_cast<float>(next + 1));
        ++guard;
    }
    if (next != cursor_) {
        cursor_ = next;
        if (notify && on_focus_changed) {
            on_focus_changed(*this, cursor_);
        }
    }
    EnsureRectVisible(EntryRect(cursor_));
    return *this;
}

float Menu::BreadcrumbHeight() const {
    if (!show_breadcrumb || stack_.size() <= 1) {
        return 0.0f;
    }
    return Theme::kFontSmall + 6.0f;
}

Rect Menu::EntryRect(int index) const {
    const float width = content_rect.Width();
    const float top = content_rect.min.y + BreadcrumbHeight();
    if (vertical) {
        const float y = top + static_cast<float>(index) * (row_height + gap);
        return Rect::FromPosSize(ImVec2(content_rect.min.x, y), ImVec2(width, row_height));
    }
    const float x = content_rect.min.x + static_cast<float>(index) * (width + gap);
    return Rect::FromPosSize(ImVec2(x, top), ImVec2(width, row_height));
}

std::string Menu::EntryLabel(int index) const {
    const std::vector<Entry>& entries = CurrentNode().entries;
    if (index < 0 || index >= static_cast<int>(entries.size())) {
        return std::string();
    }
    return entries[static_cast<std::size_t>(index)].label;
}

std::string Menu::Breadcrumb() const {
    std::string out;
    for (std::size_t i = 0; i < stack_.size(); ++i) {
        if (stack_[i]->title.empty()) {
            continue;
        }
        if (!out.empty()) {
            out += " › ";
        }
        out += stack_[i]->title;
    }
    return out;
}

void Menu::ActivateCurrent() {
    const int count = EntryCount();
    if (count == 0 || cursor_ < 0 || cursor_ >= count) {
        return;
    }
    Entry& entry = CurrentEntries()[static_cast<std::size_t>(cursor_)];
    if (entry.disabled || entry.separator) {
        return;
    }
    if (entry.submenu >= 0 && entry.submenu < static_cast<int>(CurrentNode().children.size())) {
        Node* child = CurrentNode().children[static_cast<std::size_t>(entry.submenu)].get();
        stack_.push_back(child);
        cursor_ = 0;
        scroll = ImVec2(0.0f, 0.0f);
        scroll_target = ImVec2(0.0f, 0.0f);
        SetCursor(0, true);
        return;
    }
    entry.selected = !entry.selected;
    if (on_activate) {
        on_activate(*this, cursor_);
    }
}

bool Menu::GoBack() {
    if (stack_.size() <= 1) {
        return false;
    }
    stack_.pop_back();
    cursor_ = 0;
    scroll = ImVec2(0.0f, 0.0f);
    scroll_target = ImVec2(0.0f, 0.0f);
    return true;
}

ImVec2 Menu::MeasureContent(const ImVec2& available) {
    (void)available;
    const int count = EntryCount();
    if (count == 0) {
        return ImVec2(200.0f, row_height);
    }
    const float height = static_cast<float>(count) * (row_height + gap) - gap;
    if (vertical) {
        return ImVec2(260.0f, height);
    }
    return ImVec2(static_cast<float>(count) * (200.0f + gap) - gap, row_height);
}

void Menu::OnAfterLayout() {
    content_extent = MeasureContent(ImVec2(0.0f, 0.0f));
    content_extent.y += BreadcrumbHeight();
    if (vertical) {
        content_extent.x = content_rect.Width();
    } else {
        content_extent.y = content_rect.Height();
    }
}

void Menu::OnUpdate(float dt) {
    capture_vertical = vertical;
    capture_horizontal = !vertical;
    cursor_anim_ = SmoothTo(cursor_anim_, static_cast<float>(cursor_), 20.0f, dt);
    focus_mix_local_ = SmoothTo(focus_mix_local_, (focused && enabled) ? 1.0f : 0.0f, 16.0f, dt);
    if (focused) {
        EnsureRectVisible(EntryRect(cursor_));
    }
}

bool Menu::OnPadAction(InputAction action) {
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
        ActivateCurrent();
        return true;
    case InputAction::Cancel:
        return GoBack();
    case InputAction::PageLeft:
        if (sections_.size() > 1) {
            section_ = (section_ - 1 + static_cast<int>(sections_.size())) % static_cast<int>(sections_.size());
            stack_.assign(1, &sections_[static_cast<std::size_t>(section_)]);
            cursor_ = 0;
            scroll = scroll_target = ImVec2(0.0f, 0.0f);
            return true;
        }
        return false;
    case InputAction::PageRight:
        if (sections_.size() > 1) {
            section_ = (section_ + 1) % static_cast<int>(sections_.size());
            stack_.assign(1, &sections_[static_cast<std::size_t>(section_)]);
            cursor_ = 0;
            scroll = scroll_target = ImVec2(0.0f, 0.0f);
            return true;
        }
        return false;
    case InputAction::TriggerLeft:
        SetCursor(0);
        return true;
    case InputAction::TriggerRight:
        SetCursor(EntryCount() - 1);
        return true;
    default:
        return false;
    }
}

void Menu::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float font = (font_size > 0.0f ? font_size : Theme::kFontBody) * scale;
    const int count = EntryCount();

    // 面包屑（在子菜单里显示路径，占内容区顶部一行）
    if (BreadcrumbHeight() > 0.0f) {
        const std::string crumb = Breadcrumb();
        Draw::Text(dl, nullptr, Theme::kFontSmall * scale, ImVec2(content.min.x + 4.0f * scale, content.min.y),
                   Tint(Theme::kTextMuted), crumb.c_str());
    }

    for (int i = 0; i < count; ++i) {
        const Entry& entry = CurrentEntries()[static_cast<std::size_t>(i)];
        const Rect row = EntryRect(i);
        if (row.max.y < DrawRect().min.y - 4.0f || row.min.y > DrawRect().max.y + 4.0f) {
            continue;
        }
        if (entry.separator) {
            dl->AddLine(ImVec2(row.min.x + 8.0f * scale, row.Center().y),
                        ImVec2(row.max.x - 8.0f * scale, row.Center().y), Tint(Theme::kBorder), 1.0f * scale);
            continue;
        }

        const bool is_cursor = (i == cursor_);
        const float radius = row_radius * scale;
        if (entry.selected) {
            Draw::RoundedRectFilled(dl, row, Tint(row_selected_color), radius, radius, radius, radius);
        }
        if (is_cursor && focus_mix_local_ > 0.01f) {
            Draw::RoundedRectFilled(dl, row, Theme::Alpha(row_focus_color, focus_mix_local_), radius, radius, radius,
                                    radius);
            const Rect indicator = Rect::FromPosSize(
                ImVec2(row.min.x - 4.0f * scale, row.Center().y - row.Height() * 0.28f),
                ImVec2(3.5f * scale, row.Height() * 0.56f));
            Draw::RoundedRectFilled(dl, indicator, Theme::Alpha(indicator_color, focus_mix_local_), 2.0f * scale,
                                    2.0f * scale, 2.0f * scale, 2.0f * scale);
        }

        float cursor_x = row.min.x + 16.0f * scale;
        if (show_index) {
            char buffer[8];
            std::snprintf(buffer, sizeof(buffer), "%d", i + 1);
            Draw::Text(dl, nullptr, Theme::kFontSmall * scale, ImVec2(cursor_x, row.Center().y - font * 0.4f),
                       Tint(Theme::kTextMuted), buffer);
            cursor_x += 24.0f * scale;
        }
        if (!entry.icon.empty()) {
            const float icon_size = font * 1.1f;
            const ImVec2 extent = Draw::MeasureText(nullptr, icon_size, entry.icon.c_str(), 0.0f);
            Draw::Text(dl, nullptr, icon_size, ImVec2(cursor_x, row.Center().y - extent.y * 0.5f),
                       Tint(entry.disabled ? text_color_disabled : (is_cursor ? text_color_focus : text_color)),
                       entry.icon.c_str());
            cursor_x += extent.x + icon_gap * scale;
        }

        float right_width = 0.0f;
        if (entry.submenu >= 0) {
            right_width += 22.0f * scale;
        }
        if (!entry.shortcut.empty()) {
            right_width +=
                Draw::MeasureText(nullptr, Theme::kFontSmall * scale, entry.shortcut.c_str(), 0.0f).x + 14.0f * scale;
        }
        if (!entry.value.empty()) {
            right_width +=
                Draw::MeasureText(nullptr, Theme::kFontSmall * scale, entry.value.c_str(), 0.0f).x + 14.0f * scale;
        }

        const ImU32 color = entry.disabled ? text_color_disabled : (is_cursor ? text_color_focus : text_color);
        const char* shown =
            Draw::Ellipsize(nullptr, font, entry.label.c_str(), row.max.x - cursor_x - right_width - 12.0f * scale);
        Draw::Text(dl, nullptr, font, ImVec2(cursor_x, row.Center().y - font * 0.5f), Tint(color), shown);

        float right_x = row.max.x - 16.0f * scale;
        if (!entry.shortcut.empty()) {
            const ImVec2 extent = Draw::MeasureText(nullptr, Theme::kFontSmall * scale, entry.shortcut.c_str(), 0.0f);
            right_x -= extent.x;
            Draw::Text(dl, nullptr, Theme::kFontSmall * scale, ImVec2(right_x, row.Center().y - extent.y * 0.5f),
                       Tint(entry.disabled ? text_color_disabled : shortcut_color), entry.shortcut.c_str());
            right_x -= 14.0f * scale;
        }
        if (!entry.value.empty()) {
            const ImVec2 extent = Draw::MeasureText(nullptr, Theme::kFontSmall * scale, entry.value.c_str(), 0.0f);
            right_x -= extent.x;
            Draw::Text(dl, nullptr, Theme::kFontSmall * scale, ImVec2(right_x, row.Center().y - extent.y * 0.5f),
                       Tint(entry.disabled ? text_color_disabled : Theme::kTextPrimary), entry.value.c_str());
            right_x -= 14.0f * scale;
        }
        if (entry.submenu >= 0) {
            Draw::TriangleRight(dl, ImVec2(right_x - 6.0f * scale, row.Center().y), 10.0f * scale,
                                Tint(entry.disabled ? text_color_disabled : text_color));
        }
    }
}

} // namespace gui_dev::cv
