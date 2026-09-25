// FocusRing：页面级的焦点框图层。
//
// 起因：焦点框原本由每个控件在自己的 OnDrawOverlay 里画（Widget 画单色框、Button 画流光框），
// 于是「焦点框长什么样」和「控件怎么画」绑在一起，而且每帧每个控件都要判一次。
// 现在改成：控件只通过 Widget::BuildFocusVisual() 描述自己想要的焦点框，
// 由这一层统一在「页面内容之上、Toast 之下」画一个（见 Page::Render）。
//
// 附带的行为：
//   * 平滑跟随：焦点在附近移动时框会滑过去（和 examples/pause_menu 的焦点框一个手感）；
//     目标跳太远（> kSnapDistance）就直接对齐，避免彩虹框横穿整个屏幕。
//   * 会裁到最近的滚动容器里，不会跑到面板外面。
//   * 一帧只画一个框，FocusVisual::alpha 负责淡入淡出。
#pragma once

#include <imgui.h>

#include "component_view/Types.h"

namespace gui_dev::cv {

class Widget;

class FocusRing {
public:
    // 清空状态（页面重进时调）
    void Reset();

    // 每帧调用一次：target 一般是 Global::focused（没有就传 nullptr，框会淡出）
    void Draw(ImDrawList* dl, Widget* target);

private:
    Rect rect_{};
    float radius_ = 0.0f;
    float width_ = 2.0f;
    float alpha_ = 0.0f;
    float phase_ = 0.0f;
    float saturation_ = 0.75f;
    float brightness_ = 1.0f;
    ImU32 color_ = 0;
    bool flowing_ = false;
    bool has_rect_ = false; // 还没有基准矩形 -> 下一个目标直接对齐，不从别处滑过来
};

} // namespace gui_dev::cv
