#include "component_view/components/VirtualKeyboard.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

int CharCountOf(const std::string& value) {
    int count = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if ((static_cast<unsigned char>(value[i]) & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

std::size_t ByteOffsetOf(const std::string& value, int char_index) {
    if (char_index <= 0) {
        return 0;
    }
    int seen = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if ((static_cast<unsigned char>(value[i]) & 0xC0) != 0x80) {
            if (seen == char_index) {
                return i;
            }
            ++seen;
        }
    }
    return value.size();
}

// 每页的字符矩阵：10 列 × 4 行 + 功能行
const char* const kLetters[4][10] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", "@"},
    {"z", "x", "c", "v", "b", "n", "m", ".", "-", "_"},
};

const char* const kSymbols[4][10] = {
    {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")"},
    {"-", "_", "=", "+", "[", "]", "{", "}", "\\", "|"},
    {";", ":", "'", "\"", ",", ".", "<", ">", "/", "?"},
    {"`", "~", "\xE2\x82\xAC", "\xC2\xA3", "\xC2\xA5", "\xE2\x80\xA2", "\xC2\xB0", "\xC2\xA9", "\xC2\xAE", "\xE2\x84\xA2"},
};

const char* const kNumbers[4][10] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"-", "+", "*", "/", "=", "%", ".", ",", "(", ")"},
    {"[", "]", "{", "}", "<", ">", ";", ":", "'", "\""},
    {"`", "~", "!", "?", "#", "&", "|", "\\", "^", "$"},
};

// 错位行：让键盘看起来像真实 QWERTY 而不是矩阵
const float kStagger[4] = {0.0f, 0.0f, 0.5f, 1.5f};

} // namespace

VirtualKeyboard::VirtualKeyboard() : Widget("virtual_keyboard") {
    focusable = true;
    capture_horizontal = true; // 键盘自己管方向键
    capture_vertical = true;
    overflow = Overflow::Hidden;
    padding = EdgeInsets::All(10.0f);
    corner_radius = Theme::kRadiusLarge;
    background = Theme::kBgSideBar;
    border = BorderStyle{1.0f, Theme::kBorderStrong, 0.0f};
}

void VirtualKeyboard::SetInitial(std::string text) {
    buffer = std::move(text);
    ClampBuffer();
    caret_ = CharCount();
    selection_start_ = selection_end_ = -1;
    layout_dirty_ = true;
}

void VirtualKeyboard::SetPrompt(std::string value) {
    prompt = std::move(value);
}

void VirtualKeyboard::Reset() {
    buffer.clear();
    caret_ = 0;
    selection_start_ = selection_end_ = -1;
    shift = false;
    page = Page::Letters;
    cursor_ = 0;
    layout_dirty_ = true;
}

int VirtualKeyboard::CharCount() const {
    return CharCountOf(buffer);
}

void VirtualKeyboard::ClampBuffer() {
    if (max_length > 0 && CharCount() > max_length) {
        buffer = buffer.substr(0, ByteOffsetOf(buffer, max_length));
    }
}

void VirtualKeyboard::SetCursorIndex(int value) {
    if (keys_.empty()) {
        cursor_ = 0;
        return;
    }
    cursor_ = static_cast<int>(Clampf(static_cast<float>(value), 0.0f, static_cast<float>(keys_.size()) - 1.0f));
}

std::string VirtualKeyboard::PageName() const {
    switch (page) {
    case Page::Letters:
        return "ABC";
    case Page::Symbols:
        return "符号";
    default:
        return "数字";
    }
}

std::string VirtualKeyboard::DisplayBuffer() const {
    if (password) {
        return std::string(static_cast<std::size_t>(CharCount()), '*');
    }
    return buffer;
}

void VirtualKeyboard::InsertText(const std::string& value) {
    if (value.empty()) {
        return;
    }
    if (HasSelection()) {
        buffer.erase(ByteOffsetOf(buffer, selection_start_),
                     ByteOffsetOf(buffer, selection_end_) - ByteOffsetOf(buffer, selection_start_));
        caret_ = selection_start_;
        selection_start_ = selection_end_ = -1;
    }
    const std::size_t at = ByteOffsetOf(buffer, caret_);
    buffer.insert(at, value);
    caret_ += CharCountOf(value);
    ClampBuffer();
    if (caret_ > CharCount()) {
        caret_ = CharCount();
    }
    layout_dirty_ = true;
    if (on_changed) {
        on_changed(buffer);
    }
}

