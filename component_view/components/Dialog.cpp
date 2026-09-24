#include "component_view/components/Dialog.h"

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

Dialog::Dialog() : Widget("dialog") {
    focusable = true;
    focus_frame = true;
    focus_frame_offset = 6.0f;
    visible = false;
    padding = EdgeInsets::All(24.0f);
    corner_radius = Theme::kRadiusLarge;
    background = Theme::kBgSideBar;
    border = BorderStyle{1.0f, Theme::kBorderStrong, 0.0f};
    shadow = ShadowStyle::Soft(40.0f);
    shadow.offset = ImVec2(0.0f, 14.0f);
    shadow.color = IM_COL32(0, 0, 0, 180);
    layout = LayoutMode::Free;
    z_order = 1000;
}

Dialog::Dialog(std::string dialog_title, std::string text) : Dialog() {
    title = std::move(dialog_title);
    message = std::move(text);
}

Dialog& Dialog::SetTitle(std::string value) {
    title = std::move(value);
    return *this;
}

Dialog& Dialog::SetMessage(std::string value) {
    message = std::move(value);
    return *this;
}

Dialog& Dialog::SetIcon(std::string glyph) {
    icon = std::move(glyph);
    return *this;
}

Dialog& Dialog::SetButtons(std::vector<std::string> labels, int default_index, int cancel_index) {
    buttons = std::move(labels);
    default_button = default_index;
    cancel_button = cancel_index;
    cursor_ = default_index;
    return *this;
}

void Dialog::Open() {
    open_ = true;
    visible = true;
    cursor_ = default_button;
    cursor_anim_ = static_cast<float>(cursor_);
    Global::modal = this;
    Global::SetFocus(this);
}

void Dialog::Close(int result) {
    if (!open_) {
        return;
    }
    open_ = false;
    if (Global::modal == this) {
        Global::modal = nullptr;
    }
    if (on_result) {
        on_result(*this, result);
    }
}

void Dialog::SetFocusedButton(int value) {
    if (buttons.empty()) {
        return;
    }
    cursor_ = static_cast<int>(
        Clampf(static_cast<float>(value), 0.0f, static_cast<float>(buttons.size()) - 1.0f));
}

Rect Dialog::ButtonRect(int index) const {
    const float scale = DrawScale();
    const float width = button_width * scale;
    const float height = button_height * scale;
    const float gap = button_gap * scale;
    const Rect content = DrawContentRect();
    const float total_width = static_cast<float>(buttons.size()) * width + static_cast<float>(buttons.size() - 1) * gap;
    const float start_x = content.Center().x - total_width * 0.5f;
    const float y = content.max.y - height;
    if (vertical_buttons) {
        const float total_height = static_cast<float>(buttons.size()) * height +
                                   static_cast<float>(buttons.size() - 1) * gap;
        const float start_y = content.max.y - total_height;
        return Rect::FromPosSize(ImVec2(content.Center().x - width * 0.5f,
                                        start_y + static_cast<float>(index) * (height + gap)),
                                 ImVec2(width, height));
    }
    return Rect::FromPosSize(ImVec2(start_x + static_cast<float>(index) * (width + gap), y), ImVec2(width, height));
}

ImVec2 Dialog::MeasureContent(const ImVec2& available) {
    (void)available;
    const float text_size = Theme::kFontBody;
    const ImVec2 message_extent = Draw::MeasureText(nullptr, text_size, message.c_str(), 0.0f);
    const float width = Maxf(420.0f, message_extent.x + 80.0f);
    if (vertical_buttons) {
        return ImVec2(width, 150.0f + static_cast<float>(buttons.size()) * (button_height + button_gap));
    }
    return ImVec2(width, 150.0f + button_height);
}

