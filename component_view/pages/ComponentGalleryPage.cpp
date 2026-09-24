#include "component_view/pages/ComponentGalleryPage.h"

#include <cmath>
#include <cstdio>
#include <memory>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Image.h"
#include "component_view/components/Label.h"
#include "ui/Icons.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {
namespace {

constexpr float kColumnWidth = 600.0f;
constexpr float kColumnGap = 32.0f;
constexpr float kRightColumnX = kColumnWidth + kColumnGap;

} // namespace

// ---------------------------------------------------------------- 构建 ----

Box* ComponentGalleryPage::Section(const char* title, const ImVec2& position, const ImVec2& size) {
    Box* section = Root().Emplace<Box>(std::string("section:") + title);
    section->Surface();
    section->SetPosition(position.x, position.y);
    section->SetSize(size.x, size.y);
    section->layout = LayoutMode::Vertical;
    section->gap = ImVec2(0.0f, 10.0f);
    section->padding = EdgeInsets::All(16.0f);
    section->align_x = Align::Stretch;
    section->AddLabel(title, Theme::kFontHeader, Theme::kTextBright);
    return section;
}

Box* ComponentGalleryPage::MiniBox(Box* parent, const char* caption, float radius, bool accent_border,
                                   bool soft_shadow) {
    Box* mini = parent->Emplace<Box>(std::string("mini:") + caption);
    mini->SetSize(176.0f, 116.0f);
    mini->SetRadius(radius);
    mini->SetBackground(Theme::kBgWidget);
    mini->SetBorder(accent_border ? 2.0f : 1.0f, accent_border ? Theme::kAccent : Theme::kBorder);
    if (soft_shadow) {
        mini->shadow = ShadowStyle::Soft(24.0f);
        mini->shadow.offset = ImVec2(0.0f, 6.0f);
        mini->shadow.color = Theme::kShadow;
    }
    mini->layout = LayoutMode::Vertical;
    mini->align_x = Align::Center;
    mini->align_y = Align::Center;
    mini->padding = EdgeInsets::All(8.0f);
    mini->AddLabel(caption, Theme::kFontSmall, Theme::kTextPrimary)->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    return mini;
}

