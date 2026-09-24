#include "component_view/components/Image.h"

#include <cmath>

#include "component_view/Draw.h"

namespace gui_dev::cv {

Image::Image() : Widget("image") {}

Image::Image(ImTextureRef texture_ref, const ImVec2& size) : Widget("image") {
    SetTexture(texture_ref, size);
}

void Image::SetTexture(ImTextureRef texture_ref, const ImVec2& size) {
    texture = texture_ref;
    native_size = size;
}

Image& Image::SetUV(const ImVec2& min_uv, const ImVec2& max_uv) {
    uv0 = min_uv;
    uv1 = max_uv;
    return *this;
}

Image& Image::Tinted(ImU32 color) {
    tint = color;
    return *this;
}

Image& Image::SetFit(Fit value) {
    fit = value;
    return *this;
}

ImVec2 Image::MeasureContent(const ImVec2& available) {
    (void)available;
    // size 为 0 时退化成原始像素尺寸（单个像素在 720p 设计空间里不是逻辑像素，仅作兜底）。
    return native_size;
}

void Image::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (!HasTexture() || content.Width() <= 0.0f || content.Height() <= 0.0f) {
        return;
    }

    ImVec2 uv_min = uv0;
    ImVec2 uv_max = uv1;
    Rect destination = content;

    if (native_size.x > 0.0f && native_size.y > 0.0f) {
        if (tiled) {
            uv_max = ImVec2(uv0.x + uv1.x * content.Width() / native_size.x,
                            uv0.y + uv1.y * content.Height() / native_size.y);
        } else if (fit != Fit::Stretch) {
            const float scale_x = content.Width() / native_size.x;
            const float scale_y = content.Height() / native_size.y;
            const float scale = fit == Fit::Cover ? Maxf(scale_x, scale_y) : Minf(scale_x, scale_y);
            const ImVec2 fitted(native_size.x * scale, native_size.y * scale);
            destination = Rect::FromPosSize(
                ImVec2(content.Center().x - fitted.x * 0.5f, content.Center().y - fitted.y * 0.5f), fitted);
        }
    }

    // Cover 会超出内容区，按内容区裁剪（圆角由 AddImageRounded 处理）。
    if (flip_x) {
        const float t = uv_min.x;
        uv_min.x = uv_max.x;
        uv_max.x = t;
    }
    if (flip_y) {
        const float t = uv_min.y;
        uv_min.y = uv_max.y;
        uv_max.y = t;
    }

    const bool rotated = Absf(rotation) > 0.01f;
    const bool clip = (fit == Fit::Cover && !tiled) || rotated;
    if (clip) {
        dl->PushClipRect(content.min, content.max, true);
    }

    if (rotated) {
        // 旋转用四边形绘制：绕中心旋转四个角（此时不支持圆角裁剪）
        const float radians = rotation * 3.14159265358979f / 180.0f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const ImVec2 center = destination.Center();
        auto rotate = [&](const ImVec2& p) {
            const float dx = p.x - center.x;
            const float dy = p.y - center.y;
            return ImVec2(center.x + dx * c - dy * s, center.y + dx * s + dy * c);
        };
        dl->AddImageQuad(texture, rotate(destination.min), rotate(ImVec2(destination.max.x, destination.min.y)),
                         rotate(destination.max), rotate(ImVec2(destination.min.x, destination.max.y)),
                         ImVec2(uv_min.x, uv_min.y), ImVec2(uv_max.x, uv_min.y), ImVec2(uv_max.x, uv_max.y),
                         ImVec2(uv_min.x, uv_max.y), Tint(tint));
    } else {
        const ImDrawFlags flags = Draw::CornerFlags(CornerTL(), CornerTR(), CornerBL(), CornerBR());
        const float rounding = Draw::MaxCorner(CornerTL(), CornerTR(), CornerBL(), CornerBR()) * DrawScale();
        dl->AddImageRounded(texture, destination.min, destination.max, uv_min, uv_max, Tint(tint), rounding, flags);
    }

    if (clip) {
        dl->PopClipRect();
    }
}

} // namespace gui_dev::cv
