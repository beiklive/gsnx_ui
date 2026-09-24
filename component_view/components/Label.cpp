#include "component_view/components/Label.h"

#include "component_view/Draw.h"

namespace gui_dev::cv {

Label::Label() : Widget("label") {}

Label::Label(std::string value, float size, ImU32 text_color) : Widget("label") {
    text = std::move(value);
    font_size = size;
    color = text_color;
}

Label& Label::SetText(std::string value) {
    text = std::move(value);
    return *this;
}

Label& Label::SetColor(ImU32 value) {
    color = value;
    return *this;
}

Label& Label::SetFontSize(float value) {
    font_size = value;
    return *this;
}

Label& Label::SetAlign(TextAlign horizontal, VerticalAlign vertical) {
    text_align = horizontal;
    vertical_align = vertical;
    return *this;
}

Label& Label::SetWrap(float width) {
    wrap_width = width;
    single_line = width <= 0.0f ? single_line : false;
    return *this;
}

Label& Label::SetShadow(ImU32 value) {
    shadow_color = value;
    return *this;
}

float Label::EffectiveWrap(float available) const {
    if (wrap_width > 0.0f) {
        return wrap_width;
    }
    if (size.x > 0.0f) {
        return Maxf(size.x - padding.Horizontal(), 1.0f);
    }
    if (wrap_to_available) {
        return Maxf(available, 1.0f);
    }
    return 0.0f;
}

ImVec2 Label::MeasureContent(const ImVec2& available) {
    const float size = ResolvedFontSize();
    if (single_line) {
        return Draw::MeasureText(font, size, text.c_str(), 0.0f);
    }
    return Draw::MeasureText(font, size, text.c_str(), EffectiveWrap(available.x));
}

void Label::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (text.empty()) {
        return;
    }
    const float size = ResolvedFontSize();
    const float wrap = single_line ? 0.0f : EffectiveWrap(content.Width());
    const ImVec2 extent = Draw::MeasureText(font, size, text.c_str(), wrap);

    ImVec2 pos = content.min;
    switch (text_align) {
    case TextAlign::Left:
        pos.x = content.min.x;
        break;
    case TextAlign::Center:
        pos.x = content.Center().x - extent.x * 0.5f;
        break;
    case TextAlign::Right:
        pos.x = content.max.x - extent.x;
        break;
    }
    switch (vertical_align) {
    case VerticalAlign::Top:
        pos.y = content.min.y;
        break;
    case VerticalAlign::Middle:
        pos.y = content.Center().y - extent.y * 0.5f;
        break;
    case VerticalAlign::Bottom:
        pos.y = content.max.y - extent.y;
        break;
    }

    if (((shadow_color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
        Draw::Text(dl, font, size, ImVec2(pos.x + 1.0f, pos.y + 1.0f), Tint(shadow_color), text.c_str(), wrap);
    }
    Draw::Text(dl, font, size, pos, Tint(color), text.c_str(), wrap);
}

} // namespace gui_dev::cv
