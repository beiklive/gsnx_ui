#include "component_view/components/Badge.h"

#include <utility>

#include "component_view/Draw.h"

namespace gui_dev::cv {

PlatformBadgeInfo PlatformBadgeInfoOf(EmuPlatform platform) {
    // 机种 id 顺序 = EmuPlatform 枚举顺序（1 起），底色 alpha 一律 220
    switch (platform) {
    case EmuPlatform::GBA:
        return {"GBA", Theme::rgba(108, 77, 191, 220)};
    case EmuPlatform::GBC:
        return {"GBC", Theme::rgba(0, 112, 221, 220)};
    case EmuPlatform::GB:
        return {"GB", Theme::rgba(0, 168, 107, 220)};
    case EmuPlatform::FC:
        return {"FC", Theme::rgba(218, 41, 28, 220)};
    case EmuPlatform::SFC:
        return {"SFC", Theme::rgba(160, 100, 180, 220)};
    case EmuPlatform::NDS:
        return {"NDS", Theme::rgba(54, 150, 190, 220)};
    case EmuPlatform::_3DS:
        return {"3DS", Theme::rgba(230, 79, 91, 220)};
    case EmuPlatform::MD:
        return {"MD", Theme::rgba(23, 55, 139, 220)};
    case EmuPlatform::Arcade:
        return {"Arcade", Theme::rgba(236, 134, 44, 220)};
    case EmuPlatform::DC:
        return {"DC", Theme::rgba(0, 142, 180, 220)};
    case EmuPlatform::PSP:
        return {"PSP", Theme::rgba(67, 118, 226, 220)};
    case EmuPlatform::PS1:
        return {"PS1", Theme::rgba(74, 74, 82, 220)};
    case EmuPlatform::Saturn:
        return {"Saturn", Theme::rgba(68, 82, 150, 220)};
    case EmuPlatform::Dolphin:
        return {"GC / Wii", Theme::rgba(54, 102, 196, 220)};
    case EmuPlatform::Unknown:
        break;
    }
    // 其它：文字为空（按规格，空文字不画徽标），底色给个中性灰
    return {"", Theme::rgba(100, 100, 100, 200)};
}

BadgeStyleParams BadgeStyleOf(BadgeStyle style) {
    BadgeStyleParams p;
    switch (style) {
    case BadgeStyle::GridListDetail:
        p = BadgeStyleParams{}; // 默认就是这一档：12 / 20 / 36 / pad 8 / 圆角 4 / 主题文字色
        break;
    case BadgeStyle::IisuCover:
        p.font_size = 12.0f;
        p.height = 17.0f;
        p.min_width = 30.0f;
        p.pad_x = 14.0f;
        p.radius = -1.0f; // 胶囊
        p.white_text = true;
        break;
    case BadgeStyle::GameDataView:
        p.font_size = 14.0f;
        p.height = 26.0f;
        p.min_width = 62.0f;
        p.pad_x = 8.0f;
        p.radius = 5.0f;
        p.fixed_width = true;
        p.white_text = true;
        p.fixed_blue_bg = true;
        break;
    case BadgeStyle::GridItem:
        p.font_size = 12.0f;
        p.height = 20.0f;
        p.min_width = 36.0f;
        p.pad_x = 8.0f;
        p.radius = 4.0f;
        p.long_text_min_width = 58.0f;
        p.white_text = true;
        break;
    }
    return p;
}

Badge::Badge() : Widget("badge") {
    padding = EdgeInsets{}; // 徽标自身没有内边距，全部走 pad_x
    focusable = false;      // 纯展示，不进焦点导航
}

Badge::Badge(EmuPlatform value) : Badge() {
    setPlatform(value);
}

Badge& Badge::setPlatform(EmuPlatform value) {
    platform = value;
    const PlatformBadgeInfo info = PlatformBadgeInfoOf(value);
    text = info.text;
    background = info.background;
    return *this;
}

Badge& Badge::setText(std::string value) {
    text = std::move(value);
    return *this;
}

Badge& Badge::setColors(ImVec4 bg, ImVec4 fg) {
    background = bg;
    foreground = fg;
    foreground_follows_theme_ = false; // 显式设过色：切主题不动
    return *this;
}

Badge& Badge::setFontSize(float value) {
    font_size = Maxf(value, 1.0f);
    return *this;
}

Badge& Badge::setHeight(float value) {
    height = Maxf(value, 1.0f);
    return *this;
}

Badge& Badge::setMinWidth(float value) {
    min_width = Maxf(value, 0.0f);
    return *this;
}

Badge& Badge::setPadding(float value) {
    pad_x = Maxf(value, 0.0f);
    return *this;
}

Badge& Badge::setRadius(float value) {
    radius = value;
    return *this;
}

Badge& Badge::setFixedWidth(bool value) {
    fixed_width = value;
    return *this;
}

Badge& Badge::setAlpha(float value) {
    alpha = Clampf(value, 0.0f, 1.0f);
    return *this;
}

Badge& Badge::setStyle(BadgeStyle style) {
    const BadgeStyleParams p = BadgeStyleOf(style);
    font_size = p.font_size;
    height = p.height;
    min_width = p.min_width;
    pad_x = p.pad_x;
    radius = p.radius;
    fixed_width = p.fixed_width;
    long_text_min_width = p.long_text_min_width;
    // 底色：GameDataView 固定蓝，其余用平台色（setPlatform 注入的那个）
    if (p.fixed_blue_bg) {
        background = Theme::rgba(79, 153, 222, 205);
    } else {
        background = PlatformBadgeInfoOf(platform).background;
    }
    if (p.white_text) {
        foreground = (style == BadgeStyle::GameDataView) ? Theme::rgba(255, 255, 255, 245)
                                                         : Theme::rgba(255, 255, 255, 255);
        foreground_follows_theme_ = false;
    } else {
        foreground = Theme::kTextPrimary;
        foreground_follows_theme_ = true;
    }
    return *this;
}

Badge& Badge::moveTo(float x, float y) {
    position = ImVec2(x, y);
    return *this;
}

float Badge::badgeWidth() const {
    if (text.empty()) {
        return 0.0f;
    }
    if (fixed_width) {
        return min_width;
    }
    const float text_w = Draw::MeasureText(nullptr, font_size, text.c_str(), 0.0f).x;
    float width = Maxf(min_width, text_w + pad_x * 2.0f);
    if (long_text_min_width > 0.0f && text.size() > 3) {
        width = Maxf(width, long_text_min_width);
    }
    return width;
}

ImVec2 Badge::MeasureContent(const ImVec2& available) {
    (void)available;
    return ImVec2(badgeWidth(), height);
}

void Badge::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (text.empty()) {
        return; // 规格：空文字不画徽标（其它机种就是这种）
    }
    const float r = resolvedRadius();
    Draw::RoundedRectFilled(dl, content, Theme::U32(background, alpha), r, r, r, r);
    // NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE：文本落在徽标中心
    const ImVec2 extent = Draw::MeasureText(nullptr, font_size, text.c_str(), 0.0f);
    Draw::Text(dl, nullptr, font_size,
               ImVec2(content.Center().x - extent.x * 0.5f, content.Center().y - extent.y * 0.5f),
               Theme::U32(foreground, alpha), text.c_str());
}

void Badge::OnThemeChanged() {
    if (foreground_follows_theme_) {
        foreground = Theme::kTextPrimary; // 浅色主题就是深字
    }
}

} // namespace gui_dev::cv
