#include <cmath>

#include "component_view/pages/ControlPage.h"

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"

namespace gui_dev::cv {

void BoxPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    // 第一行：圆角 / 边框 / 阴影
    Box* first = helpers::Row(host, 14.0f);
    first->SetSize(0.0f, 118.0f);

    Box* radius0 = helpers::Tile(first, "radius 0", 150.0f, 112.0f, 0.0f);
    (void)radius0;

    Box* radius8 = helpers::Tile(first, "radius 8", 150.0f, 112.0f, Theme::kRadius);
    (void)radius8;

    Box* radius20 = helpers::Tile(first, "radius 20", 150.0f, 112.0f, 20.0f);
    (void)radius20;

    // 四角独立 + 边框 + 阴影
    Box* mixed = helpers::Tile(first, "四角独立 + shadow", 190.0f, 112.0f, 0.0f, Theme::kBgWidget, Theme::kBorder);
    mixed->SetRadius(22.0f, 4.0f, 22.0f, 4.0f);
    mixed->SetBorder(2.0f, Theme::kOrange);
    mixed->shadow = ShadowStyle::Soft(26.0f);
    mixed->shadow.offset = ImVec2(0.0f, 8.0f);
    mixed->shadow.color = Theme::kShadow;

    // 第二行：溢出 + 可聚焦 Box
    Box* second = helpers::Row(host, 14.0f);
    second->SetSize(0.0f, 112.0f);

    overflow_hidden_ = helpers::Tile(second, "overflow: hidden", 220.0f, 108.0f, Theme::kRadiusSmall, Theme::kBgEditor,
                                     Theme::kBorderStrong);
    overflow_hidden_->overflow = Overflow::Hidden;
    overflow_hidden_->layout = LayoutMode::Vertical;
    overflow_hidden_->align_x = Align::Start;
    overflow_hidden_->padding = EdgeInsets::All(10.0f);
    for (int i = 0; i < 6; ++i) {
        overflow_hidden_->AddLabel("超出这一行的内容会被裁剪 " + std::to_string(i), Theme::kFontSmall,
                                   Theme::kTextMuted);
    }

    Box* overflow_scroll = helpers::Tile(second, "", 220.0f, 108.0f, Theme::kRadiusSmall, Theme::kBgEditor,
                                         Theme::kBorderStrong);
    overflow_scroll->overflow = Overflow::Scroll;
    overflow_scroll->layout = LayoutMode::Vertical;
    overflow_scroll->gap = ImVec2(0.0f, 6.0f);
    overflow_scroll->align_x = Align::Stretch;
    overflow_scroll->padding = EdgeInsets::All(10.0f);
    overflow_scroll->scroll_bar_auto_hide = false;
    for (int i = 0; i < 8; ++i) {
        overflow_scroll->AddLabel("overflow: scroll 第 " + std::to_string(i + 1) + " 行", Theme::kFontSmall,
                                  Theme::kTextPrimary);
    }

    focus_card_ = helpers::Tile(second, "焦点：缩放 + 位移 + 焦点框", 236.0f, 108.0f, Theme::kRadius,
                                Theme::kBgWidget, Theme::kBorderStrong);
    focus_card_->SetFocusable(true);
    focus_card_->focus_frame = true;
    focus_card_->focus_scale = 1.06f;
    focus_card_->focus_translate = ImVec2(0.0f, -4.0f);
    focus_card_->focus_frame_offset = 6.0f;

    helpers::Caption(host, "方向键移动焦点：这个 Box 自己就能聚焦（focus_scale / focus_translate / focus_frame）",
                     Theme::kFontSmall, Theme::kTextMuted);
}

void BoxPage::OnUpdate(float dt) {
    elapsed_ += dt;
    if (overflow_hidden_ != nullptr) {
        // 演示：运行时改 opacity（父类属性，子节点一起淡出）
        overflow_hidden_->opacity = 0.75f + 0.25f * (0.5f + 0.5f * std::sin(elapsed_ * 1.6f));
    }
}

void BoxPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Visual", {
                               Row("Background", "kBgWidget / kBgEditor"),
                               Row("Border", "1.0 / 2.0 px"),
                               Row("Radius", "0 / 8 / 20 / 四角独立"),
                               Row("Opacity", "0.75 ~ 1.00（动画中）"),
                               Row("Shadow", "blur 26 · offset (0,8)"),
                           });
    PushSection(out, "Layout", {
                               Row("Size", "150x112 / 190x112 / 236x108"),
                               Row("Padding", "10"),
                               Row("Gap", "14"),
                               Row("Align", "Horizontal · Center"),
                               Row("Overflow", "hidden / scroll"),
                           });
    PushSection(out, "State", {
                               Row("Normal", "●", true),
                               Row("Focused", "可聚焦 Box 支持", true),
                               Row("Pressed", "—"),
                               Row("Disabled", "enabled = false 时整体变淡"),
                           });
    PushSection(out, "Focus", {
                               Row("Focusable", "是"),
                               Row("Focus Frame", "是"),
                               Row("Focus Scale", "1.06"),
                               Row("Focus Translate", "(0, -4)"),
                           });
}

} // namespace gui_dev::cv
