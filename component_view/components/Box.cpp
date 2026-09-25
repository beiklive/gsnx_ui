#include "component_view/components/Box.h"

#include <utility>

#include "component_view/Global.h"

namespace gui_dev::cv {

Box::Box() : Widget("box") {
    // 默认给一个可见的底色（来自调色板，切主题会跟着变），免得新建出来「什么都没有」
    background = Theme::U32(Theme::kBgWidget);
    background_follows_theme = true;
    applyComponentStyle();
}

Box& Box::applyComponentStyle() {
    // 约定样式来自全局变量，改 Global::component_style 后调这个（或重建）即可生效
    const Global::ComponentStyle& style = Global::component_style;
    border.width = style.border_width;
    border.color = Theme::U32(style.border_color);
    corner_radius = style.corner_radius;
    shadow.enabled = true;
    shadow.offset = style.shadow_offset;
    shadow.blur = style.shadow_blur;
    shadow.color = Theme::U32(style.shadow_color);
    return *this;
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
    background_follows_theme = false; // 显式设过色：不再跟随主题
    return *this;
}

Box& Box::fillWith(const ImVec4& color) {
    background = Theme::U32(color);
    background_follows_theme = false;
    return *this;
}

void Box::OnThemeChanged() {
    if (background_follows_theme && background != 0) { // 0 = 透明，别给透明节点补底色
        background = Theme::U32(Theme::kBgWidget);
    }
    applyComponentStyle(); // 边框色 / 阴影浓淡也来自约定样式
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

Box& Box::makeFocusable(bool value) {
    focusable = value;
    // 可聚焦的 Box 默认就有可见反馈，不然按方向键看不出选中的是哪个
    if (value) {
        focusVisual();
    }
    return *this;
}

Box& Box::focusVisual(float scale, const ImVec2& translate, float frame_offset, ImU32 frame_color) {
    focus_frame = true;
    focus_frame_offset = frame_offset;
    focus_frame_color = frame_color;
    focus_scale = scale;
    focus_translate = translate;
    return *this;
}

Box& Box::focusOnlySelf(bool value) {
    focus_only_self = value;
    return *this;
}

} // namespace gui_dev::cv
