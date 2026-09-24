#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Checkbox.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void CheckboxPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 30.0f);
    row->SetSize(0.0f, 150.0f);
    row->align_y = Align::Start;

    Box* column = helpers::Column(row, 12.0f);
    column->SetSize(430.0f, 150.0f);
    column->align_x = Align::Start;

    master_ = column->Emplace<Checkbox>("全选：启用全部选项", false);
    master_->SetName("checkbox_master");
    master_->SetBackground(Theme::kBgEditor);
    master_->padding = EdgeInsets::Symmetric(14.0f, 8.0f);

    boxes_.clear();
    static const char* const kLabels[] = {"扫描子目录", "显示隐藏文件", "启动时自动加载金手指", "实验性：异步解压"};
    for (int i = 0; i < 4; ++i) {
        Checkbox* box = column->Emplace<Checkbox>(kLabels[i], i < 2);
        box->SetName("checkbox_" + std::to_string(i));
        box->box_size = 24.0f;
        boxes_.push_back(box);
    }
    boxes_[3]->SetEnabled(false);
    boxes_[3]->SetLabel(kLabels[3] + std::string("（已禁用）"));

    master_->on_changed = [this](Checkbox&, bool value) {
        for (std::size_t i = 0; i < boxes_.size(); ++i) {
            if (boxes_[i]->enabled) {
                boxes_[i]->SetChecked(value, false);
            }
        }
    };

    Box* info = helpers::Column(row, 10.0f);
    info->SetSize(400.0f, 150.0f);
    info->SetPadding(EdgeInsets::All(16.0f));
    info->SetBackground(Theme::kBgEditor);
    info->SetBorder(1.0f, Theme::kBorder);
    info->SetRadius(Theme::kRadiusSmall);
    info->align_x = Align::Start;
    info->Emplace<Label>("Checkbox 说明", Theme::kFontHeader, Theme::kTextBright);
    info->Emplace<Label>("· A 或 X 切换，不需要 Hover", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· 勾选用两段动画画出来", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· 聚焦时整体轻微右移 + 文字变亮", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· Disabled 项不可聚焦、不参与导航", Theme::kFontSmall, Theme::kTextPrimary);
    status_ = info->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void CheckboxPage::OnUpdate(float dt) {
    elapsed_ += dt;
    (void)elapsed_;
    if (status_ == nullptr) {
        return;
    }
    int checked = 0;
    for (const Checkbox* box : boxes_) {
        if (box->checked) {
            ++checked;
        }
    }
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "已勾选 %d/%d · 全选 %s", checked, static_cast<int>(boxes_.size()),
                  master_ != nullptr && master_->checked ? "开" : "关");
    status_->text = buffer;
}

void CheckboxPage::FillProperties(std::vector<PropSection>& out) const {
    int checked = 0;
    for (const Checkbox* box : boxes_) {
        if (box->checked) {
            ++checked;
        }
    }
    PushSection(out, "State", {
                               Row("Unchecked", "○"),
                               Row("Checked", std::to_string(checked) + " 项已勾选", true),
                               Row("Focused", Global::focused != nullptr ? Global::focused->name : "(无)", true),
                               Row("Disabled", "第 4 项（不可聚焦）"),
                           });
    PushSection(out, "Visual", {
                               Row("Indicator", "24x24 圆角方框"),
                               Row("Check Animation", "两段式勾（0.45 / 0.55）", true),
                               Row("Label", "框右侧 12px"),
                               Row("Border / Radius", "1.5 / 6"),
                           });
    PushSection(out, "Focus", {
                               Row("Focus Animation", "右移 6px + 文字变亮", true),
                               Row("Focus Scale", "1.02"),
                               Row("Focus Frame", "是"),
                           });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "在复选项之间移动"),
                                     Row("A / X", "切换勾选"),
                                     Row("B", "回到左侧标签列"),
                                 });
}

} // namespace gui_dev::cv