void VirtualKeyboard::Backspace() {
    if (HasSelection()) {
        buffer.erase(ByteOffsetOf(buffer, selection_start_),
                     ByteOffsetOf(buffer, selection_end_) - ByteOffsetOf(buffer, selection_start_));
        caret_ = selection_start_;
        selection_start_ = selection_end_ = -1;
    } else if (caret_ > 0) {
        buffer.erase(ByteOffsetOf(buffer, caret_ - 1), ByteOffsetOf(buffer, caret_) - ByteOffsetOf(buffer, caret_ - 1));
        --caret_;
    } else {
        return;
    }
    layout_dirty_ = true;
    if (on_changed) {
        on_changed(buffer);
    }
}

void VirtualKeyboard::DeleteForward() {
    if (HasSelection()) {
        Backspace();
        return;
    }
    if (caret_ >= CharCount()) {
        return;
    }
    buffer.erase(ByteOffsetOf(buffer, caret_), ByteOffsetOf(buffer, caret_ + 1) - ByteOffsetOf(buffer, caret_));
    layout_dirty_ = true;
    if (on_changed) {
        on_changed(buffer);
    }
}

void VirtualKeyboard::MoveCaret(int delta) {
    caret_ = static_cast<int>(Clampf(static_cast<float>(caret_ + delta), 0.0f, static_cast<float>(CharCount())));
    selection_start_ = selection_end_ = -1;
}

void VirtualKeyboard::SelectAll() {
    selection_start_ = 0;
    selection_end_ = CharCount();
    caret_ = selection_end_;
}

void VirtualKeyboard::ClearBuffer() {
    buffer.clear();
    caret_ = 0;
    selection_start_ = selection_end_ = -1;
    layout_dirty_ = true;
    if (on_changed) {
        on_changed(buffer);
    }
}

ImVec2 VirtualKeyboard::MeasureContent(const ImVec2& available) {
    (void)available;
    // 尺寸由使用方给定；这里给一个合理的默认值
    const float width = static_cast<float>(columns) * 34.0f + static_cast<float>(columns - 1) * key_gap + 24.0f;
    return ImVec2(width, preview_height + 5.0f * (key_height + key_gap) + 24.0f);
}

