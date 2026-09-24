#include "component_view/components/Button.h"

#include <cstdio>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

// 右侧内容滚动切换用的 L / R 图标（任天堂私用区字形）
const char* kLeftKeyGlyph = Icons::Glyph(Icons::Button::L);
const char* kRightKeyGlyph = Icons::Glyph(Icons::Button::R);

} // namespace

// ---------------------------------------------------------------- 基类 ----

Button::Button() : Widget("button") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = false; // 用流光框代替基类的单色焦点框
    focus_scale = 1.0f;
    applyComponentStyle();
}

Button::Button(std::string value) : Button() {
    text = std::move(value);
}

Button& Button::applyComponentStyle() {
    const Global::ComponentStyle& style = Global::component_style;
    border.width = style.border_width;
    border.color = Theme::U32(style.border_color);
    corner_radius = style.corner_radius;
    shadow.enabled = true;
    shadow.offset = style.shadow_offset;
    shadow.blur = style.shadow_blur;
    shadow.color = Theme::U32(style.shadow_color);
    padding = EdgeInsets::All(style.content_padding);
    if (background == 0) {
        background = Theme::U32(Theme::kBgWidget);
    }
    return *this;
}

Button& Button::setIcon(std::string glyph) {
    icon = std::move(glyph);
    return *this;
}

Button& Button::setText(std::string value) {
    text = std::move(value);
    return *this;
}

Button& Button::setSubtitle(std::string value, bool visible) {
    subtitle = std::move(value);
    show_subtitle = visible;
    return *this;
}

Button& Button::showSubtitle(bool value) {
    show_subtitle = value;
    return *this;
}

Button& Button::setTextAlign(TextAlign value) {
    text_align = value;
    return *this;
}

Button& Button::setIconGap(float value) {
    icon_gap = value;
    return *this;
}

Button& Button::setFontSize(float main_size, float subtitle_size) {
    font_size = main_size;
    subtitle_size = subtitle_size;
    return *this;
}

Button& Button::setTextColors(ImVec4 main_color, ImVec4 sub_color) {
    text_color = main_color;
    subtitle_color = sub_color;
    return *this;
}

Button& Button::resize(float width, float height) {
    size = ImVec2(width, height);
    return *this;
}

Button& Button::moveTo(float x, float y) {
    position = ImVec2(x, y);
    return *this;
}

Button& Button::setContentPadding(float value) {
    padding = EdgeInsets::All(value);
    return *this;
}

Button& Button::setBorder(float width, ImVec4 color) {
    border.width = width;
    border.color = Theme::U32(color);
    return *this;
}

Button& Button::setCornerRadius(float value) {
    corner_radius = value;
    return *this;
}

Button& Button::setShadow(ImVec2 offset, float blur, ImVec4 color) {
    shadow.enabled = true;
    shadow.offset = offset;
    shadow.blur = blur;
    shadow.color = Theme::U32(color);
    return *this;
}

Button& Button::setFlowingFocus(bool enabled) {
    flowing_focus = enabled;
    return *this;
}

float Button::ResolvedFocusMargin() const {
    return focus_margin >= 0.0f ? focus_margin : Global::component_style.focus_margin;
}

float Button::ResolvedFocusWidth() const {
    return focus_width > 0.0f ? focus_width : Global::component_style.focus_width;
}

Button::LeftBlock Button::computeLeftBlock(const Rect& content) const {
    LeftBlock block;
    const float main_size = mainFontSize();
    const float sub_size = subFontSize();
    const ImVec2 main_extent = Draw::MeasureText(nullptr, main_size, text.c_str(), 0.0f);
    const ImVec2 sub_extent =
        show_subtitle ? Draw::MeasureText(nullptr, sub_size, subtitle.c_str(), 0.0f) : ImVec2(0.0f, 0.0f);
    const float text_w = Maxf(main_extent.x, sub_extent.x);
    const float text_h = main_extent.y + (show_subtitle ? sub_extent.y + 2.0f : 0.0f);

    // 图标是正方形，边长 = 内容区高度 → 到边框的上下左右留白完全相同
    const float icon_side = icon.empty() ? 0.0f : content.Height();
    const float gap = icon.empty() ? 0.0f : icon_gap;
    block.width = icon_side + gap + text_w;
    const float block_h = Maxf(icon_side, text_h);

    // 文字块整体垂直居中（有没有说明行都居中）
    const float block_x = (text_align == TextAlign::Center) ? content.Center().x - block.width * 0.5f
                                                            : content.min.x;
    const float block_y = content.Center().y - block_h * 0.5f;
    block.icon = Rect::FromPosSize(ImVec2(block_x, content.Center().y - icon_side * 0.5f),
                                  ImVec2(icon_side, icon_side));
    block.text = Rect::FromPosSize(ImVec2(block_x + icon_side + gap, block_y), ImVec2(text_w, text_h));
    return block;
}

