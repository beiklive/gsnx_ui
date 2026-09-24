// ImageButton：图片按钮。模拟器 UI 里最常用的控件（游戏封面、图标格）。
//
// 五种状态贴图（缺省时用色调叠加同一张图）：Normal / Focused / Pressed / Selected / Disabled
// 额外支持：角标 Badge、标题 Caption、焦点边框动画、焦点缩放、按压缩放、无贴图时的占位图标。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class ImageButton : public Widget {
public:
    ImageButton();
    explicit ImageButton(ImTextureRef texture_ref, const ImVec2& native_size = ImVec2(0.0f, 0.0f));

    ImTextureRef image_normal;
    ImTextureRef image_focus;
    ImTextureRef image_pressed;
    ImTextureRef image_selected;
    ImTextureRef image_disabled;
    ImVec2 native_size{0.0f, 0.0f};
    ImVec2 uv0{0.0f, 0.0f};
    ImVec2 uv1{1.0f, 1.0f};

    enum class Fit { Stretch, Contain, Cover };
    Fit fit = Fit::Cover;

    // 无贴图时的占位
    ImU32 placeholder_bg = Theme::kBgWidget;
    std::string placeholder_icon; // Material 图标字形

    // 状态色调
    ImU32 tint_normal = IM_COL32(255, 255, 255, 255);
    ImU32 tint_hover = IM_COL32(255, 255, 255, 255);
    ImU32 tint_pressed = IM_COL32(210, 210, 210, 255);
    ImU32 tint_selected = IM_COL32(255, 255, 255, 255);

    // 标题与角标
    std::string caption;
    float caption_size = 0.0f;
    bool caption_inside = true;
    std::string badge;
    ImU32 badge_bg = Theme::kError;
    ImU32 badge_fg = Theme::kTextBright;

    // 动画
    float focus_scale_amount = 1.06f;
    float press_scale_amount = 0.96f;
    ImU32 focus_border_color = Theme::kAccent;
    float transition_speed = 14.0f;

    ImageButton& SetTexture(ImTextureRef texture_ref, const ImVec2& size);
    ImageButton& SetUV(const ImVec2& min_uv, const ImVec2& max_uv);
    ImageButton& SetBadge(std::string value);
    ImageButton& SetCaption(std::string value);
    ImageButton& SetFocusImage(ImTextureRef texture_ref);
    ImageButton& SetPressedImage(ImTextureRef texture_ref);
    ImageButton& SetSelectedImage(ImTextureRef texture_ref);
    ImageButton& SetDisabledImage(ImTextureRef texture_ref);
    ImageButton& SetPlaceholder(ImU32 background, std::string icon);
    ImageButton& SetFit(Fit value);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;

private:
    ImTextureRef ResolveTexture() const;
    ImU32 ResolveTint() const;
    float hover_mix_ = 0.0f;
    float press_mix_ = 0.0f;
};

} // namespace gui_dev::cv
