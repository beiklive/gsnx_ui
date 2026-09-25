#include "component_view/components/Button.h"

#include <cmath>
#include <cstdio>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

// 右侧内容滚动切换用的 L / R 图标（任天堂私用区字形）
const char* kLeftKeyGlyph = Icons::Glyph(Icons::Button::L);
const char* kRightKeyGlyph = Icons::Glyph(Icons::Button::R);

// [L] <间隔> [R] 里，图标和间隔之间的固定间距
constexpr float kLrGap = 10.0f;

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
    ApplyComponentBoxStyle();
    padding = EdgeInsets::All(style.content_padding);
    if (background == 0) {
        background = Theme::U32(Theme::kBgWidget);
        background_follows_theme = true;
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

Button& Button::setIconCellSize(float value) {
    icon_cell = value;
    return *this;
}

Button& Button::setSlotWidth(float value) {
    lr_slot_width = value;
    return *this;
}

Button& Button::setFontSize(float main_size, float sub_size) {
    font_size = main_size;
    this->subtitle_size = sub_size; // 注意别写成 subtitle_size = subtitle_size（参数遮蔽成员）
    return *this;
}

Button& Button::setTextColors(ImVec4 main_color, ImVec4 sub_color) {
    text_color = main_color;
    subtitle_color = sub_color;
    text_color_follows_theme = false; // 显式设过色：切主题时保持不动
    subtitle_color_follows_theme = false;
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

float Button::ResolvedSlotWidth() const {
    return lr_slot_width > 0.0f ? lr_slot_width : Global::component_style.lr_slot_width;
}

bool Button::SubtitleAllowed() const {
    return true;
}

bool Button::CaptionOutside() const {
    return false;
}

Button::LeftBlock Button::computeLeftBlock(const Rect& content) const {
    LeftBlock block;
    const bool show_sub = SubtitleVisible();
    const float main_size = mainFontSize();
    const float sub_size = subFontSize();
    const ImVec2 main_extent = Draw::MeasureText(nullptr, main_size, text.c_str(), 0.0f);
    const ImVec2 sub_extent =
        show_sub ? Draw::MeasureText(nullptr, sub_size, subtitle.c_str(), 0.0f) : ImVec2(0.0f, 0.0f);

    // 图标占左侧一个正方形格：边长默认 = 内容区高度 → 到边框的上下左右留白完全相同
    const float cell = icon.empty() ? 0.0f : (icon_cell > 0.0f ? icon_cell : content.Height());
    const float gap = (icon.empty() || text.empty()) ? 0.0f : icon_gap;

    if (text.empty() && !icon.empty()) {
        // 纯图标：图标在按钮里居中；说明行（如果有）是画在外面的，不占这里的位置
        const float text_w = show_sub ? sub_extent.x : 0.0f;
        const float text_h = show_sub ? sub_extent.y : 0.0f;
        block.vertical = !show_sub;
        block.width = Maxf(cell, text_w);
        const float total_h = cell + (show_sub ? text_h + 2.0f : 0.0f);
        const float block_x = (text_align == TextAlign::Center) ? content.Center().x - block.width * 0.5f
                                                               : content.min.x;
        const float block_y = content.Center().y - total_h * 0.5f;
        block.icon = Rect::FromPosSize(ImVec2(block_x + (block.width - cell) * 0.5f, block_y),
                                      ImVec2(cell, cell));
        block.text = Rect::FromPosSize(ImVec2(block_x + (block.width - text_w) * 0.5f, block_y + cell + 2.0f),
                                      ImVec2(text_w, text_h));
        return block;
    }

    const float text_w = Maxf(main_extent.x, sub_extent.x);
    const float text_h = main_extent.y + (show_sub ? sub_extent.y + 2.0f : 0.0f);
    block.width = cell + gap + text_w;

    // 文字块（主文字 [+ 说明行]）整体相对内容区垂直居中：
    // 位置按文字块自己的高度算，而不是按图标格的高度算 —— 否则关掉说明行后主文字会停在偏上的位置。
    const float block_x = (text_align == TextAlign::Center) ? content.Center().x - block.width * 0.5f
                                                            : content.min.x;
    block.icon = Rect::FromPosSize(ImVec2(block_x, content.Center().y - cell * 0.5f), ImVec2(cell, cell));
    block.text = Rect::FromPosSize(ImVec2(block_x + cell + gap, content.Center().y - text_h * 0.5f),
                                   ImVec2(text_w, text_h));
    return block;
}

void Button::drawLeftBlock(ImDrawList* dl, const LeftBlock& block) const {
    const float main_size = mainFontSize();
    const float sub_size = subFontSize();
    const bool show_sub = SubtitleVisible();

    if (!icon.empty() && block.icon.Width() > 0.0f) {
        // 图标大小跟着格子走（额外 0.86 让四周留白看起来均匀）
        const float glyph_size = block.icon.Height() * 0.86f;
        const ImVec2 extent = Draw::MeasureText(nullptr, glyph_size, icon.c_str(), 0.0f);
        // 格内水平居中；垂直按「墨迹」居中：行盒下方有 descender 空白，按行盒居中看起来会偏上
        float y = block.icon.Center().y - extent.y * 0.5f;
        float ink_top = 0.0f;
        float ink_bottom = 0.0f;
        if (Draw::GlyphInkExtent(nullptr, glyph_size, icon.c_str(), ink_top, ink_bottom)) {
            y = block.icon.Center().y - (ink_top + ink_bottom) * 0.5f;
        }
        Draw::Text(dl, nullptr, glyph_size, ImVec2(block.icon.Center().x - extent.x * 0.5f, y),
                   Ink(text_color), icon.c_str());
    }
    if (text.empty() && !show_sub) {
        return;
    }
    const ImVec2 main_extent = Draw::MeasureText(nullptr, main_size, text.c_str(), 0.0f);

    if (block.vertical) {
        // 纯图标形态：说明行在图标下方居中
        if (show_sub) {
            Draw::Text(dl, nullptr, sub_size, ImVec2(block.text.min.x, block.text.min.y),
                       Ink(subtitle_color), subtitle.c_str());
        }
        return;
    }

    // 主文字在上、说明行在下，两块作为整体已经垂直居中
    const float main_y = show_sub ? block.text.min.y : block.text.Center().y - main_extent.y * 0.5f;
    Draw::Text(dl, nullptr, main_size, ImVec2(block.text.min.x, main_y), Ink(text_color), text.c_str());
    if (show_sub) {
        Draw::Text(dl, nullptr, sub_size, ImVec2(block.text.min.x, main_y + main_extent.y + 2.0f),
                   Ink(subtitle_color), subtitle.c_str());
    }
}

float Button::rightSideWidth() const {
    return 0.0f;
}

void Button::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    (void)dl;
    (void)right_rect;
}

// [L] 间隔 [R]：L / R 图标贴在右侧两端，中间是**固定宽度**的间隔（默认
// Global::component_style.lr_slot_width），文字/数字在间隔里居中；放不下就在间隔里滚动。
float Button::lrKeysWidth() const {
    const float size = mainFontSize();
    const float left = Draw::MeasureText(nullptr, size, kLeftKeyGlyph, 0.0f).x;
    const float right = Draw::MeasureText(nullptr, size, kRightKeyGlyph, 0.0f).x;
    return left + kLrGap + ResolvedSlotWidth() + kLrGap + right;
}

void Button::drawLrRow(ImDrawList* dl, const Rect& right_rect, const char* content, ImU32 content_color) const {
    const float size = mainFontSize();
    const ImVec2 left_extent = Draw::MeasureText(nullptr, size, kLeftKeyGlyph, 0.0f);
    const ImVec2 right_extent = Draw::MeasureText(nullptr, size, kRightKeyGlyph, 0.0f);
    const float total = left_extent.x + kLrGap + ResolvedSlotWidth() + kLrGap + right_extent.x;
    const float x = right_rect.max.x - total; // 整行靠右对齐
    const float center_y = right_rect.Center().y;
    Draw::Text(dl, nullptr, size, ImVec2(x, center_y - left_extent.y * 0.5f), Ink(Theme::kTextMuted),
               kLeftKeyGlyph);
    const Rect slot = Rect::FromPosSize(ImVec2(x + left_extent.x + kLrGap, right_rect.min.y),
                                       ImVec2(ResolvedSlotWidth(), right_rect.Height()));
    Draw::MarqueeText(dl, nullptr, size, slot, content_color, content, Global::time,
                      Global::component_style.marquee_speed);
    const float right_x = x + left_extent.x + kLrGap + ResolvedSlotWidth() + kLrGap;
    Draw::Text(dl, nullptr, size, ImVec2(right_x, center_y - right_extent.y * 0.5f), Ink(Theme::kTextMuted),
               kRightKeyGlyph);
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

FocusVisual Button::BuildFocusVisual() const {
    FocusVisual visual;
    if (!flowing_focus || focus_mix <= 0.01f) {
        return visual;
    }
    // 完整闭合的流光框：四边都在，颜色沿边框流动，与按钮留 margin（默认 2px）
    const Global::ComponentStyle& style = Global::component_style;
    const float margin = ResolvedFocusMargin();
    visual.enabled = true;
    visual.flowing = true;
    visual.rect = DrawRect().Expanded(margin);
    // 外扩后的圆角 = 按钮圆角 + 外扩量，这样流光框的圆角跟按钮轮廓平行
    visual.radius = corner_radius + margin;
    visual.width = ResolvedFocusWidth();
    visual.alpha = focus_mix * EffectiveOpacity(); // 入场/禁用时和按钮本体一起淡
    visual.phase = Global::time * style.focus_flow_speed + focus_phase_offset;
    visual.saturation = focus_saturation >= 0.0f ? focus_saturation : style.focus_saturation;
    visual.brightness = focus_brightness >= 0.0f ? focus_brightness : style.focus_brightness;
    return visual;
}

bool Button::OnPadAction(InputAction action) {
    return onRightSideKey(action);
}

void Button::OnThemeChanged() {
    applyComponentStyle(); // 边框 / 阴影 / 圆角 / 内容留白
    if (background_follows_theme) {
        background = Theme::U32(Theme::kBgWidget);
    }
    if (text_color_follows_theme) {
        text_color = Theme::kTextPrimary;
    }
    if (subtitle_color_follows_theme) {
        subtitle_color = Theme::kTextMuted;
    }
}

// ---------------------------------------------------------- 1 纯文字按钮 ----
// 弹窗的「确认 / 取消」这类提示文字：文字居中，不带说明行。
// 说明行接口在这里无效（SubtitleAllowed() = false），避免有人在提示按钮上误加小字。

TextButton::TextButton() {
    name = "text_button";
    text_align = TextAlign::Center;
    show_subtitle = false;
}

TextButton::TextButton(std::string value) : Button(std::move(value)) {
    name = "text_button";
    text_align = TextAlign::Center;
    show_subtitle = false;
}

bool TextButton::SubtitleAllowed() const {
    return false;
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
// 只有圆角正方形 / 圆形两种形态；边长 setSide()，圆形时圆角 = 边长的一半。

IconButton::IconButton() {
    name = "icon_button";
    text_align = TextAlign::Center;
}

IconButton::IconButton(std::string glyph) : IconButton() {
    icon = std::move(glyph);
}

IconButton& IconButton::setShape(IconButtonShape value) {
    shape = value;
    setSide(side); // 重新算圆角
    return *this;
}

IconButton& IconButton::setSide(float value) {
    side = Maxf(value, 8.0f);
    size = ImVec2(side, side);
    // 圆形：圆角 = 边长的一半；圆角正方形：用全局约定圆角
    corner_radius = (shape == IconButtonShape::Circle) ? side * 0.5f : Global::component_style.corner_radius;
    corner_tl = corner_tr = corner_bl = corner_br = -1.0f;
    return *this;
}

ImVec2 IconButton::MeasureContent(const ImVec2& available) {
    (void)available;
    const float inner = Maxf(side - padding.left - padding.right, 8.0f);
    return ImVec2(inner, inner);
}

// 说明行能落在哪块区域里：画布 ∩ 父节点（没有父节点就用画布）
Rect IconButton::CaptionLimit() const {
    Rect limit = Global::CanvasRect();
    if (parent != nullptr) {
        const Rect& outer = parent->rect;
        limit.min.x = Maxf(limit.min.x, outer.min.x);
        limit.min.y = Maxf(limit.min.y, outer.min.y);
        limit.max.x = Minf(limit.max.x, outer.max.x);
        limit.max.y = Minf(limit.max.y, outer.max.y);
    }
    return limit;
}

// 说明行画在按钮外面：默认在下面；下面空间不够就等距放到上面；上下都不够就不画
void IconButton::drawCaption(ImDrawList* dl) const {
    if (!show_subtitle || subtitle.empty()) {
        return;
    }
    const float size = subFontSize();
    const ImVec2 extent = Draw::MeasureText(nullptr, size, subtitle.c_str(), 0.0f);
    const Rect limit = CaptionLimit();
    const Rect self = DrawRect();
    const float below = limit.max.y - self.max.y;
    const float above = self.min.y - limit.min.y;
    float y = 0.0f;
    if (below >= extent.y + caption_gap) {
        y = self.max.y + caption_gap;
    } else if (above >= extent.y + caption_gap) {
        y = self.min.y - caption_gap - extent.y;
    } else {
        return; // 上下都放不下：不显示提示文字
    }
    Draw::Text(dl, nullptr, size, ImVec2(self.Center().x - extent.x * 0.5f, y), Ink(subtitle_color),
               subtitle.c_str());
}

void IconButton::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    (void)content; // 焦点框由 FocusRing 图层画
    drawCaption(dl);
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

ToggleButton& ToggleButton::setSwitchSize(float width, float height) {
    switch_width = Maxf(width, 16.0f);
    switch_height = Clampf(height, 10.0f, Maxf(switch_width, 10.0f));
    return *this;
}

ToggleButton& ToggleButton::setSwitchColors(ImVec4 on, ImVec4 off, ImVec4 knob) {
    on_color = on;
    off_color = off;
    knob_color = knob;
    switch_colors_follow_theme = false; // 显式设过色：切主题时保持不动
    return *this;
}

void ToggleButton::OnThemeChanged() {
    Button::OnThemeChanged();
    if (switch_colors_follow_theme) {
        on_color = Theme::kAccent;
        off_color = Theme::kSwitchOff;
        knob_color = Theme::kSwitchKnob;
    }
}

float ToggleButton::rightSideWidth() const {
    return switch_width;
}

void ToggleButton::OnUpdate(float dt) {
    const float target = checked ? 1.0f : 0.0f;
    if (knob_mix_ < 0.0f) {
        knob_mix_ = target; // 第一帧直接对齐，避免刚打开就滑一下
        return;
    }
    // 指数平滑，和 Widget 的焦点动画同一种手感
    const float k = 1.0f - std::exp(-Maxf(knob_speed, 0.01f) * Maxf(dt, 0.0f));
    knob_mix_ += (target - knob_mix_) * k;
    if (std::fabs(knob_mix_ - target) < 0.001f) {
        knob_mix_ = target;
    }
}

void ToggleButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const float t = Clampf(knob_mix_ < 0.0f ? (checked ? 1.0f : 0.0f) : knob_mix_, 0.0f, 1.0f);
    const float height = switch_height;
    const Rect track = Rect::FromPosSize(ImVec2(right_rect.max.x - switch_width, right_rect.Center().y - height * 0.5f),
                                         ImVec2(switch_width, height));
    const float radius = height * 0.5f;
    // 轨道：关闭灰 -> 打开蓝，颜色跟着动画一起过渡
    Draw::RoundedRectFilled(dl, track, Ink(Theme::Mix(Theme::U32(off_color), Theme::U32(on_color), t)), radius, radius,
                            radius, radius);
    // 旋钮：从左滑到右
    const float inset = Maxf(2.0f, height * 0.12f);
    const float knob_r = Maxf(radius - inset, 2.0f);
    const float travel = Maxf(track.Width() - 2.0f * inset - 2.0f * knob_r, 0.0f);
    const float knob_x = track.min.x + inset + knob_r + travel * t;
    const Rect knob = Rect::FromPosSize(ImVec2(knob_x - knob_r, track.Center().y - knob_r),
                                        ImVec2(knob_r * 2.0f, knob_r * 2.0f));
    ShadowStyle knob_shadow;
    knob_shadow.enabled = true;
    knob_shadow.color = Ink(Theme::U32(Theme::rgba(0, 0, 0, 110)));
    knob_shadow.offset = ImVec2(0.0f, 1.0f);
    knob_shadow.blur = 3.0f;
    Draw::SoftShadow(dl, knob, knob_shadow, knob_r, knob_r, knob_r, knob_r);
    Draw::RoundedRectFilled(dl, knob, Ink(knob_color), knob_r, knob_r, knob_r, knob_r);
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
    return setRightColor(color);
}

CustomButton& CustomButton::setRightColor(ImVec4 color) {
    right_color = color;
    right_color_follows_theme = false; // 显式设过色就不再跟主题
    return *this;
}

void CustomButton::OnThemeChanged() {
    Button::OnThemeChanged();
    if (right_color_follows_theme) {
        right_color = Theme::kTextPrimary;
    }
}

float CustomButton::rightSideWidth() const {
    return Draw::MeasureText(nullptr, mainFontSize(), right_text.c_str(), 0.0f).x;
}

void CustomButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const float size = mainFontSize();
    const ImVec2 extent = Draw::MeasureText(nullptr, size, right_text.c_str(), 0.0f);
    Draw::Text(dl, nullptr, size,
               ImVec2(right_rect.max.x - extent.x, right_rect.Center().y - extent.y * 0.5f), Ink(right_color),
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

OptionButton& OptionButton::setOptionColor(ImVec4 color) {
    option_color = color;
    option_color_follows_theme = false;
    return *this;
}

void OptionButton::OnThemeChanged() {
    Button::OnThemeChanged();
    if (option_color_follows_theme) {
        option_color = Theme::kTextBright; // 浅色主题 = 黑字，深色 = 白字
    }
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
    return lrKeysWidth();
}

void OptionButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    drawLrRow(dl, right_rect, currentOption(), Ink(option_color));
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

ValueButton& ValueButton::setValueColor(ImVec4 color) {
    value_color = color;
    value_color_follows_theme = false;
    return *this;
}

void ValueButton::OnThemeChanged() {
    Button::OnThemeChanged();
    if (value_color_follows_theme) {
        value_color = Theme::kTextBright;
    }
}

std::string ValueButton::valueText() const {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.*f", precision, static_cast<double>(value));
    return buffer;
}

float ValueButton::rightSideWidth() const {
    return lrKeysWidth();
}

void ValueButton::drawRightSide(ImDrawList* dl, const Rect& right_rect) {
    const std::string shown = valueText();
    drawLrRow(dl, right_rect, shown.c_str(), Ink(value_color));
}

void ValueButton::stepBy(int direction, float multiplier) {
    setValue(value + step * multiplier * static_cast<float>(direction), false);
}

bool ValueButton::OnPadAction(InputAction action) {
    const int direction = (action == InputAction::PageLeft) ? -1 : (action == InputAction::PageRight) ? 1 : 0;
    if (direction == 0) {
        return false;
    }
    // 短按：先改一步（只改显示值，不发信号）；按住时间交给 OnUpdate 做加速重复
    const float before = value;
    stepBy(direction, 1.0f);
    hold_dir_ = direction;
    hold_time_ = 0.0f;
    repeat_timer_ = 0.0f;
    pending_change_ = (value != before); // 已经顶到边界就不算变化，松开时也不发信号
    return true;
}

void ValueButton::flushPending() {
    if (pending_change_) {
        pending_change_ = false;
        emit valueChanged(value);
    }
}

void ValueButton::OnUpdate(float dt) {
    if (hold_dir_ == 0) {
        return;
    }
    const InputAction held_action = hold_dir_ < 0 ? InputAction::PageLeft : InputAction::PageRight;
    if (!Global::pad.Held(held_action)) {
        // 松开：短按 / 长按都在这里发一次 valueChanged
        hold_dir_ = 0;
        hold_time_ = 0.0f;
        repeat_timer_ = 0.0f;
        flushPending();
        return;
    }
    hold_time_ += dt;
    if (hold_time_ < repeat_delay) {
        return;
    }
    // 长按加速：间隔从 repeat_interval 线性收紧到 repeat_min_interval（有下限），
    // 步长倍率从 1 涨到 repeat_max_multiplier（有上限）。
    const float progress = Clampf((hold_time_ - repeat_delay) / Maxf(repeat_accel_time, 0.01f), 0.0f, 1.0f);
    const float interval = Maxf(repeat_interval + (repeat_min_interval - repeat_interval) * progress,
                                repeat_min_interval);
    const float multiplier = Clampf(1.0f + (repeat_max_multiplier - 1.0f) * progress, 1.0f, repeat_max_multiplier);
    // 倍率取整：跳跃始终是 step 的整数倍，长按调出来的值仍在网格上（不会出现 77.54）
    const float jump = static_cast<float>(static_cast<int>(multiplier));
    repeat_timer_ += dt;
    while (repeat_timer_ >= interval) {
        repeat_timer_ -= interval;
        const float before = value;
        stepBy(hold_dir_, jump);
        if (value == before) {
            hold_dir_ = 0; // 已经顶到边界：结束这次长按，把已有的变化发出去
            flushPending();
            break;
        }
        pending_change_ = true;
    }
}

} // namespace gui_dev::cv