void Button::drawLeftBlock(ImDrawList* dl, const LeftBlock& block) const {
    const float main_size = mainFontSize();
    const float sub_size = subFontSize();

    if (!icon.empty() && block.icon.Width() > 0.0f) {
        const float glyph_size = block.icon.Height() * 0.86f;
        const ImVec2 extent = Draw::MeasureText(nullptr, glyph_size, icon.c_str(), 0.0f);
        Draw::Text(dl, nullptr, glyph_size,
                   ImVec2(block.icon.Center().x - extent.x * 0.5f, block.icon.Center().y - extent.y * 0.5f),
                   Theme::U32(text_color), icon.c_str());
    }
    if (text.empty() && !show_subtitle) {
        return;
    }
    const ImVec2 main_extent = Draw::MeasureText(nullptr, main_size, text.c_str(), 0.0f);

    // 主文字在上、说明行在下，两块作为整体已经垂直居中
    const float main_y = show_subtitle ? block.text.min.y : block.text.Center().y - main_extent.y * 0.5f;
    Draw::Text(dl, nullptr, main_size, ImVec2(block.text.min.x, main_y), Theme::U32(text_color), text.c_str());
    if (show_subtitle) {
        Draw::Text(dl, nullptr, sub_size, ImVec2(block.text.min.x, main_y + main_extent.y + 2.0f),
                   Theme::U32(subtitle_color), subtitle.c_str());
    }
}

float Button::rightSideWidth() const {
    return 0.0f;
}

void Button::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    (void)dl;
    (void)right_rect;
}

ImVec2 Button::MeasureContent(const ImVec2& available) {
    (void)available;
    const Button* self = this;
    LeftBlock block = self->computeLeftBlock(Rect::FromPosSize(ImVec2(0.0f, 0.0f), ImVec2(0.0f, Theme::kControlHeight)));
    const float height = Maxf(Theme::kControlHeight, subFontSize() + mainFontSize() + 8.0f);
    const float width = block.width + (rightSideWidth() > 0.0f ? rightSideWidth() + 16.0f : 0.0f);
    return ImVec2(Maxf(width, 120.0f), height);
}

void Button::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const LeftBlock block = computeLeftBlock(content);
    drawLeftBlock(dl, block);

    const float right_w = rightSideWidth();
    if (right_w > 0.0f) {
        const Rect right_rect = Rect::FromPosSize(ImVec2(content.max.x - right_w, content.min.y),
                                                 ImVec2(right_w, content.Height()));
        drawRightSide(dl, right_rect);
    }
}

void Button::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    (void)content;
    if (!flowing_focus || focus_mix <= 0.01f) {
        return;
    }
    // 完整闭合的流光框：四边都在，颜色沿边框流动，与按钮留 margin（默认 2px）
    const float margin = ResolvedFocusMargin();
    const float width = ResolvedFocusWidth();
    const Rect ring = DrawRect().Expanded(margin);
    const float phase = Global::time * Global::component_style.focus_flow_speed + focus_phase_offset;
    const float saturation =
        focus_saturation >= 0.0f ? focus_saturation : Global::component_style.focus_saturation;
    const float brightness =
        focus_brightness >= 0.0f ? focus_brightness : Global::component_style.focus_brightness;
    // 外扩后的圆角 = 按钮圆角 + 外扩量，这样流光框的圆角跟按钮轮廓平行
    Draw::FlowingRing(dl, ring, width, phase, saturation, brightness, focus_mix, 3.0f, corner_radius + margin);
}

bool Button::OnPadAction(InputAction action) {
    return onRightSideKey(action);
}

// ---------------------------------------------------------- 1 纯文字按钮 ----

TextButton::TextButton() {
    name = "text_button";
    text_align = TextAlign::Center;
}

TextButton::TextButton(std::string value) : Button(std::move(value)) {
    text_align = TextAlign::Center;
}

