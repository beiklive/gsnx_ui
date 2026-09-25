#include "component_view/components/Header.h"

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 覆盖色（alpha > 0）优先，否则跟主题角色色
ImVec4 Pick(const ImVec4& override_color, const ImVec4& theme_color) {
    return override_color.w > 0.0f ? override_color : theme_color;
}

} // namespace

Header::Header() : Widget("header") {
    focusable = false; // 标题不是焦点停靠点
    background = 0;
    border.width = 0.0f;
    shadow.enabled = false;
}

Header::Header(std::string value) : Header() {
    title = std::move(value);
}

Header& Header::setTitle(std::string value) {
    title = std::move(value);
    return *this;
}

Header& Header::setInfo(std::string value) {
    info = std::move(value);
    return *this;
}

Header& Header::setBarColor(const ImVec4& color) {
    bar_color_ = color;
    return *this;
}

Header& Header::setTextColor(const ImVec4& color) {
    text_color_ = color;
    return *this;
}

Header& Header::setDividerColor(const ImVec4& color) {
    divider_color_ = color;
    return *this;
}

ImVec2 Header::MeasureContent(const ImVec2& available) {
    const float title_w = Draw::MeasureText(nullptr, style.text_size, title.c_str(), 0.0f).x;
    const float info_w =
        info.empty() ? 0.0f : (style.info_gap + Draw::MeasureText(nullptr, style.info_size, info.c_str(), 0.0f).x);
    const float width = style.text_offset_x + title_w + info_w + style.divider_inset_right;
    return ImVec2(Minf(width, available.x > 0.0f ? available.x : width), style.height);
}

void Header::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr) {
        return;
    }
    const float opacity = EffectiveOpacity();
    if (opacity <= 0.002f) {
        return;
    }
    const float center_y = content.Center().y;

    // 1) 左侧竖条
    if (style.bar_width > 0.0f && style.bar_height > 0.0f) {
        const Rect bar = Rect::FromPosSize(
            ImVec2(content.min.x + style.bar_offset_x, center_y - style.bar_height * 0.5f),
            ImVec2(style.bar_width, style.bar_height));
        const ImU32 color = Theme::Alpha(Theme::U32(Pick(bar_color_, Theme::kAccent)), opacity);
        Draw::RoundedRectFilled(dl, bar, color, style.bar_radius, style.bar_radius, style.bar_radius, style.bar_radius);
    }

    // 2) 标题（垂直居中）
    const ImU32 title_color = Theme::Alpha(Theme::U32(Pick(text_color_, Theme::kTextPrimary)), opacity);
    const ImVec2 title_extent = Draw::MeasureText(nullptr, style.text_size, title.c_str(), 0.0f);
    float text_x = content.min.x + style.text_offset_x;
    Draw::Text(dl, nullptr, style.text_size, ImVec2(text_x, center_y - title_extent.y * 0.5f), title_color,
               title.c_str());
    text_x += title_extent.x;

    // 3) 右侧补充文字
    if (!info.empty()) {
        const ImVec2 info_extent = Draw::MeasureText(nullptr, style.info_size, info.c_str(), 0.0f);
        const float info_x = content.max.x - style.divider_inset_right - info_extent.x;
        if (info_x > text_x + style.info_gap) {
            Draw::Text(dl, nullptr, style.info_size, ImVec2(info_x, center_y - info_extent.y * 0.5f),
                       Theme::Alpha(Theme::U32(Theme::kTextMuted), opacity), info.c_str());
        }
    }

    // 4) 底部分隔线（把上下两段内容水平分开）
    if (style.divider && style.divider_width > 0.0f) {
        const ImVec4 base = Pick(divider_color_, Theme::kBorder);
        if (base.w > 0.0f) {
            const float line_y = content.max.y - style.divider_offset_y;
            const float x0 = content.min.x + style.divider_inset_x;
            const float x1 = content.max.x - style.divider_inset_right;
            if (x1 > x0) {
                dl->AddLine(ImVec2(x0, line_y), ImVec2(x1, line_y), Theme::Alpha(Theme::U32(base), opacity),
                            style.divider_width);
            }
        }
    }
}

} // namespace gui_dev::cv
