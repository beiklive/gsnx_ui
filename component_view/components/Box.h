// Box：容器组件。矩形底 + 圆角 + 边框 + 阴影 + 子节点排列。
// 所有视觉属性都来自 Widget 基类，这里只提供常用预设与内容便捷方法。
#pragma once

#include "component_view/components/Label.h"

namespace gui_dev::cv {

class Box : public Widget {
public:
    Box();
    explicit Box(std::string widget_name);

    // ---- 外观预设 ----------------------------------------------------------
    Box& Surface();                    // 面板：sidebar 底色 + 1px 边框 + 8 圆角
    Box& Card();                       // 卡片：widget 底色 + 软阴影 + 12 圆角
    Box& Toolbar();                    // 工具条：activity 底色
    Box& Outlined(ImU32 border_color); // 只描边
    Box& Glow(ImU32 accent);           // 强调色描边 + 柔光

    // ---- 内容 --------------------------------------------------------------
    Label* AddLabel(std::string text, float font_size = 0.0f, ImU32 color = Theme::kTextPrimary);
    // 纵向排列 + 统一留白（最常用的面板内部布局）
    Box& Stack(const EdgeInsets& inner_padding = EdgeInsets::All(Theme::kGap), float spacing = Theme::kGapSmall);
    // 横向排列
    Box& Row(const EdgeInsets& inner_padding = EdgeInsets::All(Theme::kGap), float spacing = Theme::kGapSmall);
};

} // namespace gui_dev::cv
