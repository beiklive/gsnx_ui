#include "component_view/components/Choice.h"

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

float ResolvedFont(const float value) {
    return value > 0.0f ? value : Theme::kFontBody;
}

} // namespace

// ============================================================== Checkbox =====

Checkbox::Checkbox() : Widget("checkbox") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = true;
    focus_frame_offset = 3.0f;
    // 复选框通常做列表行用，给一个默认高度；宽度按内容自适应
    size.y = 0.0f;
}

Checkbox::Checkbox(std::string value) : Checkbox() {
    caption = std::move(value);
}

Checkbox& Checkbox::setChecked(bool value, bool notify) {
    if (checked == value) {
        return *this;
    }
    checked = value;
    if (notify) {
        emit toggled(checked);
    }
    return *this;
}

Checkbox& Checkbox::setCaption(std::string value) {
    caption = std::move(value);
    return *this;
}

Checkbox& Checkbox::setBoxSize(float value) {
    box_size = value;
    return *this;
}

Checkbox& Checkbox::setColors(ImVec4 box_fill, ImVec4 text) {
    check_color = box_fill;
    text_color = text;
    text_color_follows_theme = false;
    return *this;
}

ImVec2 Checkbox::MeasureContent(const ImVec2& available) {
    (void)available;
    const float size = ResolvedFont(font_size);
    const ImVec2 text = Draw::MeasureText(Draw::CurrentFont(), size, caption.c_str(), 0.0f);
    return ImVec2(box_size + gap + text.x, Maxf(box_size, text.y));
}

void Checkbox::OnUpdate(float dt) {
    if (check_mix_ < 0.0f) {
        check_mix_ = checked ? 1.0f : 0.0f; // 第一帧直接对齐
    }
    check_mix_ = Anim::SmoothTo(check_mix_, checked ? 1.0f : 0.0f, animation_speed, dt);
}

void Checkbox::Activate() {
    setChecked(!checked, true);
}

void Checkbox::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float opacity = EffectiveOpacity();
    const ImVec4 accent = check_color.w <= 0.0f ? Theme::kAccent : check_color;
    const ImU32 box_bg = Theme::Alpha(Theme::U32(accent), opacity * (0.35f + 0.65f * check_mix_));
    const ImU32 border_color = Theme::Alpha(Theme::U32(Theme::kBorderStrong), opacity);

    const float side = Minf(box_size, content.Height());
    const Rect box{ImVec2(content.min.x, content.min.y + (content.Height() - side) * 0.5f),
                   ImVec2(content.min.x + side, content.min.y + (content.Height() - side) * 0.5f + side)};
    Draw::RoundedRectFilled(dl, box, check_mix_ > 0.02f ? box_bg : Theme::Alpha(Theme::U32(Theme::kBgInput), opacity),
                            corner, corner, corner, corner);
    Draw::RoundedRectOutline(dl, box, check_mix_ > 0.02f ? Theme::Alpha(Theme::U32(accent), opacity) : border_color,
                             1.0f, corner, corner, corner, corner);
    if (check_mix_ > 0.05f) {
        Draw::CheckMark(dl, box, Theme::Alpha(Theme::U32(Theme::kWhite), opacity), 2.5f, check_mix_);
    }

    const float font_px = ResolvedFont(this->font_size);
    const float text_x = box.max.x + gap;
    const float text_height = Draw::MeasureText(Draw::CurrentFont(), font_px, "Ag", 0.0f).y;
    const float text_y = content.min.y + (content.Height() - text_height) * 0.5f;
    Draw::Text(dl, Draw::CurrentFont(), font_px, ImVec2(text_x, text_y),
               Theme::Alpha(Theme::U32(text_color), opacity),
               Draw::Ellipsize(Draw::CurrentFont(), font_px, caption.c_str(), Maxf(content.max.x - text_x, 1.0f)),
               0.0f);
}

