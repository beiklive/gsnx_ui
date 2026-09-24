// Button：按钮组件（手柄 UI 里最重要的控件之一）。
//
// 状态：Normal / Focused / Pressed / Selected / Disabled
// 视觉：背景、圆角、边框、图标、图标+文本、焦点框、焦点缩放、按压缩放、按压缩减动画
// 交互：A=确认，X/Y=辅助操作（on_aux / on_aux2），方向键交给页面导航
#pragma once

#include <functional>
#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Button : public Widget {
public:
    Button();
    explicit Button(std::string text);

    std::string text;
    std::string icon;      // 图标字形（Icons::Glyph / Material 图标字符），可为空
    std::string hint;      // 右侧快捷键提示（例如 "A" / "ZL"）
    float font_size = 0.0f;
    float icon_size = 0.0f; // 0 = font_size * 1.15
    float icon_gap = 8.0f;

    // 五态颜色
    ImU32 color_normal = Theme::kButton;
    ImU32 color_hover = Theme::kAccentHover;
    ImU32 color_pressed = Theme::kButtonActive;
    ImU32 color_selected = Theme::kSelection;
    ImU32 color_disabled = Theme::kBgWidget;
    ImU32 text_color = Theme::kTextBright;
    ImU32 text_color_disabled = Theme::kTextDisabled;

    // 动画
    float transition_speed = 16.0f;
    float press_scale = 0.97f; // 按下时的缩放
    float press_translate = 1.5f; // 按下时向下位移

    // 选中态（Qt 的 checkable / checked）
    bool checkable = false;
    bool checked = false;

signals:
    // Qt 命名：clicked/down/released 继承自 Widget，这里是按钮自己的信号
    Signal<bool> toggled;   // checkable 且状态变化时发射
    Signal<int> auxTriggered; // X = 0，Y = 1（手柄辅助键）
public:

    // ---- 预设 --------------------------------------------------------------
    Button& Primary();
    Button& Secondary();
    Button& Ghost();
    Button& Danger();

    Button& SetText(std::string value);
    Button& SetIcon(std::string glyph);
    Button& SetHint(std::string glyph);
    Button& SetFontSize(float value);
    Button& SetIconGap(float value);
    // Qt 风格：setCheckable / setChecked / isChecked
    Button& setCheckable(bool value);
    Button& setChecked(bool value);
    bool isChecked() const { return checked; }
    Button& FitContent(float horizontal_padding = 12.0f, float height = Theme::kControlHeight);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    float hover_mix_ = 0.0f; // 0 = normal，1 = hover
    float press_mix_ = 0.0f; // 0 = 未按下，1 = 按下
    float ResolvedIconSize() const { return icon_size > 0.0f ? icon_size : ResolvedFontSize() * 1.15f; }
    float ResolvedFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }
    ImVec2 ContentExtent() const;
};

} // namespace gui_dev::cv
