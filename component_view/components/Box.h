// Box：最基础的矩形组件（也是后面所有组件的容器）。
//
// 现在它只做一件事：在指定位置画一个指定大小的矩形（可选圆角 / 边框 / 阴影）。
// 位置、尺寸、圆角、边框、阴影这些属性都在父类 Widget 里，
// Box 只是给一组读起来更短的链式写法 + 一个默认底色，方便从它开始往外长。
//
// 注意：链式函数名不能和 Widget 的成员变量同名（border / shadow / size 都是成员变量），
// 所以这里用 outline / dropShadow / resize 这类名字。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Box : public Widget {
public:
    Box();
    explicit Box(std::string widget_name);

    // ---- 链式设置 ----------------------------------------------------------
    Box& moveTo(float x, float y);           // 位置（相对父节点内容区左上角）
    Box& resize(float width, float height);  // 尺寸（外框尺寸）
    Box& fillWith(ImU32 color);              // 底色（打包值）
    Box& fillWith(const ImVec4& color);      // 底色（Theme 的 rgba()/rgb() 结果）
    Box& roundCorners(float value);          // 圆角
    Box& outline(float width, ImU32 color);  // 边框
    Box& dropShadow(float blur, ImU32 color); // 阴影

    // 重新套用全局约定样式（Global::component_style：1px 灰白边框 / 5px 圆角 / 右下阴影）
    Box& applyComponentStyle();

    // ---- 焦点 --------------------------------------------------------------
    // Box 有两种身份，按需要选：
    //   容器：不调 makeFocusable()，它只负责排版/背景，焦点落在子节点上
    //   可聚焦控件：调 makeFocusable()，手柄/键盘能选中它，A 触发 clicked 信号
    //               （焦点时还可以叠加缩放/位移，见 focusVisual）
    // 函数名不能叫 focusable —— 那是 Widget 的成员变量，同名会遮蔽。
    Box& makeFocusable(bool value = true);
    // 焦点视觉：scale = 聚焦时的缩放，translate = 聚焦时的位移，frame_offset = 焦点框外扩
    Box& focusVisual(float scale = 1.04f, const ImVec2& translate = ImVec2(0.0f, 0.0f),
                     float frame_offset = 3.0f, ImU32 frame_color = Theme::U32(Theme::kAccent));
    // 复合控件语义：自己可聚焦，但不把子节点算进焦点导航（容器保持默认即可）
    Box& focusOnlySelf(bool value = true);
};

} // namespace gui_dev::cv
