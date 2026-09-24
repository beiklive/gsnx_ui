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
    Box& fillWith(ImU32 color);              // 底色
    Box& roundCorners(float value);          // 圆角
    Box& outline(float width, ImU32 color);  // 边框
    Box& dropShadow(float blur, ImU32 color); // 阴影
};

} // namespace gui_dev::cv
