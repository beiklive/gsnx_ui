#include "component_view/components/ScrollBox.h"

#include "component_view/Draw.h"

namespace gui_dev::cv {

ScrollBox::ScrollBox() : Widget("scroll_box") {
    overflow = Overflow::Scroll;
    scroll_bar = true;
    scroll_bar_auto_hide = true;
    scroll_overscroll = true;
    padding = EdgeInsets::All(9.0f);
    layout = LayoutMode::Vertical;
    gap = ImVec2(0.0f, 7.0f);
    align_x = Align::Stretch;
}

ScrollBox::ScrollBox(std::string widget_name) : ScrollBox() {
    name = std::move(widget_name);
}

ScrollBox& ScrollBox::SetDirection(Direction value) {
    direction = value;
    return *this;
}

ScrollBox& ScrollBox::SetPadding(const EdgeInsets& value) {
    padding = value;
    return *this;
}

ScrollBox& ScrollBox::SetGap(const ImVec2& value) {
    gap = value;
    return *this;
}

ImVec2 ScrollBox::MeasureContent(const ImVec2& available) {
    // 滚动容器自己不撑大，尺寸由使用方（页面）给；这里只保证有最小值
    return ImVec2(Minf(available.x, 120.0f), Minf(available.y, 120.0f));
}

bool ScrollBox::OnPadAction(InputAction action) {
    switch (action) {
    case InputAction::PageLeft:
        ScrollPage(-1, page_scale);
        return true;
    case InputAction::PageRight:
        ScrollPage(1, page_scale);
        return true;
    case InputAction::TriggerLeft:
        ScrollPage(-1, 2.0f);
        return true;
    case InputAction::TriggerRight:
        ScrollPage(1, 2.0f);
        return true;
    case InputAction::Menu:
        scroll_target = ImVec2(0.0f, 0.0f);
        return true;
    default:
        return false;
    }
}

void ScrollBox::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (content_bg != 0) {
        Draw::RoundedRectFilled(dl, DrawRect(), Tint(content_bg), CornerTL() * DrawScale(), CornerTR() * DrawScale(),
                                CornerBL() * DrawScale(), CornerBR() * DrawScale());
    }
    if (!show_hint) {
        return;
    }
    const float scale = DrawScale();
    // 上下还有内容时给个提示箭头（左右同理）
    const bool can_up = scroll.y > 1.0f;
    const bool can_down = scroll.y < scroll_max.y - 1.0f;
    const bool can_left = scroll.x > 1.0f;
    const bool can_right = scroll.x < scroll_max.x - 1.0f;
    const ImU32 color = Theme::Alpha(hint_color, 0.75f * EffectiveOpacity());

    if (can_down) {
        const ImVec2 corner(content.Center().x, content.max.y - 6.0f * scale);
        dl->AddTriangleFilled(ImVec2(corner.x - 7.0f * scale, corner.y - 6.0f * scale),
                              ImVec2(corner.x + 7.0f * scale, corner.y - 6.0f * scale),
                              ImVec2(corner.x, corner.y + 3.0f * scale), color);
    }
    if (can_up) {
        const ImVec2 corner(content.Center().x, content.min.y + 6.0f * scale);
        dl->AddTriangleFilled(ImVec2(corner.x - 7.0f * scale, corner.y + 6.0f * scale),
                              ImVec2(corner.x + 7.0f * scale, corner.y + 6.0f * scale),
                              ImVec2(corner.x, corner.y - 3.0f * scale), color);
    }
    if (can_right) {
        const ImVec2 corner(content.max.x - 6.0f * scale, content.Center().y);
        Draw::TriangleRight(dl, corner, 14.0f * scale, color);
    }
    if (can_left) {
        const ImVec2 corner(content.min.x + 6.0f * scale, content.Center().y);
        dl->AddTriangleFilled(ImVec2(corner.x + 7.0f * scale, corner.y - 7.0f * scale),
                              ImVec2(corner.x + 7.0f * scale, corner.y + 7.0f * scale),
                              ImVec2(corner.x - 3.0f * scale, corner.y), color);
    }
}

} // namespace gui_dev::cv
