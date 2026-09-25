// Button：按钮组件（按约定的 7 种形态）。
//
// 全局约定样式（Global::component_style）：1px 灰白边框 / 5px 圆角 / 右下角阴影，
// 聚焦时画「完整闭合、颜色沿边框流动」的流光框，四边与控件留 2px 边距。
// 每个实例都能用链式接口覆盖这些默认值。
//
// 七种形态（都在 Button 的 box 内部）：
//   1 TextButton        文字居中（弹窗的确认/取消这类提示文字，不带说明行）
//   2 IconTextButton    图标 + 文字，都靠左；图标占左侧一个正方形格，格子内水平+垂直居中
//   3 IconButton        只有图标，只有「圆角正方形 / 圆形」两种形态
//   4 ToggleButton      左：图标 + 文字；右：开/关（开=蓝色，关=灰色），点击切换
//   5 CustomButton      左：图标 + 文字；右：自定义文字（内容与颜色都可改）
//   6 OptionButton      左：图标 + 文字；右：[L] 选项 [R]，固定间隔、超长滚动，L/R 切换
//   7 ValueButton       左：图标 + 文字；右：[L] 数值 [R]，固定间隔、超长滚动，L/R 调值，
//                       长按加速（有上限），valueChanged 在短按或长按松开后触发
//
// 说明行：除 TextButton 外都支持 showSubtitle(true)：主文字下加一行小号浅色文字。
// 有主文字时图标在左侧方形格里居中、文字紧跟其右；没有主文字（纯图标）时说明行落到图标下方居中。
// 开关与否文字块都整体垂直居中。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

// 纯图标按钮的两种形态
enum class IconButtonShape {
    RoundedSquare, // 圆角正方形
    Circle,        // 圆形
};

class Button : public Widget {
public:
    Button();
    explicit Button(std::string text);

    // ---- 内容 --------------------------------------------------------------
    std::string icon;             // 字体图标（Material 字形），可为空
    std::string text;             // 主文字
    std::string subtitle;         // 说明行（小号浅色）
    bool show_subtitle = false;   // 说明行开关
    TextAlign text_align = TextAlign::Left;
    float icon_gap = 8.0f;
    float icon_cell = -1.0f;      // 左侧图标正方形格的边长，<0 = 内容区高度（四周留白相同）
    float font_size = 0.0f;       // 0 = Theme::kFontBody
    float subtitle_size = 0.0f;   // 0 = Theme::kFontSmall
    ImVec4 text_color = Theme::kTextPrimary;
    ImVec4 subtitle_color = Theme::kTextMuted;
    // 文字颜色是否跟随主题：用 setTextColors() 设过颜色就置 false（固定色）
    bool text_color_follows_theme = true;
    bool subtitle_color_follows_theme = true;
    float lr_slot_width = -1.0f;  // LR 选择器中间那一格的宽度，<0 = Global::component_style

    // ---- 流光聚焦框（默认取 Global::component_style） -----------------------
    bool flowing_focus = true;
    float focus_margin = -1.0f;   // <0 = 用全局值
    float focus_width = -1.0f;
    float focus_phase_offset = 0.0f; // 让每个按钮的流光错开
    float focus_saturation = -1.0f;
    float focus_brightness = -1.0f;

    // ---- 链式接口 ----------------------------------------------------------
    Button& setIcon(std::string glyph);
    Button& setText(std::string value);
    Button& setSubtitle(std::string value, bool visible = true);
    Button& showSubtitle(bool value);
    Button& setTextAlign(TextAlign value);
    Button& setIconGap(float value);
    Button& setIconCellSize(float value);
    Button& setFontSize(float main_size, float subtitle_size = 0.0f);
    Button& setTextColors(ImVec4 main_color, ImVec4 sub_color);
    Button& resize(float width, float height);
    Button& moveTo(float x, float y);
    Button& setContentPadding(float value);
    Button& setSlotWidth(float value); // LR 选择器中间那一格的宽度
    // 覆盖全局约定样式（单实例）
    Button& setBorder(float width, ImVec4 color);
    Button& setCornerRadius(float value);
    Button& setShadow(ImVec2 offset, float blur, ImVec4 color);
    Button& setFlowingFocus(bool enabled);
    // 重新套用 Global::component_style（改了全局样式后调）
    Button& applyComponentStyle();

signals:
    // clicked / pressed / released / focusIn / focusOut 继承自 Widget
    Signal<bool> toggled;           // 开关类：状态变化
    Signal<int> selectionChanged;   // LR 选项类：当前索引
    Signal<float> valueChanged;     // LR 数值类：当前值

protected:
    void OnThemeChanged() override; // 切主题：重新套用约定样式 + 刷新跟随主题的颜色