void Dialog::OnUpdate(float dt) {
    open_mix_ = SmoothTo(open_mix_, open_ ? 1.0f : 0.0f, animation_speed, dt);
    cursor_anim_ = SmoothTo(cursor_anim_, static_cast<float>(cursor_), 18.0f, dt);
    press_mix_ = SmoothTo(press_mix_, (Global::pad.Held(InputAction::Confirm) && focused) ? 1.0f : 0.0f, 20.0f, dt);
    if (!open_ && open_mix_ < 0.02f) {
        visible = false; // 退出动画播完再隐藏
    }
    // 进入动画：缩放 + 上移
    const float scale = 0.9f + 0.1f * open_mix_;
    visual_scale = ImVec2(scale, scale);
    visual_translate = ImVec2(0.0f, (1.0f - open_mix_) * 18.0f);
}

bool Dialog::OnPadAction(InputAction action) {
    if (!open_) {
        return false;
    }
    switch (action) {
    case InputAction::Left:
        if (!vertical_buttons) {
            SetFocusedButton(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Right:
        if (!vertical_buttons) {
            SetFocusedButton(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Up:
        if (vertical_buttons) {
            SetFocusedButton(cursor_ - 1);
            return true;
        }
        return false;
    case InputAction::Down:
        if (vertical_buttons) {
            SetFocusedButton(cursor_ + 1);
            return true;
        }
        return false;
    case InputAction::Confirm:
        Close(cursor_);
        return true;
    case InputAction::Cancel:
        if (dismiss_on_cancel) {
            Close(cancel_button);
        }
        return true;
    default:
        return false;
    }
}

void Dialog::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float alpha = Clampf(open_mix_, 0.0f, 1.0f);

    if (!icon.empty()) {
        const float icon_size = Theme::kFontTitle * scale;
        Draw::Text(dl, nullptr, icon_size, ImVec2(content.min.x, content.min.y),
                   Tint(Theme::Alpha(Theme::kAccent, alpha)), icon.c_str());
    }
    if (!title.empty()) {
        const float title_size = Theme::kFontHeader * scale;
        const float x = icon.empty() ? content.min.x : content.min.x + Theme::kFontTitle * scale + 14.0f * scale;
        Draw::Text(dl, nullptr, title_size, ImVec2(x, content.min.y + 2.0f * scale),
                   Tint(Theme::Alpha(title_color, alpha)), title.c_str());
    }
    if (!message.empty()) {
        const float text_size = Theme::kFontBody * scale;
        Draw::Text(dl, nullptr, text_size, ImVec2(content.min.x, content.min.y + 52.0f * scale),
                   Tint(Theme::Alpha(message_color, alpha)), message.c_str());
    }

    for (std::size_t i = 0; i < buttons.size(); ++i) {
        const bool is_focus = (static_cast<int>(i) == cursor_);
        const Rect button = ButtonRect(static_cast<int>(i));
        const bool is_primary = static_cast<int>(i) == default_button;
        ImU32 fill = is_primary ? button_color : cancel_color;
        if (is_focus) {
            fill = Theme::Mix(fill, button_focus_color, 0.85f);
        }
        const float radius = Theme::kRadiusSmall * scale;
        const float press = is_focus ? press_mix_ : 0.0f;
        const Rect painted = button.Expanded(-press * 1.5f * scale);
        Draw::RoundedRectFilled(dl, painted, Tint(Theme::Alpha(fill, alpha)), radius, radius, radius, radius);
        if (is_focus) {
            Draw::RoundedRectOutline(dl, button.Expanded(3.0f * scale), Theme::Alpha(Theme::kAccent, alpha * 0.95f),
                                     2.0f * scale, radius + 3.0f * scale, radius + 3.0f * scale, radius + 3.0f * scale,
                                     radius + 3.0f * scale);
        }
        const float text_size = Theme::kFontBody * scale;
        const ImVec2 extent = Draw::MeasureText(nullptr, text_size, buttons[i].c_str(), 0.0f);
        Draw::Text(dl, nullptr, text_size,
                   ImVec2(button.Center().x - extent.x * 0.5f, button.Center().y - extent.y * 0.5f),
                   Tint(Theme::Alpha(is_primary ? Theme::kTextBright : Theme::kTextPrimary, alpha)),
                   buttons[i].c_str());
    }
}

} // namespace gui_dev::cv