void VirtualKeyboard::BuildLayout(const Rect& content) {
    keys_.clear();
    const int rows = 4;
    const float gap = key_gap * DrawScale();
    const float available_width = content.Width();
    const float key_width = (available_width - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float key_h = key_height * DrawScale();

    const float grid_top = content.min.y + (show_preview ? (preview_height * DrawScale() + 8.0f * DrawScale()) : 0.0f);

    const char* const(*table)[10] = kLetters;
    if (page == Page::Symbols) {
        table = kSymbols;
    } else if (page == Page::Numbers) {
        table = kNumbers;
    }

    auto add_key = [&](Key::Kind kind, std::string label, std::string insert, int row, int column, float x, float y) {
        Key key;
        key.kind = kind;
        key.label = std::move(label);
        key.insert = std::move(insert);
        key.row = row;
        key.column = column;
        key.rect = Rect::FromPosSize(ImVec2(x, y), ImVec2(key_width, key_h));
        keys_.push_back(std::move(key));
    };

    for (int row = 0; row < rows; ++row) {
        const float y = grid_top + static_cast<float>(row) * (key_h + gap);
        const float row_x = content.min.x + kStagger[row] * (key_width + gap);
        for (int column = 0; column < columns; ++column) {
            const float x = row_x + static_cast<float>(column) * (key_width + gap);
            std::string insert = table[row][column];
            std::string label = insert;
            if (shift && label.size() == 1 && label[0] >= 'a' && label[0] <= 'z') {
                label[0] = static_cast<char>(label[0] - 'a' + 'A');
                insert = label;
            }
            add_key(Key::Kind::Character, label, insert, row, column, x, y);
        }
    }

    // 功能行：10 个键与上方网格对齐
    const float function_y = grid_top + static_cast<float>(rows) * (key_h + gap);
    const Key::Kind kinds[10] = {Key::Kind::Shift,    Key::Kind::Space,      Key::Kind::CaretLeft, Key::Kind::CaretRight,
                                 Key::Kind::Backspace, Key::Kind::Delete,    Key::Kind::SelectAll, Key::Kind::Clear,
                                 Key::Kind::Cancel,    Key::Kind::Confirm};
    const char* labels[10] = {"SHIFT", "SPACE", "<", ">", "BKSP", "DEL", "SEL", "CLR", "取消", "确定"};
    for (int column = 0; column < 10; ++column) {
        const float x = content.min.x + static_cast<float>(column) * (key_width + gap);
        add_key(kinds[column], labels[column], std::string(), rows, column, x, function_y);
    }

    if (cursor_ >= static_cast<int>(keys_.size())) {
        cursor_ = static_cast<int>(keys_.size()) - 1;
    }
    layout_dirty_ = false;
}

void VirtualKeyboard::FocusKey(int index, bool animate) {
    if (keys_.empty()) {
        return;
    }
    cursor_ = static_cast<int>(Clampf(static_cast<float>(index), 0.0f, static_cast<float>(keys_.size()) - 1.0f));
    const Key& key = keys_[static_cast<std::size_t>(cursor_)];
    if (!animate) {
        focus_x_ = key.rect.min.x;
        focus_y_ = key.rect.min.y;
        focus_w_ = key.rect.Width();
        focus_h_ = key.rect.Height();
    }
}

void VirtualKeyboard::MoveFocus(int dx, int dy) {
    if (keys_.empty()) {
        return;
    }
    const Key& from = keys_[static_cast<std::size_t>(cursor_)];
    const ImVec2 center = from.rect.Center();
    const ImVec2 dir(static_cast<float>(dx), static_cast<float>(dy));
    int best = -1;
    float best_score = FLT_MAX;
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (static_cast<int>(i) == cursor_) {
            continue;
        }
        const ImVec2 target = keys_[i].rect.Center();
        const ImVec2 delta(target.x - center.x, target.y - center.y);
        const float along = delta.x * dir.x + delta.y * dir.y;
        if (along <= 1.0f) {
            continue;
        }
        const float perpendicular = Absf(dx != 0 ? delta.y : delta.x);
        const float score = along + perpendicular * 2.0f;
        if (score < best_score) {
            best_score = score;
            best = static_cast<int>(i);
        }
    }
    if (best >= 0) {
        cursor_ = best;
    }
}

void VirtualKeyboard::ActivateKey(const Key& key) {
    switch (key.kind) {
    case Key::Kind::Character:
        InsertText(key.insert);
        last_char_ = key.insert;
        flash_ = 1.0f;
        if (shift) {
            shift = false; // 与主机软键盘一致：输入一个大写后自动回到小写
            layout_dirty_ = true;
        }
        break;
    case Key::Kind::Shift:
        shift = !shift;
        layout_dirty_ = true;
        break;
    case Key::Kind::Space:
        InsertText(" ");
        break;
    case Key::Kind::CaretLeft:
        MoveCaret(-1);
        break;
    case Key::Kind::CaretRight:
        MoveCaret(1);
        break;
    case Key::Kind::Backspace:
        Backspace();
        break;
    case Key::Kind::Delete:
        DeleteForward();
        break;
    case Key::Kind::SelectAll:
        SelectAll();
        break;
    case Key::Kind::Clear:
        ClearBuffer();
        break;
    case Key::Kind::Cancel:
        if (on_cancel) {
            on_cancel();
        }
        break;
    case Key::Kind::Confirm:
        if (on_accept) {
            on_accept(buffer);
        }
        break;
    }
}

