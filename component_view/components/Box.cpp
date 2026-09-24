#include "component_view/components/Box.h"

#include <utility>

namespace gui_dev::cv {

Box::Box() : Widget("box") {
    // 默认给一个可见的底色，免得新建出来「什么都没有」看不出位置
    background = Theme::U32(Theme::kBgWidget);
    corner_radius = Theme::kRadius;
}

Box::Box(std::string widget_name) : Box() {
    name = std::move(widget_name);
}

Box& Box::moveTo(float x, float y) {
    position = ImVec2(x, y);
    return *this;
}

Box& Box::resize(float width, float height) {
    size = ImVec2(width, height);
    return *this;
}

Box& Box::fillWith(ImU32 color) {
    background = color;
    return *this;
}

Box& Box::roundCorners(float value) {
    corner_radius = value;
    return *this;
}

Box& Box::outline(float width, ImU32 color) {
    border.width = width;
    border.color = color;
    return *this;
}

Box& Box::dropShadow(float blur, ImU32 color) {
    shadow.enabled = true;
    shadow.blur = blur;
    shadow.color = color;
    shadow.offset = ImVec2(0.0f, 4.0f);
    return *this;
}

} // namespace gui_dev::cv