// --------------------------------------------------------- 2 图标+文字按钮 ---

IconTextButton::IconTextButton() {
    name = "icon_text_button";
    text_align = TextAlign::Left;
}

IconTextButton::IconTextButton(std::string glyph, std::string value) : IconTextButton() {
    icon = std::move(glyph);
    text = std::move(value);
}

// ------------------------------------------------------------- 3 图标按钮 ---

IconButton::IconButton() {
    name = "icon_button";
    text_align = TextAlign::Center;
}

IconButton::IconButton(std::string glyph) : IconButton() {
    icon = std::move(glyph);
}

// --------------------------------------------------------------- 4 开关 ----

ToggleButton::ToggleButton() {
    name = "toggle_button";
    text_align = TextAlign::Left;
}

ToggleButton::ToggleButton(std::string glyph, std::string value) : ToggleButton() {
    icon = std::move(glyph);
    text = std::move(value);
}

ToggleButton& ToggleButton::setChecked(bool value, bool notify) {
    if (checked == value) {
        return *this;
    }
    checked = value;
    selected = value;
    if (notify) {
        emit toggled(checked);
    }
    return *this;
}

float ToggleButton::rightSideWidth() const {
    const char* label = checked ? on_text.c_str() : off_text.c_str();
    return Draw::MeasureText(nullptr, mainFontSize(), label, 0.0f).x;
}

void ToggleButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const char* label = checked ? on_text.c_str() : off_text.c_str();
    const float size = mainFontSize();
    const ImVec2 extent = Draw::MeasureText(nullptr, size, label, 0.0f);
    Draw::Text(dl, nullptr, size,
               ImVec2(right_rect.max.x - extent.x, right_rect.Center().y - extent.y * 0.5f),
               Theme::U32(checked ? on_color : off_color), label);
}

bool ToggleButton::OnPadAction(InputAction action) {
    if (onRightSideKey(action)) {
        return true;
    }
    if (action == InputAction::Confirm) {
        // 切换后返回 false，让基类继续发 clicked（Qt 里 toggle 也会发 clicked）
        setChecked(!checked);
        return false;
    }
    return false;
}

// ---------------------------------------------------- 5 自定义右侧文字按钮 ---

CustomButton::CustomButton() {
    name = "custom_button";
    text_align = TextAlign::Left;
}

CustomButton::CustomButton(std::string glyph, std::string value) : CustomButton() {
    icon = std::move(glyph);
    text = std::move(value);
}

CustomButton& CustomButton::setRightText(std::string value, ImVec4 color) {
    right_text = std::move(value);
    right_color = color;
    return *this;
}

float CustomButton::rightSideWidth() const {
    return Draw::MeasureText(nullptr, mainFontSize(), right_text.c_str(), 0.0f).x;
}

void CustomButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const float size = mainFontSize();
    const ImVec2 extent = Draw::MeasureText(nullptr, size, right_text.c_str(), 0.0f);
    Draw::Text(dl, nullptr, size,
               ImVec2(right_rect.max.x - extent.x, right_rect.Center().y - extent.y * 0.5f), Theme::U32(right_color),
               right_text.c_str());
}

// ------------------------------------------------------- 6 LR 选项选择按钮 ---

OptionButton::OptionButton() {
    name = "option_button";
    text_align = TextAlign::Left;
}

OptionButton::OptionButton(std::string glyph, std::string value) : OptionButton() {
    icon = std::move(glyph);
    text = std::move(value);
}

OptionButton& OptionButton::setOptions(std::vector<std::string> values, int start_index) {
    options = std::move(values);
    index = options.empty() ? 0 : Clampf(static_cast<float>(start_index), 0.0f, static_cast<float>(options.size()) - 1.0f);
    return *this;
}

OptionButton& OptionButton::setIndex(int value, bool notify) {
    if (options.empty()) {
        return *this;
    }
    const int count = static_cast<int>(options.size());
    int next = value;
    if (wrap) {
        next = ((value % count) + count) % count;
    } else {
        next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    }
    if (next == index) {
        return *this;
    }
    index = next;
    if (notify) {
        emit selectionChanged(index);
    }
    return *this;
}

const char* OptionButton::currentOption() const {
    return options.empty() ? "" : options[static_cast<std::size_t>(index)].c_str();
}

