#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/components/TabBar.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void TabPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 24.0f);
    row->SetSize(0.0f, 196.0f);
    row->align_y = Align::Start;

    Box* column = helpers::Column(row, 12.0f);
    column->SetSize(520.0f, 196.0f);
    column->align_x = Align::Start;

    // 水平 Tab（图标 + 文本）
    horizontal_ = column->Emplace<TabBar>("tab_horizontal");
    horizontal_->SetSize(500.0f, 46.0f);
    horizontal_->orientation = TabBar::Orientation::Horizontal;
    horizontal_->tab_size = ImVec2(112.0f, 44.0f);
    horizontal_->immediate = true;
    horizontal_->AddTab("画面", Icons::Glyph(Icons::Material::ImagePlaceholder));
    horizontal_->AddTab("音频", Icons::Glyph(Icons::Material::Memory));
    horizontal_->AddTab("按键", Icons::Glyph(Icons::Material::Settings));
    horizontal_->AddTab("存档", Icons::Glyph(Icons::Material::Save));
    horizontal_->AddTab("关于", Icons::Glyph(Icons::Material::HelpOutline));
    horizontal_->AddTab("网络", Icons::Glyph(Icons::Material::Wifi));

    // 垂直 Tab（可滚动，演示自动滚动）
    vertical_ = column->Emplace<TabBar>("tab_vertical");
    vertical_->SetSize(230.0f, 138.0f);
    vertical_->orientation = TabBar::Orientation::Vertical;
    vertical_->tab_size = ImVec2(0.0f, 34.0f);
    vertical_->immediate = false;
    vertical_->AddTab("Vertical 01");
    vertical_->AddTab("Vertical 02");
    vertical_->AddTab("Vertical 03");
    vertical_->AddTab("Vertical 04");
    vertical_->AddTab("Vertical 05");
    vertical_->AddTab("Vertical 06");
    vertical_->AddTab("Vertical 07");
    vertical_->AddTab("Vertical 08");

    // 右侧说明
    Box* info = helpers::Column(row, 10.0f);
    info->SetSize(390.0f, 196.0f);
    info->SetPadding(EdgeInsets::All(16.0f));
    info->SetBackground(Theme::kBgEditor);
    info->SetBorder(1.0f, Theme::kBorder);
    info->SetRadius(Theme::kRadiusSmall);
    info->align_x = Align::Start;
    info->Emplace<Label>("Tab 关键点", Theme::kFontHeader, Theme::kTextBright);
    info->Emplace<Label>("· 一个 TabBar 只占一个焦点停靠点", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· 指示条在光标之间滑动（动画）", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· A 选中，immediate=true 时移动即切换", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· L / R 翻页，ZL / ZR 首尾跳转", Theme::kFontSmall, Theme::kTextPrimary);
    status_ = info->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void TabPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr || horizontal_ == nullptr || vertical_ == nullptr) {
        return;
    }
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "水平选中 #%d · 垂直选中 #%d · 焦点: %s", horizontal_->Index() + 1,
                  vertical_->Index() + 1,
                  Global::focused != nullptr ? Global::focused->name.c_str() : "(无)");
    status_->text = buffer;
}

std::vector<std::pair<Icons::Button, std::string>> TabPage::Navigation() const {
    return {{Icons::Button::Left, "切换水平 Tab"},
            {Icons::Button::A, "选中"},
            {Icons::Button::Up, "切换垂直 Tab"},
            {Icons::Button::L, "上一页"},
            {Icons::Button::R, "下一页"},
            {Icons::Button::ZL, "第一项"},
            {Icons::Button::ZR, "最后一项"}};
}

void TabPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Layout", {
                               Row("Orientation", "Horizontal / Vertical"),
                               Row("Tab Size", "112x44 / 撑满 x 34"),
                               Row("Gap", "4"),
                               Row("Scroll", "垂直 Tab 超出时自动滚动"),
                           });
    PushSection(out, "Visual", {
                               Row("Icon + Text", "✓"),
                               Row("Active 底色", "kSelection"),
                               Row("Cursor 底色", "kListRowFocus"),
                               Row("Indicator", "4px 强调色 + 滑动动画", true),
                           });
    PushSection(out, "State", {
                               Row("Horizontal #", horizontal_ != nullptr ? std::to_string(horizontal_->Index() + 1) : "-",
                                   true),
                               Row("Vertical #", vertical_ != nullptr ? std::to_string(vertical_->Index() + 1) : "-",
                                   true),
                               Row("Disabled", "AddTab(..., disabled = true)"),
                               Row("immediate", "水平开 / 垂直关"),
                           });
    PushSection(out, "Navigation", {
                                     Row("← →", "水平 Tab 内移动"),
                                     Row("↑ ↓", "垂直 Tab 内移动"),
                                     Row("A", "选中当前 Tab"),
                                     Row("L / R", "按页滚动"),
                                     Row("ZL / ZR", "跳到第一 / 最后一项"),
                                 });
}

} // namespace gui_dev::cv