void Checkbox::OnThemeChanged() {
    if (text_color_follows_theme) {
        text_color = Theme::kTextPrimary;
    }
    focus_frame_color = Theme::U32(Theme::kAccent);
}

// ============================================================ RadioGroup =====

RadioGroup::RadioGroup() : Widget("radio_group") {
    focusable = true;
    focus_on_hover = true;
    focus_only_self = true; // 组内自己做导航
    focus_frame = true;
    focus_frame_offset = 3.0f;
    SyncCapture();
}

RadioGroup::RadioGroup(std::vector<std::string> values) : RadioGroup() {
    setOptions(std::move(values), 0);
}

void RadioGroup::SyncCapture() {
    // 纵向组吃掉上下键（组内切换），左右键仍可离开；横向组相反
    capture_vertical = orientation == Orientation::Vertical;
    capture_horizontal = orientation == Orientation::Horizontal;
}

RadioGroup& RadioGroup::setOptions(std::vector<std::string> values, int start_index) {
    options = std::move(values);
    const int total = count();
    if (total <= 0) {
        index = 0;
    } else if (start_index < 0) {
        index = 0;
    } else if (start_index >= total) {
        index = total - 1;
    } else {
        index = start_index;
    }
    mark_mix_ = 1.0f; // 建好就位，不播动画
    return *this;
}

RadioGroup& RadioGroup::setIndex(int value, bool notify) {
    SelectAt(value, notify);
    return *this;
}

RadioGroup& RadioGroup::setOrientation(Orientation value) {
    orientation = value;
    SyncCapture();
    return *this;
}

RadioGroup& RadioGroup::setColors(ImVec4 mark, ImVec4 text) {
    mark_color = mark;
    text_color = text;
    text_color_follows_theme = false;
    return *this;
}

const char* RadioGroup::currentOption() const {
    if (index < 0 || index >= count()) {
        return "";
    }
    return options[static_cast<std::size_t>(index)].c_str();
}

void RadioGroup::SelectAt(int next, bool notify) {
    if (count() <= 0) {
        return;
    }
    const int clamped = next < 0 ? 0 : (next >= count() ? count() - 1 : next);
    if (clamped == index && mark_mix_ >= 0.0f) {
        return;
    }
    if (clamped != index) {
        mark_mix_ = 0.0f; // 换项：圆点重新长出来
    }
    index = clamped;
    if (notify) {
        emit selectionChanged(index);
    }
}

Rect RadioGroup::ItemRect(const Rect& content, int item) const {
    if (orientation == Orientation::Vertical) {
        const float y = content.min.y + static_cast<float>(item) * (item_height + item_gap);
        return Rect{ImVec2(content.min.x, y), ImVec2(content.max.x, y + item_height)};
    }
    const float x = content.min.x + static_cast<float>(item) * (horizontal_item_width + item_gap);
    const float width = Minf(horizontal_item_width, Maxf(content.max.x - x, 0.0f));
    return Rect{ImVec2(x, content.min.y), ImVec2(x + width, content.min.y + Maxf(item_height, marker_size))};
}

ImVec2 RadioGroup::MeasureContent(const ImVec2& available) {
    const float font_px = ResolvedFont(this->font_size);
    float label_width = 0.0f;
    for (const std::string& option : options) {
        label_width = Maxf(label_width, Draw::MeasureText(Draw::CurrentFont(), font_px, option.c_str(), 0.0f).x);
    }
    const float row_width = marker_size + label_gap + label_width;
    if (orientation == Orientation::Vertical) {
        const float height = static_cast<float>(count()) * item_height +
                             static_cast<float>(Maxf(count() - 1, 0)) * item_gap;
        return ImVec2(Minf(row_width, Maxf(available.x, 1.0f)), Maxf(height, item_height));
    }
    const float width = static_cast<float>(count()) * horizontal_item_width +
                        static_cast<float>(Maxf(count() - 1, 0)) * item_gap;
    return ImVec2(Minf(width, Maxf(available.x, 1.0f)), Maxf(item_height, marker_size));
}

