#include "component_view/components/ImageButton.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

ImageButton::ImageButton() : Widget("image_button") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = true;
    focus_frame_offset = 4.0f;
    focus_frame_width = 2.5f;
    focus_scale = focus_scale_amount;
    corner_radius = Theme::kRadiusLarge;
}

ImageButton::ImageButton(ImTextureRef texture_ref, const ImVec2& size) : ImageButton() {
    SetTexture(texture_ref, size);
}

ImageButton& ImageButton::SetTexture(ImTextureRef texture_ref, const ImVec2& size) {
    image_normal = texture_ref;
    native_size = size;
    return *this;
}

ImageButton& ImageButton::SetUV(const ImVec2& min_uv, const ImVec2& max_uv) {
    uv0 = min_uv;
    uv1 = max_uv;
    return *this;
}

ImageButton& ImageButton::SetBadge(std::string value) {
    badge = std::move(value);
    return *this;
}

ImageButton& ImageButton::SetCaption(std::string value) {
    caption = std::move(value);
    return *this;
}

ImageButton& ImageButton::SetFocusImage(ImTextureRef texture_ref) {
    image_focus = texture_ref;
    return *this;
}

ImageButton& ImageButton::SetPressedImage(ImTextureRef texture_ref) {
    image_pressed = texture_ref;
    return *this;
}

ImageButton& ImageButton::SetSelectedImage(ImTextureRef texture_ref) {
    image_selected = texture_ref;
    return *this;
}

ImageButton& ImageButton::SetDisabledImage(ImTextureRef texture_ref) {
    image_disabled = texture_ref;
    return *this;
}

ImageButton& ImageButton::SetPlaceholder(ImU32 background, std::string icon) {
    placeholder_bg = background;
    placeholder_icon = std::move(icon);
    return *this;
}

ImageButton& ImageButton::SetFit(Fit value) {
    fit = value;
    return *this;
}

ImVec2 ImageButton::MeasureContent(const ImVec2& available) {
    (void)available;
    return native_size;
}

ImTextureRef ImageButton::ResolveTexture() const {
    if (!enabled && image_disabled.GetTexID() != ImTextureID_Invalid) {
        return image_disabled;
    }
    if (pressed && image_pressed.GetTexID() != ImTextureID_Invalid) {
        return image_pressed;
    }
    if (selected && image_selected.GetTexID() != ImTextureID_Invalid) {
        return image_selected;
    }
    if (focused && image_focus.GetTexID() != ImTextureID_Invalid) {
        return image_focus;
    }
    return image_normal;
}

ImU32 ImageButton::ResolveTint() const {
    if (!enabled) {
        return Theme::Alpha(tint_normal, 0.4f);
    }
    if (selected) {
        return tint_selected;
    }
    return tint_normal;
}

void ImageButton::OnUpdate(float dt) {
    hover_mix_ = SmoothTo(hover_mix_, (hovered || focused) && enabled ? 1.0f : 0.0f, transition_speed, dt);
    press_mix_ = SmoothTo(press_mix_, (pressed && enabled) ? 1.0f : 0.0f, transition_speed * 1.5f, dt);

    // 焦点缩放 + 按压缩放叠加
    const float scale = focus_scale_amount + (1.0f - focus_scale_amount) * (1.0f - focus_mix);
    const float press = 1.0f + (press_scale_amount - 1.0f) * press_mix_;
    const float total = scale * press;
    visual_scale = ImVec2(total, total);
}

