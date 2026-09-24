// 控件页里反复用到的小排版工具（header-only，避免每个页面重复写同样的几行）。
#pragma once

#include <string>

#include "component_view/Theme.h"
#include "component_view/Widget.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"

namespace gui_dev::cv::helpers {

// 一行文字（默认小号 + 次要色）
inline Label* Caption(Widget* parent, std::string text, float size = Theme::kFontSmall,
                      ImU32 color = Theme::kTextMuted) {
    return parent->Emplace<Label>(std::move(text), size, color);
}

// 横向排列的容器
inline Box* Row(Widget* parent, float gap = 14.0f, const EdgeInsets& padding = EdgeInsets{}) {
    Box* box = parent->Emplace<Box>("row");
    box->layout = LayoutMode::Horizontal;
    box->gap = ImVec2(gap, 0.0f);
    box->align_y = Align::Center;
    box->padding = padding;
    return box;
}

// 纵向排列的容器
inline Box* Column(Widget* parent, float gap = 10.0f, const EdgeInsets& padding = EdgeInsets{}) {
    Box* box = parent->Emplace<Box>("column");
    box->layout = LayoutMode::Vertical;
    box->gap = ImVec2(0.0f, gap);
    box->align_x = Align::Center;
    box->padding = padding;
    return box;
}

// 展示用的小方块（Surface 底 + 边框 + 圆角），内部居中文案
inline Box* Tile(Widget* parent, std::string caption, float width, float height, float radius = Theme::kRadius,
                 ImU32 background = Theme::kBgWidget, ImU32 border_color = Theme::kBorder) {
    Box* tile = parent->Emplace<Box>("tile:" + caption);
    tile->SetSize(width, height);
    tile->SetBackground(background);
    tile->SetBorder(1.0f, border_color);
    tile->SetRadius(radius);
    tile->layout = LayoutMode::Vertical;
    tile->align_x = Align::Center;
    tile->align_y = Align::Center;
    tile->padding = EdgeInsets::All(8.0f);
    if (!caption.empty()) {
        Label* label = tile->AddLabel(std::move(caption), Theme::kFontSmall, Theme::kTextPrimary);
        label->SetAlign(TextAlign::Center, VerticalAlign::Middle);
        label->ellipsis = true;
    }
    return tile;
}

// 属性行里显示 key:value 的便捷函数
inline std::string Prop(const char* name, const std::string& value) {
    return std::string(name) + " = " + value;
}

inline std::string YesNo(bool value) {
    return value ? "是" : "否";
}

} // namespace gui_dev::cv::helpers
