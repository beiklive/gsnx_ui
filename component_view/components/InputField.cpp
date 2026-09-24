#include "component_view/components/InputField.h"

#include <cmath>
#include <vector>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 每个字符的起始字节偏移（UTF-8），size = 字符数 + 1
std::vector<std::size_t> CharOffsets(const std::string& value) {
    std::vector<std::size_t> offsets;
    std::size_t index = 0;
    while (index <= value.size()) {
        offsets.push_back(index);
        if (index == value.size()) {
            break;
        }
        std::size_t next = index + 1;
        while (next < value.size() && (static_cast<unsigned char>(value[next]) & 0xC0) == 0x80) {
            ++next;
        }
        index = next;
    }
    return offsets;
}

} // namespace

InputField::InputField() : Widget("input_field") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = true;
    focus_frame_offset = 4.0f;
    focus_scale = 1.01f;
    padding = EdgeInsets::Symmetric(14.0f, 10.0f);
    corner_radius = Theme::kRadiusSmall;
}

InputField::InputField(std::string text, std::string initial) : InputField() {
    label = std::move(text);
    text = std::move(initial);
    cursor = CharCount();
}

InputField& InputField::SetLabel(std::string value) {
    label = std::move(value);
    return *this;
}

InputField& InputField::SetPlaceholder(std::string value) {
    placeholder = std::move(value);
    return *this;
}

InputField& InputField::SetPrefix(std::string value) {
    prefix = std::move(value);
    return *this;
}

InputField& InputField::SetSuffix(std::string value) {
    suffix = std::move(value);
    return *this;
}

InputField& InputField::SetMaxLength(int value) {
    max_length = value;
    return *this;
}

InputField& InputField::SetPassword(bool value) {
    password = value;
    return *this;
}

InputField& InputField::SetReadOnly(bool value) {
    read_only = value;
    return *this;
}

int InputField::CharCount() const {
    return static_cast<int>(CharOffsets(text).size()) - 1;
}

int InputField::CursorByteOffset() const {
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const int index = Clampf(static_cast<float>(cursor), 0.0f, static_cast<float>(offsets.size()) - 1.0f);
    return static_cast<int>(offsets[static_cast<std::size_t>(index)]);
}

void InputField::SetText(std::string value, bool notify) {
    text = std::move(value);
    if (max_length > 0 && CharCount() > max_length) {
        const std::vector<std::size_t> offsets = CharOffsets(text);
        text = text.substr(0, offsets[static_cast<std::size_t>(max_length)]);
    }
    cursor = CharCount();
    selection_start = selection_end = -1;
    if (notify && on_changed) {
        on_changed(*this, text);
    }
}

void InputField::Insert(const std::string& value) {
    if (read_only || value.empty()) {
        return;
    }
    DeleteSelection();
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const int index = Clampf(static_cast<float>(cursor), 0.0f, static_cast<float>(offsets.size()) - 1.0f);
    const std::size_t byte = offsets[static_cast<std::size_t>(index)];
    text.insert(byte, value);
    cursor += static_cast<int>(CharOffsets(value).size()) - 1;
    if (max_length > 0 && CharCount() > max_length) {
        const std::vector<std::size_t> trimmed = CharOffsets(text);
        text = text.substr(0, trimmed[static_cast<std::size_t>(max_length)]);
        cursor = max_length;
    }
    if (on_changed) {
        on_changed(*this, text);
    }
}

void InputField::Backspace() {
    if (read_only) {
        return;
    }
    if (HasSelection()) {
        DeleteSelection();
        return;
    }
    if (cursor <= 0) {
        return;
    }
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const std::size_t from = offsets[static_cast<std::size_t>(cursor - 1)];
    const std::size_t to = offsets[static_cast<std::size_t>(cursor)];
    text.erase(from, to - from);
    --cursor;
    if (on_changed) {
        on_changed(*this, text);
    }
}

void InputField::DeleteForward() {
    if (read_only || cursor >= CharCount()) {
        return;
    }
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const std::size_t from = offsets[static_cast<std::size_t>(cursor)];
    const std::size_t to = offsets[static_cast<std::size_t>(cursor + 1)];
    text.erase(from, to - from);
    if (on_changed) {
        on_changed(*this, text);
    }
}