    // 子类扩展点
    virtual float rightSideWidth() const;
    virtual void drawRightSide(ImDrawList* dl, const Rect& right_rect);
    virtual bool onRightSideKey(InputAction action) { (void)action; return false; }

    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    // 焦点框改成页面级图层统一画：这里只描述「流光框长什么样」
    cv::FocusVisual BuildFocusVisual() const override;
    bool OnPadAction(InputAction action) override;

    float mainFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }
    float subFontSize() const { return subtitle_size > 0.0f ? subtitle_size : Theme::kFontSmall; }
    float ResolvedFocusMargin() const;
    float ResolvedFocusWidth() const;
    float ResolvedSlotWidth() const;                  // LR 选择器中间那一格的宽度
    virtual bool SubtitleAllowed() const;             // TextButton 返回 false（不带说明行）
    virtual bool CaptionOutside() const;              // IconButton：说明行画在控件外面
    bool SubtitleVisible() const { return show_subtitle && SubtitleAllowed() && !CaptionOutside(); }

    // 文字 / 图标 / 开关的颜色统一过一遍 Widget::opacity：入出场淡入淡出、禁用态都靠它
    ImU32 Ink(const ImVec4& color) const { return Theme::Alpha(Theme::U32(color), EffectiveOpacity()); }
    ImU32 Ink(ImU32 color) const { return Theme::Alpha(color, EffectiveOpacity()); }

    // 左侧「图标 + 文字」块：
    //   有主文字 = 图标（正方形格，格内水平+垂直居中）+ 文字紧跟其右
    //   没主文字 = 图标在上、说明行在下方居中（竖排）
    struct LeftBlock {
        Rect icon;
        Rect text;
        float width = 0.0f;
        bool vertical = false; // true = 图标在上、说明行在下
    };
    LeftBlock computeLeftBlock(const Rect& content) const;
    void drawLeftBlock(ImDrawList* dl, const LeftBlock& block) const;

    // LR 选择器右侧那一行：[L] <固定间隔> [R]，内容在间隔里居中、放不下就滚动
    float lrKeysWidth() const;
    void drawLrRow(ImDrawList* dl, const Rect& right_rect, const char* content, ImU32 content_color) const;
};

// ---------------------------------------------------------------- 1 纯文字 ----
class TextButton : public Button {
public:
    TextButton();
    explicit TextButton(std::string value);

protected:
    bool SubtitleAllowed() const override; // 提示文字按钮不带说明行
};

// --------------------------------------------------------------- 2 图标+文字 ---
class IconTextButton : public Button {
public:
    IconTextButton();
    IconTextButton(std::string glyph, std::string value);
};

// ------------------------------------------------------------------ 3 纯图标 --
// 只有两种形态：圆角正方形 / 圆形。边长用 setSide()（默认 52，按 720p 手持尺寸）。
// 说明行不在按钮里画，而是画在按钮外面：默认在下面，下面空间不够就等距放到上面，都不够就不画。
class IconButton : public Button {
public:
    IconButton();
    explicit IconButton(std::string glyph);

    IconButtonShape shape = IconButtonShape::RoundedSquare;
    float side = 52.0f;
    float caption_gap = 4.0f; // 说明行与按钮之间的间距

    IconButton& setShape(IconButtonShape value);
    IconButton& setSide(float value); // 正方形/圆形的边长

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override;
    bool CaptionOutside() const override { return true; }
    Rect CaptionLimit() const;      // 可用空间：画布 ∩ 父节点
    void drawCaption(ImDrawList* dl) const;
};

// ------------------------------------------------------------------- 4 开关 ---
// 右侧是一个滑块式开关（轨道 + 旋钮），切换时旋钮位置与轨道颜色都做平滑动画。
class ToggleButton : public Button {
public:
    ToggleButton();
    ToggleButton(std::string glyph, std::string value);

