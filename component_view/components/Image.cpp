#include "component_view/components/Image.h"

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
    const bool clip = fit == Fit::Cover && !tiled;
    if (clip) {
        dl->PushClipRect(content.min, content.max, true);
    }

    const ImDrawFlags flags = Draw::CornerFlags(CornerTL(), CornerTR(), CornerBL(), CornerBR());
    const float rounding = Draw::MaxCorner(CornerTL(), CornerTR(), CornerBL(), CornerBR());
    dl->AddImageRounded(texture, destination.min, destination.max, uv_min, uv_max, Tint(tint), rounding, flags);

    if (clip) {
        dl->PopClipRect();
    }
}

} // namespace gui_dev::cv
