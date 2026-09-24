// Label：文本组件。字号 / 颜色 / 对齐 / 自动换行 / 阴影。
#pragma once

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Label : public Widget {
public:
    Label();
    explicit Label(std::string text, float font_size = 0.0f, ImU32 color = Theme::kTextPrimary);

    std::string text;
    ImFont* font = nullptr;                 // nullptr = 当前主字体（含图标 fallback）
    float font_size = 0.0f;                 // 0 = Theme::kFontBody
    ImU32 color = Theme::kTextPrimary;
    ImU32 shadow_color = 0;                 // alpha > 0 时给文字加 1px 投影
    TextAlign text_align = TextAlign::Left;
    VerticalAlign vertical_align = VerticalAlign::Middle;
    float wrap_width = 0.0f;                // 0 = 用 size.x；都未设置则按可用宽度换行
    bool wrap_to_available = true;
    bool single_line = true;                // true = 不换行（超出部分按 wrap_width 裁切显示）

    Label& SetText(std::string value);
    Label& SetColor(ImU32 value);
    Label& SetFontSize(float value);
    Label& SetAlign(TextAlign horizontal, VerticalAlign vertical);
    Label& SetWrap(float width);
    Label& SetShadow(ImU32 value);
    // 等效字号语义
    float ResolvedFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;

private:
    float EffectiveWrap(float available) const;
};

} // namespace gui_dev::cv