void RadioGroup::OnUpdate(float dt) {
    if (mark_mix_ < 0.0f) {
        mark_mix_ = 1.0f; // 第一帧直接就位
    }
    mark_mix_ = Anim::SmoothTo(mark_mix_, 1.0f, animation_speed, dt);

    // 触摸/鼠标：点到哪一行就选哪一行（并让焦点落到组上，和手柄共用同一套状态）
    if (Global::mouse_available && Global::mouse_pressed[0] && enabled && visible) {
        const Rect content = content_rect;
        for (int i = 0; i < count(); ++i) {
            if (ItemRect(content, i).Contains(Global::mouse)) {
                RequestFocus();
                if (i == index) {
                    emit activated(i);
                } else {
                    SelectAt(i, true);
                }
                break;
            }
        }
    }
}

bool RadioGroup::OnPadAction(InputAction action) {
    if (count() <= 0) {
        return false;
    }
    int step = 0;
    if (orientation == Orientation::Vertical) {
        if (action == InputAction::Up) {
            step = -1;
        } else if (action == InputAction::Down) {
            step = 1;
        }
    } else {
        if (action == InputAction::Left) {
            step = -1;
        } else if (action == InputAction::Right) {
            step = 1;
        }
    }
    if (step == 0) {
        return false;
    }
    int next = index + step;
    if (next < 0 || next >= count()) {
        if (!wrap) {
            return true; // 已经到头：消费掉按键，避免焦点跳出组
        }
        next = next < 0 ? count() - 1 : 0;
    }
    SelectAt(next, true);
    return true;
}

void RadioGroup::Activate() {
    if (index >= 0 && index < count()) {
        emit activated(index);
    }
}

void RadioGroup::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float opacity = EffectiveOpacity();
    const ImVec4 mark = mark_color.w <= 0.0f ? Theme::kAccent : mark_color;
    const float font_px = ResolvedFont(this->font_size);

    item_rects_.clear();
    for (int i = 0; i < count(); ++i) {
        const Rect row = ItemRect(content, i);
        item_rects_.push_back(row);
        const bool selected = i == index;

        const float side = Minf(marker_size, row.Height());
        const Rect marker{ImVec2(row.min.x, row.min.y + (row.Height() - side) * 0.5f),
                          ImVec2(row.min.x + side, row.min.y + (row.Height() - side) * 0.5f + side)};
        const float radius = side * 0.5f;
        const ImU32 ring = selected ? Theme::Alpha(Theme::U32(mark), opacity)
                                    : Theme::Alpha(Theme::U32(Theme::kBorderStrong), opacity);
        dl->AddCircle(marker.Center(), radius - 1.0f, ring, 24, 2.0f);
        if (selected) {
            const float dot = radius * 0.45f * Clampf(mark_mix_, 0.0f, 1.0f);
            if (dot > 0.5f) {
                dl->AddCircleFilled(marker.Center(), dot, Theme::Alpha(Theme::U32(mark), opacity), 24);
            }
        }

        const float text_x = marker.max.x + label_gap;
        const float text_height = Draw::MeasureText(Draw::CurrentFont(), font_px, "Ag", 0.0f).y;
        const float text_y = row.min.y + (row.Height() - text_height) * 0.5f;
        const ImU32 text_color_u32 = Theme::Alpha(Theme::U32(text_color), opacity * (selected ? 1.0f : 0.75f));
        Draw::Text(dl, Draw::CurrentFont(), font_px, ImVec2(text_x, text_y), text_color_u32,
                   Draw::Ellipsize(Draw::CurrentFont(), font_px, options[static_cast<std::size_t>(i)].c_str(),
                                   Maxf(row.max.x - text_x, 1.0f)),
                   0.0f);
    }
}

void RadioGroup::OnThemeChanged() {
    if (text_color_follows_theme) {
        text_color = Theme::kTextPrimary;
    }
    focus_frame_color = Theme::U32(Theme::kAccent);
}

} // namespace gui_dev::cv
