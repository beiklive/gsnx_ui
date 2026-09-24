// Button：按钮组件（按约定的 7 种形态）。
//
// 全局约定样式（Global::component_style）：1px 灰白边框 / 5px 圆角 / 右下角阴影，
// 聚焦时画「完整闭合、颜色沿边框流动」的流光框，四边与控件留 2px 边距。
// 每个实例都能用链式接口覆盖这些默认值。
//
// 七种形态（都在 Button 的 box 内部）：
//   1 TextButton        文字居中
//   2 IconTextButton    图标 + 文字，都靠左；图标到边框上下左右距离相同
//   3 IconButton        只显示图标
//   4 ToggleButton      左：图标 + 文字；右：开/关（开=蓝色，关=灰色），点击切换
//   5 CustomButton      左：图标 + 文字；右：自定义文字（内容与颜色都可改）
//   6 OptionButton      左：图标 + 文字；右：[L] 选项 [R]，L/R 切换选项
//   7 ValueButton       左：图标 + 文字；右：[L] 数值 [R]，初始化精度/步长/范围，L/R 调值
//
// 所有形态的左文字部分都支持「说明行」开关：show_subtitle(true) 会在主文字下加一行
// 小号浅色文字；开关与否，文字块都作为一个整体垂直居中（align 控制水平位置：
// 普通按钮用 Center 居中，其余默认靠左）。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

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
    float font_size = 0.0f;       // 0 = Theme::kFontBody
    float subtitle_size = 0.0f;   // 0 = Theme::kFontSmall
    ImVec4 text_color = Theme::kTextPrimary;
    ImVec4 subtitle_color = Theme::kTextMuted;

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
    Button& setFontSize(float main_size, float subtitle_size = 0.0f);
    Button& setTextColors(ImVec4 main_color, ImVec4 sub_color);
    Button& resize(float width, float height);
    Button& moveTo(float x, float y);
    Button& setContentPadding(float value);
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
    // 子类扩展点
    virtual float rightSideWidth() const;
    virtual void drawRightSide(ImDrawList* dl, const Rect& right_rect);
    virtual bool onRightSideKey(InputAction action) { (void)action; return false; }

    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override;
    bool OnPadAction(InputAction action) override;

    float mainFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }
    float subFontSize() const { return subtitle_size > 0.0f ? subtitle_size : Theme::kFontSmall; }
    float ResolvedFocusMargin() const;
    float ResolvedFocusWidth() const;

    // 左侧「图标 + 文字块」的矩形（图标是正方形，边长等于内容区高度 → 上下左右留白相同）
    struct LeftBlock {
        Rect icon;
        Rect text;
        float width = 0.0f;
    };
    LeftBlock computeLeftBlock(const Rect& content) const;
    void drawLeftBlock(ImDrawList* dl, const LeftBlock& block) const;
};

// ---------------------------------------------------------------- 1 纯文字 ----
class TextButton : public Button {
public:
    TextButton();
    explicit TextButton(std::string value);
};

// --------------------------------------------------------------- 2 图标+文字 ---
class IconTextButton : public Button {
public:
    IconTextButton();
    IconTextButton(std::string glyph, std::string value);
};

// ------------------------------------------------------------------ 3 纯图标 --
class IconButton : public Button {
public:
    IconButton();
    explicit IconButton(std::string glyph);
};

// ------------------------------------------------------------------- 4 开关 ---
class ToggleButton : public Button {
public:
    ToggleButton();
    ToggleButton(std::string glyph, std::string value);

    bool checked = false;
    std::string on_text = "开";
    std::string off_text = "关";
    ImVec4 on_color = Theme::kAccent;
    ImVec4 off_color = Theme::kTextMuted;

    ToggleButton& setChecked(bool value, bool notify = true);

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
};

// ----------------------------------------------------------- 5 自定义右侧文字 --
class CustomButton : public Button {
public:
    CustomButton();
    CustomButton(std::string glyph, std::string value);

    std::string right_text;
    ImVec4 right_color = Theme::kTextPrimary;

    CustomButton& setRightText(std::string value, ImVec4 color);

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
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

    OptionButton& setOptions(std::vector<std::string> values, int start_index = 0);
    OptionButton& setIndex(int value, bool notify = true);
    const char* currentOption() const;

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
};

// -------------------------------------------------------------- 7 LR 数值选择 --
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

    ValueButton& setup(float initial, float min_v, float max_v, float step_v, int precision_digits);
    ValueButton& setValue(float next, bool notify = true);
    std::string valueText() const;

protected:
    float rightSideWidth() const override;
    void drawRightSide(ImDrawList* dl, const Rect& right_rect) override;
    bool OnPadAction(InputAction action) override;
};

} // namespace gui_dev::cv