float OptionButton::rightSideWidth() const {
    const float key = Draw::MeasureText(nullptr, mainFontSize(), kLeftKeyGlyph, 0.0f).x;
    const float option = Draw::MeasureText(nullptr, mainFontSize(), currentOption(), 0.0f).x;
    return key * 2.0f + option + 20.0f; // 两个按键图标 + 选项文字 + 间距
}

void OptionButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const float size = mainFontSize();
    const ImVec2 left_extent = Draw::MeasureText(nullptr, size, kLeftKeyGlyph, 0.0f);
    const ImVec2 right_extent = Draw::MeasureText(nullptr, size, kRightKeyGlyph, 0.0f);
    const ImVec2 option_extent = Draw::MeasureText(nullptr, size, currentOption(), 0.0f);
    const float total = left_extent.x + 10.0f + option_extent.x + 10.0f + right_extent.x;
    // 右侧整体靠右对齐
    float x = right_rect.max.x - total;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - left_extent.y * 0.5f), Theme::U32(Theme::kTextMuted),
               kLeftKeyGlyph);
    x += left_extent.x + 10.0f;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - option_extent.y * 0.5f),
               Theme::U32(option_color), currentOption());
    x += option_extent.x + 10.0f;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - right_extent.y * 0.5f),
               Theme::U32(Theme::kTextMuted), kRightKeyGlyph);
}

bool OptionButton::OnPadAction(InputAction action) {
    if (action == InputAction::PageLeft) {
        setIndex(index - 1);
        return true;
    }
    if (action == InputAction::PageRight) {
        setIndex(index + 1);
        return true;
    }
    return false;
}

// ------------------------------------------------------- 7 LR 数值选择按钮 ---

ValueButton::ValueButton() {
    name = "value_button";
    text_align = TextAlign::Left;
}

ValueButton::ValueButton(std::string glyph, std::string value) : ValueButton() {
    icon = std::move(glyph);
    text = std::move(value);
}

ValueButton& ValueButton::setup(float initial, float min_v, float max_v, float step_v, int precision_digits) {
    min_value = min_v;
    max_value = max_v;
    step = step_v;
    precision = precision_digits;
    value = Clampf(initial, Minf(min_value, max_value), Maxf(min_value, max_value));
    return *this;
}

ValueButton& ValueButton::setValue(float next, bool notify) {
    const float clamped = Clampf(next, Minf(min_value, max_value), Maxf(min_value, max_value));
    if (clamped == value) {
        return *this;
    }
    value = clamped;
    if (notify) {
        emit valueChanged(value);
    }
    return *this;
}

std::string ValueButton::valueText() const {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.*f", precision, static_cast<double>(value));
    return buffer;
}

float ValueButton::rightSideWidth() const {
    const float key = Draw::MeasureText(nullptr, mainFontSize(), kLeftKeyGlyph, 0.0f).x;
    const float value_w = Draw::MeasureText(nullptr, mainFontSize(), valueText().c_str(), 0.0f).x;
    return key * 2.0f + value_w + 20.0f;
}

void ValueButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const float size = mainFontSize();
    const std::string shown = valueText();
    const ImVec2 left_extent = Draw::MeasureText(nullptr, size, kLeftKeyGlyph, 0.0f);
    const ImVec2 right_extent = Draw::MeasureText(nullptr, size, kRightKeyGlyph, 0.0f);
    const ImVec2 value_extent = Draw::MeasureText(nullptr, size, shown.c_str(), 0.0f);
    const float total = left_extent.x + 10.0f + value_extent.x + 10.0f + right_extent.x;
    float x = right_rect.max.x - total;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - left_extent.y * 0.5f), Theme::U32(Theme::kTextMuted),
               kLeftKeyGlyph);
    x += left_extent.x + 10.0f;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - value_extent.y * 0.5f),
               Theme::U32(value_color), shown.c_str());
    x += value_extent.x + 10.0f;
    Draw::Text(dl, nullptr, size, ImVec2(x, right_rect.Center().y - right_extent.y * 0.5f),
               Theme::U32(Theme::kTextMuted), kRightKeyGlyph);
}

bool ValueButton::OnPadAction(InputAction action) {
    if (action == InputAction::PageLeft) {
        setValue(value - step);
        return true;
    }
    if (action == InputAction::PageRight) {
        setValue(value + step);
        return true;
    }
    return false;
}

} // namespace gui_dev::cv
