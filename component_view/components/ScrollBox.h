// ScrollBox：滚动容器（手柄版）。
//
// 不用鼠标滚轮，全靠按键：
//   ↑ ↓        焦点在子控件之间移动，容器自动把焦点滚进可见区（Page 统一调用 EnsureVisible）
//   L / R      按页滚动
//   ZL / ZR    快速翻页
//   A          由子控件自己处理
//
// 支持：垂直/水平/双向、平滑滚动、越界回弹、吸附、滚动条自动隐藏、内容裁剪。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class ScrollBox : public Widget {
public:
    enum class Direction { Vertical, Horizontal, Both };

    ScrollBox();
    explicit ScrollBox(std::string widget_name);

    Direction direction = Direction::Vertical;
    float page_scale = 0.85f;   // 一次翻页滚动多少屏
    bool show_hint = true;      // 有滚动空间时显示方向提示箭头
    bool snap_items = false;    // 目标吸附到子项高度

    ImU32 content_bg = 0;
    ImU32 hint_color = Theme::kTextMuted;

    ScrollBox& SetDirection(Direction value);
    ScrollBox& SetPadding(const EdgeInsets& value);
    ScrollBox& SetGap(const ImVec2& value);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    bool OnPadAction(InputAction action) override;
};

} // namespace gui_dev::cv