void ComponentGalleryPage::OnBuild() {
    flow_texture_ = TextureRef(ui().GetBackend(), "img/border_gradient.png");

    // ---- 页头 --------------------------------------------------------------
    Label* title = Root().Emplace<Label>("component_view · 组件库", Theme::kFontTitle, Theme::kTextBright);
    title->SetPosition(0.0f, 0.0f);

    Label* subtitle = Root().Emplace<Label>(
        "720p 设计空间 · VSCode Dark+ 配色 · Widget 父类 + Box / Label / Button / Image", Theme::kFontSmall,
        Theme::kTextMuted);
    subtitle->SetPosition(0.0f, 46.0f);

    // ---- Box ---------------------------------------------------------------
    Box* box_section = Section("Box · 容器", ImVec2(0.0f, 88.0f), ImVec2(kColumnWidth, 210.0f));
    Box* box_row = box_section->Emplace<Box>("row");
    box_row->layout = LayoutMode::Horizontal;
    box_row->gap = ImVec2(14.0f, 0.0f);
    box_row->align_y = Align::Center;
    box_row->size.y = 116.0f;
    MiniBox(box_row, "radius 0", 0.0f, false, false);
    MiniBox(box_row, "radius 8", Theme::kRadius, false, true);
    MiniBox(box_row, "radius 20", 20.0f, true, true);

    // ---- Label -------------------------------------------------------------
    Box* label_section = Section("Label · 文本", ImVec2(kRightColumnX, 88.0f), ImVec2(kColumnWidth, 210.0f));
    Label* label = label_section->AddLabel("正文 22  ·  kTextPrimary", Theme::kFontBody, Theme::kTextPrimary);
    (void)label;
    Box* label_row = label_section->Emplace<Box>("label_row");
    label_row->layout = LayoutMode::Horizontal;
    label_row->gap = ImVec2(18.0f, 0.0f);
    label_row->align_y = Align::Center;
    label_row->size.y = 40.0f;
    label_row->Emplace<Label>("标题 34", Theme::kFontTitle, Theme::kTextBright);
    label_row->Emplace<Label>("小节 26", Theme::kFontHeader, Theme::kTextPrimary);
    label_row->Emplace<Label>("注释 16", Theme::kFontSmall, Theme::kTextMuted);
    label_row->Emplace<Label>("强调", Theme::kFontBody, Theme::kAccent);

    Label* wrapped = label_section->Emplace<Label>(
        "自动换行：single_line = false 时按 wrap_width 折行，高度由 MeasureContent 返回，容器不需要手动算。",
        Theme::kFontSmall, Theme::kTextMuted);
    wrapped->wrap_width = 540.0f;
    wrapped->single_line = false;
    wrapped->SetSize(540.0f, 0.0f);

    // ---- Button ------------------------------------------------------------
    Box* button_section = Section("Button · 按钮", ImVec2(0.0f, 308.0f), ImVec2(kColumnWidth, 164.0f));
    Box* button_row = button_section->Emplace<Box>("button_row");
    button_row->layout = LayoutMode::Horizontal;
    button_row->gap = ImVec2(12.0f, 0.0f);
    button_row->align_y = Align::Center;
    button_row->size.y = 44.0f;

    auto make_button = [this, button_row](const char* name, const char* text, int variant) {
        Button* button = button_row->Emplace<Button>(text);
        button->SetName(name);
        button->FitContent(16.0f, 44.0f);
        if (variant == 1) {
            button->Secondary();
        } else if (variant == 2) {
            button->Ghost();
        } else if (variant == 3) {
            button->Danger();
        }
        button->on_click = [this, text](Widget&) {
            ++click_count_;
            last_action_ = text;
        };
        return button;
    };
    make_button("btn_confirm", "确认", 0)->SetIcon(Icons::Glyph(Icons::Button::A));
    make_button("btn_cancel", "取消", 1);
    make_button("btn_more", "更多", 2);
    make_button("btn_danger", "删除", 3);
    make_button("btn_disabled", "禁用", 1)->SetEnabled(false);

    status_label_ = button_section->AddLabel("点击按钮，或方向键 + A（焦点环画在按钮外侧）", Theme::kFontSmall,
                                             Theme::kTextMuted);

    // ---- Image -------------------------------------------------------------
    Box* image_section = Section("Image · 图片", ImVec2(kRightColumnX, 308.0f), ImVec2(kColumnWidth, 164.0f));
    Box* image_row = image_section->Emplace<Box>("image_row");
    image_row->layout = LayoutMode::Horizontal;
    image_row->gap = ImVec2(16.0f, 0.0f);
    image_row->align_y = Align::Center;
    image_row->size.y = 84.0f;

    if (flow_texture_.Valid()) {
        const ImVec2 native = flow_texture_.Size();

        // 1) 原比例长条
        Image* bar = image_row->Emplace<Image>(flow_texture_.ImGuiRef(), native);
        bar->SetSize(232.0f, 24.0f);
        bar->SetRadius(6.0f);
        bar->fit = Image::Fit::Stretch;

        // 2) cover：裁切填满
        Image* cover = image_row->Emplace<Image>(flow_texture_.ImGuiRef(), native);
        cover->SetSize(120.0f, 84.0f);
        cover->SetRadius(Theme::kRadius);
        cover->fit = Image::Fit::Cover;

        // 3) 圆角 + 色调
        Image* tinted = image_row->Emplace<Image>(flow_texture_.ImGuiRef(), native);
        tinted->SetSize(120.0f, 84.0f);
        tinted->SetRadius(20.0f, 4.0f, 20.0f, 4.0f);
        tinted->fit = Image::Fit::Cover;
        tinted->tint = Theme::Alpha(Theme::kTeal, 0.85f);
    } else {
        image_section->AddLabel("img/border_gradient.png 加载失败", Theme::kFontSmall, Theme::kError);
    }

    // ---- VSCode 配色 -------------------------------------------------------
    Box* color_section = Section("VSCode Dark+ 配色（底色 #1E1E1E，不用纯黑）", ImVec2(0.0f, 482.0f),
                                 ImVec2(2.0f * kColumnWidth + kColumnGap, 144.0f));
    struct Swatch {
        const char* hex;
        ImU32 color;
    };
    const Swatch swatches[] = {
        {"#1E1E1E", Theme::kBgEditor},   {"#252526", Theme::kBgSideBar}, {"#333333", Theme::kBgActivity},
        {"#2D2D30", Theme::kBgWidget},   {"#3C3C3C", Theme::kBorder},    {"#D4D4D4", Theme::kTextPrimary},
        {"#858585", Theme::kTextMuted},  {"#007ACC", Theme::kAccent},    {"#0E639C", Theme::kButton},
        {"#F14C4C", Theme::kError},      {"#CCA700", Theme::kWarning},   {"#4EC9B0", Theme::kTeal},
    };
    Box* swatch_row = color_section->Emplace<Box>("swatch_row");
    swatch_row->layout = LayoutMode::Horizontal;
    swatch_row->gap = ImVec2(6.0f, 0.0f);
    swatch_row->align_y = Align::Center;
    swatch_row->size.y = 60.0f;
    for (const Swatch& swatch : swatches) {
        Box* item = swatch_row->Emplace<Box>(std::string("swatch:") + swatch.hex);
        item->layout = LayoutMode::Vertical;
        item->gap = ImVec2(0.0f, 6.0f);
        item->align_x = Align::Center;
        item->SetSize(92.0f, 58.0f);

        Box* chip = item->Emplace<Box>(std::string("chip:") + swatch.hex);
        chip->SetSize(84.0f, 38.0f);
        chip->SetRadius(4.0f);
        chip->SetBackground(swatch.color);
        chip->SetBorder(1.0f, Theme::kBorder);

        item->AddLabel(swatch.hex, Theme::kFontTiny, Theme::kTextMuted)
            ->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    }

    // ---- HUD 提示 ----------------------------------------------------------
    AddHint(Icons::Button::A, "确定");
    AddHint(Icons::Button::B, "返回");
    AddHint(Icons::Button::Left, "移动焦点");
    AddHint(Icons::Button::L, "上一页");
    AddHint(Icons::Button::R, "下一页");
}

void ComponentGalleryPage::OnUpdate(float dt) {
    elapsed_ += dt;
    if (status_label_ == nullptr) {
        return;
    }
    // 顺便演示「运行时改父类属性」：文案 + 呼吸透明度。
    if (!last_action_.empty()) {
        char buffer[160];
        const char* focus_name = (Global::focused != nullptr && !Global::focused->name.empty())
                                     ? Global::focused->name.c_str()
                                     : "(无)";
        std::snprintf(buffer, sizeof(buffer), "已触发「%s」共 %d 次 · 当前焦点：%s", last_action_.c_str(),
                      click_count_, focus_name);
        status_label_->text = buffer;
        status_label_->color = Theme::kTextPrimary;
    }
    status_label_->opacity = 0.8f + 0.2f * (0.5f + 0.5f * std::sin(elapsed_ * 2.0f));
}

} // namespace gui_dev::cv