void ImageButton::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float radius = CornerTL() * scale;
    const ImTextureRef texture_ref = ResolveTexture();
    const bool has_texture = texture_ref.GetTexID() != ImTextureID_Invalid;

    Rect image_rect = content;
    if (has_texture && native_size.x > 0.0f && native_size.y > 0.0f && fit != Fit::Stretch) {
        const float sx = content.Width() / native_size.x;
        const float sy = content.Height() / native_size.y;
        const float s = fit == Fit::Cover ? Maxf(sx, sy) : Minf(sx, sy);
        const ImVec2 fitted(native_size.x * s, native_size.y * s);
        image_rect = Rect::FromPosSize(
            ImVec2(content.Center().x - fitted.x * 0.5f, content.Center().y - fitted.y * 0.5f), fitted);
    }

    const bool clip = (fit == Fit::Cover && has_texture) || (!caption_inside && !caption.empty());
    if (clip) {
        dl->PushClipRect(content.min, content.max, true);
    }

    if (has_texture) {
        const ImDrawFlags flags = Draw::CornerFlags(CornerTL(), CornerTR(), CornerBL(), CornerBR());
        dl->AddImageRounded(texture_ref, image_rect.min, image_rect.max, uv0, uv1, Tint(ResolveTint()),
                            Draw::MaxCorner(CornerTL(), CornerTR(), CornerBL(), CornerBR()) * scale, flags);
    } else {
        Draw::RoundedRectFilled(dl, image_rect, Tint(placeholder_bg), radius, radius, radius, radius);
        if (!placeholder_icon.empty()) {
            const float icon_size = Minf(image_rect.Width(), image_rect.Height()) * 0.42f;
            const ImVec2 extent = Draw::MeasureText(nullptr, icon_size, placeholder_icon.c_str(), 0.0f);
            Draw::Text(dl, nullptr, icon_size,
                       ImVec2(image_rect.Center().x - extent.x * 0.5f, image_rect.Center().y - extent.y * 0.5f),
                       Tint(Theme::kTextMuted), placeholder_icon.c_str());
        }
    }

    // 选中标记（左上角一条强调色带）
    if (selected) {
        const Rect marker = Rect::FromPosSize(content.min, ImVec2(5.0f * scale, content.Height()));
        Draw::RoundedRectFilled(dl, marker, Theme::kAccent, 2.0f * scale, 0.0f, 0.0f, 2.0f * scale);
    }

    // 焦点/悬停高亮：内描边 + 底部渐暗，让焦点图标浮起来
    const float highlight = Maxf(hover_mix_, focus_mix);
    if (highlight > 0.02f) {
        Draw::RoundedRectOutline(dl, image_rect.Expanded(-1.0f * scale),
                                 Theme::Alpha(focus_border_color, highlight * 0.95f), 2.0f * scale, radius, radius,
                                 radius, radius);
    }

    if (caption_inside && !caption.empty()) {
        // 注意：局部变量不能和成员同名，否则初始化表达式读到的是自己（未初始化）
        const float strip_font = (this->caption_size > 0.0f ? this->caption_size : Theme::kFontSmall) * scale;
        const float strip_height = strip_font + 16.0f * scale;
        const Rect strip = Rect::FromPosSize(ImVec2(content.min.x, content.max.y - strip_height),
                                             ImVec2(content.Width(), strip_height));
        Draw::RoundedRectFilled(dl, strip, Theme::Alpha(IM_COL32(0, 0, 0, 255), 0.62f), 0.0f, 0.0f, radius, radius);
        const char* shown = Draw::Ellipsize(nullptr, strip_font, caption.c_str(), strip.Width() - 16.0f * scale);
        const ImVec2 extent = Draw::MeasureText(nullptr, strip_font, shown, 0.0f);
        Draw::Text(dl, nullptr, strip_font,
                   ImVec2(strip.Center().x - extent.x * 0.5f, strip.Center().y - extent.y * 0.5f),
                   Tint(focused ? Theme::kTextBright : Theme::kTextPrimary), shown);
    }

    if (clip) {
        dl->PopClipRect();
    }

    if (!badge.empty()) {
        const float badge_size = Theme::kFontTiny * scale;
        const ImVec2 extent = Draw::MeasureText(nullptr, badge_size, badge.c_str(), 0.0f);
        const float pad = 8.0f * scale;
        const Rect badge_rect = Rect::FromPosSize(ImVec2(content.max.x - extent.x - pad * 2.0f - 6.0f * scale,
                                                          content.min.y + 6.0f * scale),
                                                  ImVec2(extent.x + pad * 2.0f, badge_size + 8.0f * scale));
        Draw::RoundedRectFilled(dl, badge_rect, Tint(badge_bg), (badge_size + 8.0f * scale) * 0.5f,
                                (badge_size + 8.0f * scale) * 0.5f, (badge_size + 8.0f * scale) * 0.5f,
                                (badge_size + 8.0f * scale) * 0.5f);
        Draw::Text(dl, nullptr, badge_size,
                   ImVec2(badge_rect.Center().x - extent.x * 0.5f, badge_rect.Center().y - extent.y * 0.5f),
                   Tint(badge_fg), badge.c_str());
    }
}

} // namespace gui_dev::cv