void InputField::MoveCursor(int delta_chars) {
    cursor = static_cast<int>(Clampf(static_cast<float>(cursor + delta_chars), 0.0f, static_cast<float>(CharCount())));
    if (Global::pad.Held(InputAction::TriggerLeft) || Global::pad.Held(InputAction::TriggerRight)) {
        // 按住 ZL/ZR 时移动光标则扩展选区
        if (selection_start < 0) {
            selection_start = cursor;
        }
        selection_end = cursor;
    } else {
        selection_start = selection_end = -1;
    }
}

void InputField::Clear() {
    if (read_only || text.empty()) {
        return;
    }
    text.clear();
    cursor = 0;
    selection_start = selection_end = -1;
    if (on_changed) {
        on_changed(*this, text);
    }
}

void InputField::SelectAll() {
    selection_start = 0;
    selection_end = CharCount();
    cursor = selection_end;
}

std::string InputField::SelectedText() const {
    if (!HasSelection()) {
        return std::string();
    }
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const std::size_t from = offsets[static_cast<std::size_t>(selection_start)];
    const std::size_t to = offsets[static_cast<std::size_t>(selection_end)];
    return text.substr(from, to - from);
}

void InputField::DeleteSelection() {
    if (!HasSelection()) {
        return;
    }
    const std::vector<std::size_t> offsets = CharOffsets(text);
    const std::size_t from = offsets[static_cast<std::size_t>(selection_start)];
    const std::size_t to = offsets[static_cast<std::size_t>(selection_end)];
    text.erase(from, to - from);
    cursor = selection_start;
    selection_start = selection_end = -1;
    if (on_changed) {
        on_changed(*this, text);
    }
}

ImVec2 InputField::MeasureContent(const ImVec2& available) {
    (void)available;
    const float size = ResolvedFontSize();
    const ImVec2 text_extent =
        Draw::MeasureText(nullptr, size, text.empty() ? placeholder.c_str() : text.c_str(), 0.0f);
    float width = text_extent.x + 40.0f;
    if (!prefix.empty()) {
        width += Draw::MeasureText(nullptr, size, prefix.c_str(), 0.0f).x + 6.0f;
    }
    if (!suffix.empty()) {
        width += Draw::MeasureText(nullptr, size, suffix.c_str(), 0.0f).x + 6.0f;
    }
    const float box_height = Maxf(Theme::kControlHeight, text_extent.y + 20.0f);
    if (!label.empty()) {
        const float label_size = Theme::kFontSmall;
        return ImVec2(Maxf(width, Draw::MeasureText(nullptr, label_size, label.c_str(), 0.0f).x),
                      box_height + label_size + label_gap);
    }
    return ImVec2(width, box_height);
}

void InputField::OnUpdate(float dt) {
    cursor_blink_ += dt;
    if (cursor_blink_ > 1.2f) {
        cursor_blink_ -= 1.2f;
    }
    focus_edge_ += ((focused ? 1.0f : 0.0f) - focus_edge_) * (1.0f - std::exp(-16.0f * dt));
}

bool InputField::OnPadAction(InputAction action) {
    switch (action) {
    case InputAction::Confirm:
        if (!read_only) {
            editing = true;
            if (on_edit_requested) {
                on_edit_requested(*this);
            }
            return true;
        }
        return false;
    case InputAction::ActionX:
        Clear();
        return true;
    case InputAction::ActionY:
        Backspace();
        return true;
    case InputAction::Cancel:
        if (editing) {
            editing = false;
            return true;
        }
        return false;
    default:
        return false;
    }
}