    bool checked = false;
    // 开关外观
    ImVec4 on_color = Theme::kAccent;      // 打开时的轨道色
    ImVec4 off_color = Theme::kSwitchOff;      // 关闭时的轨道色
    ImVec4 knob_color = Theme::kSwitchKnob;    // 旋钮色（两套主题都是白色）
    bool switch_colors_follow_theme = true;    // setSwitchColors() 之后置 false
    float switch_width = 46.0f;
    float switch_height = 26.0f;
    float knob_speed = 14.0f; // 动画收敛速度（越大越快）

    ToggleButton& setChecked(bool value, bool notify = true);
    ToggleButton& setSwitchSize(float width, float height);
    ToggleButton& setSwitchColors(ImVec4 on, ImVec4 off, ImVec4 knob);
    float knobMix() const { return knob_mix_; }

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
    void OnUpdate(float dt) override;
    void OnThemeChanged() override;

private:
    float knob_mix_ = -1.0f; // <0 = 还没初始化，第一帧直接对齐到 checked（不播动画）
};

// ----------------------------------------------------------- 5 自定义右侧文字 --
class CustomButton : public Button {
public:
    CustomButton();
    CustomButton(std::string glyph, std::string value);

    std::string right_text;
    ImVec4 right_color = Theme::kTextPrimary;
    bool right_color_follows_theme = true; // 显式 setRightText(…, color) 之后置 false

    CustomButton& setRightText(std::string value, ImVec4 color);
    CustomButton& setRightColor(ImVec4 color); // 只改颜色、显式指定

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    void OnThemeChanged() override;
};

// -------------------------------------------------------------- 6 LR 选项选择 --
class OptionButton : public Button {
public:
    OptionButton();
    OptionButton(std::string glyph, std::string value);

    std::vector<std::string> options;
    int index = 0;
    bool wrap = true;
    ImVec4 option_color = Theme::kTextBright;
    bool option_color_follows_theme = true; // 默认跟主题（浅色=黑字 / 深色=白字）

    OptionButton& setOptions(std::vector<std::string> values, int start_index = 0);
    OptionButton& setIndex(int value, bool notify = true);
    OptionButton& setOptionColor(ImVec4 color);
    const char* currentOption() const;

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override;
};

// -------------------------------------------------------------- 7 LR 数值选择 --
// L/R 单击 = 改一步；按住 = 长按加速（延迟后开始重复，间隔递减、步长倍率递增，都有上限）。
// valueChanged 只在「短按松开」或「长按松开」时发一次，按住过程中只改显示值。
class ValueButton : public Button {
public:
    ValueButton();
    ValueButton(std::string glyph, std::string value);

    float value = 0.0f;
    float min_value = 0.0f;
    float max_value = 100.0f;
    float step = 1.0f;
    int precision = 0;          // 小数位
    bool wrap = false;
    ImVec4 value_color = Theme::kTextBright;
    bool value_color_follows_theme = true; // 默认跟主题

    // 长按加速参数（都有上限）
    float repeat_delay = 0.35f;          // 按住多久后开始重复
    float repeat_interval = 0.12f;       // 起始重复间隔
    float repeat_min_interval = 0.03f;   // 最快重复间隔（上限）
    float repeat_accel_time = 1.6f;      // 用多久从起始间隔滑到最快间隔
    float repeat_max_multiplier = 8.0f;  // 每次重复的步长倍率上限

    ValueButton& setup(float initial, float min_v, float max_v, float step_v, int precision_digits);
    ValueButton& setValue(float next, bool notify = true);
    ValueButton& setValueColor(ImVec4 color);
    std::string valueText() const;
    bool isRepeating() const { return hold_dir_ != 0 && hold_time_ >= repeat_delay; }

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
    void OnUpdate(float dt) override;
    void OnThemeChanged() override;

private:
    void stepBy(int direction, float multiplier);
    void flushPending();

    int hold_dir_ = 0;          // 当前按住的方向：-1 = L，+1 = R，0 = 没按住
    float hold_time_ = 0.0f;
    float repeat_timer_ = 0.0f;
    bool pending_change_ = false; // 松开时要发一次 valueChanged
};

} // namespace gui_dev::cv
