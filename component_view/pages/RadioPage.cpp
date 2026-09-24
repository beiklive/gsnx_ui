#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Checkbox.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void RadioPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 26.0f);
    row->SetSize(0.0f, 214.0f);
    row->align_y = Align::Start;

    // 垂直单选组
    Box* column = helpers::Column(row, 12.0f);
    column->SetSize(430.0f, 214.0f);
    column->align_x = Align::Start;

    group_ = column->Emplace<RadioGroup>("radio_vertical");
    group_->SetSize(420.0f, 190.0f);
    group_->SetBackground(Theme::kBgEditor);
    group_->SetBorder(1.0f, Theme::kBorderStrong);
    group_->SetRadius(Theme::kRadiusSmall);
    group_->row_height = 34.3f;
    group_->AddOption("画面：整数缩放", "最近邻");
    group_->AddOption("画面：线性过滤", "更平滑");
    group_->AddOption("画面：CRT 扫描线", "实验性");
    group_->AddOption("画面：保持宽高比", "推荐");
    group_->AddOption("（已禁用的选项）", "", true);
    group_->SetValue(3, false);
    group_->SetCursor(3);

    // 水平单选组
    Box* right = helpers::Column(row, 12.0f);
    right->SetSize(420.0f, 214.0f);
    right->align_x = Align::Start;

    horizontal_ = right->Emplace<RadioGroup>("radio_horizontal");
    horizontal_->SetSize(410.0f, 52.1f);
    horizontal_->vertical = false;
    horizontal_->row_height = 39.0f;
    horizontal_->SetBackground(Theme::kBgEditor);
    horizontal_->SetBorder(1.0f, Theme::kBorderStrong);
    horizontal_->SetRadius(Theme::kRadiusSmall);
    horizontal_->AddOption("自动");
    horizontal_->AddOption("30 FPS");
    horizontal_->AddOption("60 FPS");
    horizontal_->AddOption("120 FPS");
    horizontal_->SetValue(2, false);
    horizontal_->SetCursor(2);

    Box* info = right->Emplace<Box>("radio_info");
    info->SetSize(319.8f, 117.0f);
    info->SetPadding(EdgeInsets::All(10.9f));
    info->SetBackground(Theme::kBgEditor);
    info->SetBorder(1.0f, Theme::kBorder);
    info->SetRadius(Theme::kRadiusSmall);
    info->layout = LayoutMode::Vertical;
    info->gap = ImVec2(0.0f, 4.7f);
    info->align_x = Align::Start;
    info->AddLabel("RadioGroup = 一个焦点停靠点", Theme::kFontSmall, Theme::kTextBright);
    info->AddLabel("↑ ↓ 移动高亮（光标），A 选定", Theme::kFontSmall, Theme::kTextPrimary);
    info->AddLabel("圆点用两层圆做选中动画", Theme::kFontSmall, Theme::kTextPrimary);
    status_ = info->AddLabel("", Theme::kFontTiny, Theme::kTeal);
}

void RadioPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr || group_ == nullptr || horizontal_ == nullptr) {
        return;
    }
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "画面 #%d · 帧率 #%d · 光标 %d/%d", group_->Value() + 1,
                  horizontal_->Value() + 1, group_->Cursor() + 1, group_->OptionCount());
    status_->text = buffer;
}

void RadioPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Group", {
                              Row("Orientation", "Vertical / Horizontal"),
                              Row("Options", group_ != nullptr ? std::to_string(group_->OptionCount()) : "0"),
                              Row("Loop", "开"),
                              Row("Disabled Option", "第 5 项"),
                          });
    PushSection(out, "State", {
                               Row("Selected (画面)",
                                   group_ != nullptr ? "第 " + std::to_string(group_->Value() + 1) + " 项" : "-", true),
                               Row("Highlight (光标)",
                                   group_ != nullptr ? "第 " + std::to_string(group_->Cursor() + 1) + " 项" : "-", true),
                               Row("Selected (帧率)",
                                   horizontal_ != nullptr ? "第 " + std::to_string(horizontal_->Value() + 1) + " 项" : "-",
                                   true),
                               Row("Focus Frame", "整个 Group 外的焦点框"),
                           });
    PushSection(out, "Visual", {
                               Row("Indicator", "24px 圆 + 内圆"),
                               Row("Selection Animation", "内圆从 0 长到 50%", true),
                               Row("Highlight", "行底色 kListRowFocus"),
                               Row("Label", "圆右侧 12px"),
                           });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "垂直组：移动高亮"),
                                     Row("← →", "水平组：移动高亮"),
                                     Row("A", "选定当前高亮项"),
                                     Row("B", "回到左侧标签列"),
                                 });
}

} // namespace gui_dev::cv