bool VirtualKeyboard::OnPadAction(InputAction action) {
    if (keys_.empty()) {
        return false;
    }
    switch (action) {
    case InputAction::Up:
        MoveFocus(0, -1);
        return true;
    case InputAction::Down:
        MoveFocus(0, 1);
        return true;
    case InputAction::Left:
        MoveFocus(-1, 0);
        return true;
    case InputAction::Right:
        MoveFocus(1, 0);
        return true;
    case InputAction::Confirm:
        ActivateKey(keys_[static_cast<std::size_t>(cursor_)]);
        return true;
    case InputAction::Cancel:
        if (on_cancel) {
            on_cancel();
        }
        return true;
    case InputAction::ActionX:
        Backspace();
        return true;
    case InputAction::ActionY:
        InsertText(" ");
        return true;
    case InputAction::Menu:
        if (on_accept) {
            on_accept(buffer);
        }
        return true;
    case InputAction::Minus:
        Backspace();
        return true;
    case InputAction::PageLeft:
        page = page == Page::Letters ? Page::Numbers : (page == Page::Numbers ? Page::Symbols : Page::Letters);
        layout_dirty_ = true;
        return true;
    case InputAction::PageRight:
        page = page == Page::Letters ? Page::Symbols : (page == Page::Symbols ? Page::Numbers : Page::Letters);
        layout_dirty_ = true;
        return true;
    case InputAction::TriggerLeft:
        shift = !shift;
        layout_dirty_ = true;
        return true;
    case InputAction::TriggerRight:
        page = page == Page::Letters ? Page::Symbols : (page == Page::Symbols ? Page::Numbers : Page::Letters);
        layout_dirty_ = true;
        return true;
    default:
        return false;
    }
}

void VirtualKeyboard::OnUpdate(float dt) {
    flash_ = SmoothTo(flash_, 0.0f, 6.0f, dt);
    press_mix_ = SmoothTo(press_mix_, (Global::pad.Held(InputAction::Confirm) && focused) ? 1.0f : 0.0f, 22.0f, dt);
    if (keys_.empty()) {
        return;
    }
    const Key& key = keys_[static_cast<std::size_t>(cursor_)];
    focus_x_ = SmoothTo(focus_x_, key.rect.min.x, 22.0f, dt);
    focus_y_ = SmoothTo(focus_y_, key.rect.min.y, 22.0f, dt);
    focus_w_ = SmoothTo(focus_w_, key.rect.Width(), 22.0f, dt);
    focus_h_ = SmoothTo(focus_h_, key.rect.Height(), 22.0f, dt);
}

