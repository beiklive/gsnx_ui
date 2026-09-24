// Label：文本组件。
//
// 单行 / 多行 / 自动换行 / 省略号 / 跑马灯 / 对齐 / 字号字重 / 颜色 / 阴影 / 描边 /
// 字距行距，以及「焦点时换色、换文案」这套手柄 UI 常用的状态文本。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Label : public Widget {
public:
    Label();
    explicit Label(std::string text, float font_size = 0.0f, ImU32 color = Theme::kTextPrimary);

    std::string text;
    ImFont* font = nullptr; // nullptr = 当前主字体（含图标 fallback）
    float font_size = 0.0f; // 0 = Theme::kFontBody
    ImU32 color = Theme::kTextPrimary;
    ImU32 shadow_color = 0;  // alpha > 0 时给文字加 1px 投影
    ImU32 outline_color = 0; // alpha > 0 时给文字加 1px 描边
    int font_weight = 400;   // >= 600 时用 0.6px 偏移重绘模拟加粗

    float letter_spacing = 0.0f;
    float line_spacing = 0.0f;
    float line_height_scale = 1.18f;

    TextAlign text_align = TextAlign::Left;
    VerticalAlign vertical_align = VerticalAlign::Middle;
    float wrap_width = 0.0f; // 0 = 用 size.x；都未设置则按可用宽度换行
    bool wrap_to_available = true;
    bool single_line = true; // true = 不换行（超宽交给 ellipsis / marquee）
    bool ellipsis = false;   // 单行超宽时以 … 截断
    bool marquee = false;    // 单行超宽时横向滚动
    float marquee_speed = 42.0f;
    float marquee_hold = 0.6f; // 每轮开头停顿
    bool uppercase = false;

    // 焦点状态文本（手柄 UI：同一条目聚焦时换文案/换色）
    std::string focus_text;
    ImU32 focus_color = 0;
    bool disabled_strikethrough = false;

    Label& SetText(std::string value);
    Label& SetColor(ImU32 value);
    Label& SetFontSize(float value);
    Label& SetWeight(int value);
    Label& SetAlign(TextAlign horizontal, VerticalAlign vertical);
    Label& SetWrap(float width);
    Label& SetShadow(ImU32 value);
    Label& SetOutline(ImU32 value);
    Label& SetSpacing(float letter, float line);
    Label& SetEllipsis(bool value);
    Label& SetMarquee(bool value, float speed = 42.0f);
    Label& SetFocusOverride(std::string text_when_focused, ImU32 color_when_focused);

    float ResolvedFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }
    // 当前实际使用的文案（考虑 focus_text / uppercase）
    const char* ResolvedText() const;
    ImU32 ResolvedColor() const;
    // 文本布局结果（多行）
    std::vector<std::string> LayoutLines(float available_width) const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;

private:
    float EffectiveWrap(float available) const;
    float LineHeight() const { return ResolvedFontSize() * line_height_scale + line_spacing; }
    float marquee_time_ = 0.0f;
    bool marquee_active_ = false;
};

} // namespace gui_dev::cv
