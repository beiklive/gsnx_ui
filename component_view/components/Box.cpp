#include "component_view/components/Box.h"

namespace gui_dev::cv {

Box::Box() : Widget("box") {}

Box::Box(std::string widget_name) : Widget(std::move(widget_name)) {}

Box& Box::Surface() {
    background = Theme::kBgSideBar;
    corner_radius = Theme::kRadius;
    border = BorderStyle{1.0f, Theme::kBorder, 0.0f};
    padding = EdgeInsets::All(Theme::kGap);
    return *this;
}

Box& Box::Card() {
    background = Theme::kBgWidget;
    corner_radius = Theme::kRadius + 4.0f;
    border = BorderStyle{1.0f, Theme::kBorder, 0.0f};
    shadow = ShadowStyle::Soft(22.0f);
    shadow.offset = ImVec2(0.0f, 6.0f);
    shadow.color = Theme::kShadow;
    padding = EdgeInsets::All(Theme::kGap);
    return *this;
}

Box& Box::Toolbar() {
    background = Theme::kBgActivity;
    corner_radius = Theme::kRadiusSmall;
    border = BorderStyle{0.0f, 0, 0.0f};
    padding = EdgeInsets::Symmetric(Theme::kGap, Theme::kGapSmall);
    return *this;
}

Box& Box::Outlined(ImU32 border_color) {
    background = 0;
    border = BorderStyle{1.5f, border_color, 0.0f};
    corner_radius = Theme::kRadius;
    return *this;
}

Box& Box::Glow(ImU32 accent) {
    Outlined(accent);
    shadow.enabled = true;
    shadow.offset = ImVec2(0.0f, 0.0f);
    shadow.blur = 24.0f;
    shadow.color = Theme::Alpha(accent, 0.5f);
    return *this;
}

Label* Box::AddLabel(std::string text, float font_size, ImU32 color) {
    auto label = std::make_unique<Label>(std::move(text), font_size, color);
    Label* raw = label.get();
    Add(std::move(label));
    return raw;
}

Box& Box::Stack(const EdgeInsets& inner_padding, float spacing) {
    layout = LayoutMode::Vertical;
    gap = ImVec2(0.0f, spacing);
    padding = inner_padding;
    return *this;
}

Box& Box::Row(const EdgeInsets& inner_padding, float spacing) {
    layout = LayoutMode::Horizontal;
    gap = ImVec2(spacing, 0.0f);
    padding = inner_padding;
    return *this;
}

} // namespace gui_dev::cv