void InputField::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float size = ResolvedFontSize() * scale;
    const float label_size = Theme::kFontSmall * scale;

    Rect box = content;
    if (!label.empty()) {
        Draw::Text(dl, nullptr, label_size, ImVec2(content.min.x, content.min.y),
                   Tint(editing ? Theme::kAccent : Theme::kTextMuted), label.c_str());
        box.min.y += label_size + label_gap * scale;
    }

    const float radius = box_radius * scale;
    Draw::RoundedRectFilled(dl, box, Tint(box_bg), radius, radius, radius, radius);
    Draw::RoundedRectOutline(dl, box, Theme::Alpha(Theme::Mix(box_border, box_border_focus, focus_edge_),
                                                   EffectiveOpacity() * (0.8f + 0.2f * focus_edge_)),
                             (1.0f + focus_edge_) * scale, radius, radius, radius, radius);

    float cursor_x = box.min.x + 12.0f * scale;
    const float center_y = box.Center().y;

    if (!prefix.empty()) {
        const ImVec2 extent = Draw::MeasureText(nullptr, size, prefix.c_str(), 0.0f);
        Draw::Text(dl, nullptr, size, ImVec2(cursor_x, center_y - extent.y * 0.5f), Tint(Theme::kTextMuted),
                   prefix.c_str());
        cursor_x += extent.x + 6.0f * scale;
    }

    std::string display = text;
    if (password) {
        display.assign(static_cast<std::size_t>(CharCount()), '*');
    }
    const bool empty = display.empty();
    if (empty && !placeholder.empty()) {
        Draw::Text(dl, nullptr, size, ImVec2(cursor_x, center_y - size * 0.5f), Tint(placeholder_color),
                   placeholder.c_str());
    } else {
        // 选区高亮 + 光标位置（都以字符索引换算到像素）
        auto byte_offset_at = [&](int char_index) {
            const std::vector<std::size_t> offsets = CharOffsets(display);
            const int clamped =
                static_cast<int>(Clampf(static_cast<float>(char_index), 0.0f, static_cast<float>(offsets.size()) - 1.0f));
            return offsets[static_cast<std::size_t>(clamped)];
        };
        auto x_at = [&](int char_index) {
            return cursor_x +
                   Draw::MeasureText(nullptr, size, display.substr(0, byte_offset_at(char_index)).c_str(), 0.0f).x;
        };

        if (HasSelection()) {
            const float start_x = x_at(selection_start);
            const float end_x = x_at(selection_end);
            Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(start_x, box.min.y + 6.0f * scale),
                                                         ImVec2(Maxf(end_x - start_x, 2.0f * scale),
                                                                box.Height() - 12.0f * scale)),
                                    Theme::Alpha(selection_color, 0.85f * EffectiveOpacity()), 2.0f * scale,
                                    2.0f * scale, 2.0f * scale, 2.0f * scale);
        }

        // 长文本时把光标保持在可见范围内
        const float available = box.Width() - (cursor_x - box.min.x) - 12.0f * scale -
                                (suffix.empty() ? 0.0f : Draw::MeasureText(nullptr, size, suffix.c_str(), 0.0f).x + 8.0f * scale);
        const char* shown = Draw::Ellipsize(nullptr, size, display.c_str(), available);
        Draw::Text(dl, nullptr, size, ImVec2(cursor_x, center_y - size * 0.5f), Tint(text_color), shown);

        // 光标（只在编辑或聚焦时闪）
        if (focused || editing) {
            const float blink = 0.5f + 0.5f * std::cos(cursor_blink_ * 5.2f);
            const float caret = x_at(cursor);
            if (caret <= box.max.x - 10.0f * scale) {
                Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(caret, box.min.y + 8.0f * scale),
                                                             ImVec2(2.0f * scale, box.Height() - 16.0f * scale)),
                                        Theme::Alpha(cursor_color, 0.35f + 0.65f * blink), 1.0f * scale, 1.0f * scale,
                                        1.0f * scale, 1.0f * scale);
            }
        }
    }

    if (!suffix.empty()) {
        const ImVec2 extent = Draw::MeasureText(nullptr, size, suffix.c_str(), 0.0f);
        Draw::Text(dl, nullptr, size, ImVec2(box.max.x - 12.0f * scale - extent.x, center_y - extent.y * 0.5f),
                   Tint(Theme::kTextMuted), suffix.c_str());
    }
    if (!hint.empty()) {
        const ImVec2 extent = Draw::MeasureText(nullptr, label_size, hint.c_str(), 0.0f);
        Draw::Text(dl, nullptr, label_size,
                   ImVec2(box.max.x - 10.0f * scale - extent.x, box.max.y - label_size - 6.0f * scale),
                   Tint(Theme::kTextMuted), hint.c_str());
    }
    if (read_only) {
        Draw::Text(dl, nullptr, label_size, ImVec2(box.min.x + 12.0f * scale, box.max.y - label_size - 6.0f * scale),
                   Tint(Theme::kTextMuted), "只读");
    }
}

} // namespace gui_dev::cv
