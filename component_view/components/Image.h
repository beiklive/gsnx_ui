// Image：图片组件。纹理 + UV + 色调 + 圆角 + contain/cover 适配。
#pragma once

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Image : public Widget {
public:
    Image();
    explicit Image(ImTextureRef texture_ref, const ImVec2& native_size = ImVec2(0.0f, 0.0f));

    ImTextureRef texture;
    ImVec2 native_size{0.0f, 0.0f}; // 纹理原始像素尺寸（size 为 0 时按它自适应）
    ImVec2 uv0{0.0f, 0.0f};
    ImVec2 uv1{1.0f, 1.0f};
    ImU32 tint = IM_COL32_WHITE;
    enum class Fit { Stretch, Contain, Cover };
    Fit fit = Fit::Contain;
    bool tiled = false; // 尺寸大于原始尺寸时按平铺显示（UV 重复）

    void SetTexture(ImTextureRef texture_ref, const ImVec2& size);
    Image& SetUV(const ImVec2& min_uv, const ImVec2& max_uv);
    Image& Tinted(ImU32 color);
    Image& SetFit(Fit value);
    bool HasTexture() const { return texture.GetTexID() != ImTextureID_Invalid; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
};

} // namespace gui_dev::cv
