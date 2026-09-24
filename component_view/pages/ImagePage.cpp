#include "component_view/pages/ControlPage.h"

#include "component_view/components/Box.h"
#include "component_view/components/Image.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Texture.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {

namespace {
TextureRef g_flow_texture; // 页面级别的贴图句柄（页面存活期间只加载一次）
}

void ImagePage::Build(Widget* host, UiContext& ui) {
    if (!g_flow_texture.Valid()) {
        g_flow_texture = TextureRef(ui.GetBackend(), "img/border_gradient.png");
    }
    const ImVec2 native = g_flow_texture.Size();
    has_texture_ = g_flow_texture.Valid();

    (void)ui;
    Box* row = helpers::Row(host, 16.0f);
    row->SetSize(0.0f, 117.9f);

    if (has_texture_) {
        // Contain：完整显示
        Image* contain = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        contain->SetSize(148.2f, 87.4f);
        contain->SetRadius(Theme::kRadius);
        contain->fit = Image::Fit::Contain;
        contain->SetBackground(Theme::kBgEditor);
        contain->SetBorder(1.0f, Theme::kBorder);

        // Cover：裁切填满
        Image* cover = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        cover->SetSize(117.0f, 87.4f);
        cover->SetRadius(Theme::kRadius);
        cover->fit = Image::Fit::Cover;

        // Stretch：拉伸
        Image* stretch = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        stretch->SetSize(93.6f, 87.4f);
        stretch->SetRadius(Theme::kRadiusSmall);
        stretch->fit = Image::Fit::Stretch;

        // 圆形裁剪
        Image* circle = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        circle->SetSize(87.4f, 87.4f);
        circle->SetRadius(56.0f);
        circle->fit = Image::Fit::Cover;

        // UV 裁切 + Flip
        Image* cropped = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        cropped->SetSize(117.0f, 87.4f);
        cropped->SetRadius(Theme::kRadius);
        cropped->fit = Image::Fit::Stretch;
        cropped->SetUV(ImVec2(0.0f, 0.0f), ImVec2(0.5f, 1.0f));
        cropped->flip_x = true;

        // 旋转 + 半透明
        rotating_ = row->Emplace<Image>(g_flow_texture.ImGuiRef(), native);
        rotating_->SetSize(120.0f, 112.1f);
        rotating_->fit = Image::Fit::Cover;
        rotating_->tint = Theme::Alpha(Theme::kTeal, 0.85f);
    } else {
        helpers::Tile(row, "贴图加载失败", 156.0f, 87.4f, Theme::kRadius, Theme::kBgWidget, Theme::kError);
    }

    Box* footer = helpers::Row(host, 16.0f);
    footer->SetSize(0.0f, 40.0f);
    footer->Emplace<Label>("Contain · Cover · Stretch · 圆形裁剪 · UV 裁切 + FlipX · 旋转 + Tint", Theme::kFontSmall,
                           Theme::kTextMuted);
    footer->Emplace<Label>(has_texture_ ? "贴图：img/border_gradient.png (512x4)" : "无贴图",
                           Theme::kFontTiny, Theme::kTextMuted);
}

void ImagePage::OnUpdate(float dt) {
    angle_ += dt * 36.0f;
    if (angle_ > 360.0f) {
        angle_ -= 360.0f;
    }
    if (rotating_ != nullptr) {
        rotating_->rotation = angle_;
    }
}

void ImagePage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Fit", {
                             Row("Contain", "完整显示"),
                             Row("Cover", "裁切填满"),
                             Row("Stretch", "拉伸"),
                             Row("Center / Crop", "UV 裁切 0~0.5"),
                         });
    PushSection(out, "Visual", {
                               Row("Tint", "白 / kTeal 0.85"),
                               Row("Opacity", "1.00"),
                               Row("Flip X / Y", "启用 FlipX"),
                               Row("Rotation", "0 ~ 360 度（动画中）", true),
                               Row("圆角裁剪", "8 / 4 / 56(圆形)"),
                           });
    PushSection(out, "Animation", {
                                   Row("Frame Animation", "—（由外部纹理序列驱动）"),
                                   Row("Fade / Scale", "通过 tint / size 表达"),
                                   Row("Rotation", "36 度/秒", true),
                               });
    PushSection(out, "Focus", {
                               Row("Focusable", "否（Image 本身不可聚焦）"),
                               Row("Focus 替代", "用 ImageButton"),
                           });
}

} // namespace gui_dev::cv
