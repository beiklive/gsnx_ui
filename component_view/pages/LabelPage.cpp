#include "component_view/pages/ControlPage.h"

#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"

namespace gui_dev::cv {

void LabelPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    // 字号与字重
    Box* sizes = helpers::Row(host, 26.0f);
    sizes->SetSize(0.0f, 54.0f);
    sizes->Emplace<Label>("标题 34", Theme::kFontTitle, Theme::kTextBright)->font_weight = 600;
    sizes->Emplace<Label>("小节 26", Theme::kFontHeader, Theme::kTextPrimary);
    sizes->Emplace<Label>("正文 22", Theme::kFontBody, Theme::kTextPrimary);
    sizes->Emplace<Label>("注释 16", Theme::kFontSmall, Theme::kTextMuted);
    sizes->Emplace<Label>("强调", Theme::kFontBody, Theme::kAccent);

    // 对齐
    Box* align_row = helpers::Row(host, 14.0f);
    align_row->SetSize(0.0f, 46.0f);
    Box* left = helpers::Tile(align_row, "", 160.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgEditor, Theme::kBorder);
    left->AddLabel("左对齐", Theme::kFontSmall, Theme::kTextPrimary);
    Box* center = helpers::Tile(align_row, "", 160.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgEditor, Theme::kBorder);
    center->AddLabel("居中", Theme::kFontSmall, Theme::kTextPrimary)
        ->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    Box* right = helpers::Tile(align_row, "", 160.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgEditor, Theme::kBorder);
    right->AddLabel("右对齐", Theme::kFontSmall, Theme::kTextPrimary)
        ->SetAlign(TextAlign::Right, VerticalAlign::Middle);

    // 省略号 / 跑马灯 / 描边
    Box* styles = helpers::Row(host, 14.0f);
    styles->SetSize(0.0f, 46.0f);
    Box* ellipsis_tile = helpers::Tile(styles, "", 250.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgEditor, Theme::kBorder);
    Label* ellipsis = ellipsis_tile->AddLabel("这一段文字会以省略号结尾显示…", Theme::kFontSmall, Theme::kTextPrimary);
    ellipsis->ellipsis = true;
    ellipsis->SetSize(226.0f, 0.0f);

    marquee_ = nullptr;
    Box* marquee_tile = helpers::Tile(styles, "", 300.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgEditor, Theme::kBorder);
    marquee_ = marquee_tile->AddLabel("跑马灯：这是一段很长的文案，超宽之后会自动横向滚动", Theme::kFontSmall,
                                      Theme::kTextPrimary);
    marquee_->SetMarquee(true, 46.0f);
    marquee_->SetSize(272.0f, 0.0f);

    Box* outline_tile = helpers::Tile(styles, "", 200.0f, 44.0f, Theme::kRadiusSmall, Theme::kBgWidget,
                                      Theme::kBorder);
    Label* outline = outline_tile->AddLabel("描边 + 阴影", Theme::kFontBody, Theme::kTextBright);
    outline->SetOutline(Theme::Alpha(IM_COL32(0, 0, 0, 255), 0.85f));
    outline->SetShadow(Theme::kShadowSoft);

    // 多行自动换行
    wrap_label_ = host->Emplace<Label>(
        "自动换行：single_line = false 时按 wrap_width 折行；中英文混排都能正确断行，高度由内容测量得出。",
        Theme::kFontSmall, Theme::kTextMuted);
    wrap_label_->SetWrap(640.0f);
    wrap_label_->SetSize(640.0f, 0.0f);

    // 焦点换文案 / 换色
    Box* focus_row = helpers::Row(host, 14.0f);
    focus_row->SetSize(0.0f, 46.0f);
    focus_label_ = focus_row->AddLabel("这是一个可聚焦的 Label（方向键选到我）", Theme::kFontBody, Theme::kTextPrimary);
    focus_label_->SetFocusable(true);
    focus_label_->focus_frame = true;
    focus_label_->focus_scale = 1.04f;
    focus_label_->focus_text = "聚焦了！文案和颜色都会变";
    focus_label_->focus_color = Theme::kAccent;
}

void LabelPage::OnUpdate(float dt) {
    elapsed_ += dt;
    (void)elapsed_;
}

void LabelPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Text", {
                              Row("单行", "✓"),
                              Row("多行 / 自动换行", "wrap_width = 640"),
                              Row("Ellipsis", "✓"),
                              Row("Marquee", "46 px/s"),
                          });
    PushSection(out, "Style", {
                               Row("Font Size", "34 / 26 / 22 / 16"),
                               Row("Font Weight", "400 / 600"),
                               Row("Color", "kTextPrimary · kTextMuted · kAccent"),
                               Row("Shadow", "kShadowSoft"),
                               Row("Outline", "1px 四向"),
                               Row("Letter / Line", "0 / 0"),
                           });
    PushSection(out, "Focus", {
                               Row("Focus Text", "“聚焦了！文案和颜色都会变”"),
                               Row("Focus Color", "kAccent", true),
                               Row("Focus Scale", "1.04", true),
                               Row("Disabled", "enabled = false → 变淡"),
                           });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "切换 Label"),
                                     Row("A", "确定（本例无回调）"),
                                     Row("B", "回到标签列"),
                                 });
}

} // namespace gui_dev::cv