void VirtualKeyboard::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    // 可用尺寸变化（第一次显形 / 窗口缩放 / 行列调整）时重建键位
    if (Absf(content.Width() - layout_size_.x) > 0.5f || Absf(content.Height() - layout_size_.y) > 0.5f) {
        layout_dirty_ = true;
        layout_size_ = ImVec2(content.Width(), content.Height());
    }
    if (layout_dirty_) {
        BuildLayout(content);
        if (focus_w_ <= 0.0f) {
            FocusKey(cursor_, false);
        }
    }

    // 预览区：提示 + 当前文本 + 光标
    if (show_preview) {
        const Rect preview = Rect::FromPosSize(content.min, ImVec2(content.Width(), preview_height * scale));
        Draw::RoundedRectFilled(dl, preview, Tint(preview_bg), Theme::kRadiusSmall * scale, Theme::kRadiusSmall * scale,
                                Theme::kRadiusSmall * scale, Theme::kRadiusSmall * scale);
        Draw::RoundedRectOutline(dl, preview, Tint(Theme::kBorder), 1.0f * scale, Theme::kRadiusSmall * scale,
                                 Theme::kRadiusSmall * scale, Theme::kRadiusSmall * scale, Theme::kRadiusSmall * scale);

        const float prompt_size = Theme::kFontTiny * scale;
        const float text_size = Theme::kFontHeader * scale;
        const float text_y = preview.Center().y - text_size * 0.5f + 6.0f * scale;
        if (!prompt.empty()) {
            Draw::Text(dl, nullptr, prompt_size, ImVec2(preview.min.x + 9.0f * scale, preview.min.y + 4.0f * scale),
                       Tint(Theme::kTextMuted), prompt.c_str());
        }
        const std::string display = DisplayBuffer();
        const std::string head = display.substr(0, ByteOffsetOf(display, caret_));
        const std::string tail = display.substr(ByteOffsetOf(display, caret_));
        const float head_width = Draw::MeasureText(nullptr, text_size, head.c_str(), 0.0f).x;
        const float start_x = preview.min.x + 10.0f * scale;
        Draw::Text(dl, nullptr, text_size, ImVec2(start_x, text_y), Tint(Theme::kTextBright), head.c_str());
        Draw::Text(dl, nullptr, text_size, ImVec2(start_x + head_width, text_y), Tint(Theme::kTextPrimary), tail.c_str());

        // 光标 + 光标动画（刚输入的字符闪一下）
        const float caret_x = start_x + head_width;
        const float blink = 0.55f + 0.45f * std::cos(flash_ * 3.0f);
        Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(caret_x, preview.min.y + 10.0f * scale),
                                                     ImVec2(2.5f * scale, preview.Height() - 20.0f * scale)),
                                Theme::Alpha(Theme::kAccent, 0.5f + 0.5f * blink), 1.0f * scale, 1.0f * scale,
                                1.0f * scale, 1.0f * scale);
        if (HasSelection()) {
            const float sel_start = start_x + Draw::MeasureText(
                                                   nullptr, text_size, display.substr(0, ByteOffsetOf(display, selection_start_)).c_str(), 0.0f)
                                                   .x;
            const float sel_end = start_x + Draw::MeasureText(
                                                 nullptr, text_size, display.substr(0, ByteOffsetOf(display, selection_end_)).c_str(), 0.0f)
                                                 .x;
            Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(sel_start, preview.min.y + 8.0f * scale),
                                                         ImVec2(sel_end - sel_start, preview.Height() - 16.0f * scale)),
                                    Theme::Alpha(Theme::kSelection, 0.55f), 2.0f * scale, 2.0f * scale, 2.0f * scale,
                                    2.0f * scale);
        }

        char info[96];
        std::snprintf(info, sizeof(info), "%s%s · %d/%d", PageName().c_str(), shift ? " · SHIFT" : "",
                      CharCount(), max_length);
        const ImVec2 info_extent = Draw::MeasureText(nullptr, prompt_size, info, 0.0f);
        Draw::Text(dl, nullptr, prompt_size,
                   ImVec2(preview.max.x - 12.0f * scale - info_extent.x, preview.min.y + 6.0f * scale),
                   Tint(Theme::kTextMuted), info);
    }

    // 键位
    const float key_font = Theme::kFontBody * scale;
    const float small_font = Theme::kFontSmall * scale;
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        const Key& key = keys_[static_cast<std::size_t>(i)];
        const bool is_focus = (static_cast<int>(i) == cursor_);
        const bool is_function = key.row >= 4;
        const bool is_shift = key.kind == Key::Kind::Shift && shift;
        ImU32 fill = is_function ? key_bg_function : key_bg;
        if (is_shift) {
            fill = Theme::Mix(fill, Theme::kAccent, 0.85f);
        }
        if (is_focus) {
            fill = Theme::Mix(fill, key_bg_focus, 0.9f);
        }
        const float radius = 4.0f * scale;
        const float grow = is_focus ? 2.0f * scale * (0.6f + 0.4f * press_mix_) : 0.0f;
        const Rect rect = key.rect.Expanded(grow);
        Draw::RoundedRectFilled(dl, rect, Tint(fill), radius, radius, radius, radius);
        Draw::RoundedRectOutline(dl, rect, Tint(Theme::kBorder), 1.0f * scale, radius, radius, radius, radius);

        const float font = (key.kind == Key::Kind::Character) ? key_font : small_font;
        const ImVec2 extent = Draw::MeasureText(nullptr, font, key.label.c_str(), 0.0f);
        Draw::Text(dl, nullptr, font,
                   ImVec2(rect.Center().x - extent.x * 0.5f, rect.Center().y - extent.y * 0.5f),
                   Tint(is_focus ? key_text_focus : key_text), key.label.c_str());
    }

    // 焦点框（平滑移动，跟随按键）
    if (focused && focus_w_ > 0.0f) {
        const Rect ring = Rect::FromPosSize(ImVec2(focus_x_, focus_y_), ImVec2(focus_w_, focus_h_)).Expanded(2.0f * scale);
        Draw::RoundedRectOutline(dl, ring, Theme::Alpha(Theme::kAccent, 0.95f), 2.5f * scale, 8.0f * scale,
                                 8.0f * scale, 8.0f * scale, 8.0f * scale);
    }
}

} // namespace gui_dev::cv
