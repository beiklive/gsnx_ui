#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Dialog.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void DialogPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 18.0f);
    row->SetSize(0.0f, 60.0f);

    struct Scenario {
        const char* label;
        int buttons;
        bool vertical;
    };
    const Scenario scenarios[] = {
        {"确认对话框（取消 / 确定）", 2, false},
        {"三按钮（否 / 稍后 / 是）", 3, false},
        {"竖排按钮（警告）", 2, true},
    };

    for (const Scenario& scenario : scenarios) {
        Button* button = row->Emplace<Button>(scenario.label);
        button->SetName(std::string("dialog_open:") + scenario.label);
        button->Secondary().FitContent(18.0f, 52.0f);
        const int buttons = scenario.buttons;
        const bool vertical = scenario.vertical;
        button->on_click = [this, buttons, vertical](Widget&) {
            if (dialog_ == nullptr || overlay_ == nullptr) {
                return;
            }
            if (buttons == 3) {
                dialog_->SetButtons({"否", "稍后", "是"}, 2, 0);
            } else if (vertical) {
                dialog_->SetButtons({"取消", "确定"}, 1, 0);
            } else {
                dialog_->SetButtons({"取消", "确定"}, 1, 0);
            }
            dialog_->vertical_buttons = vertical;
            dialog_->SetTitle(vertical ? "警告" : "保存状态");
            dialog_->SetMessage(vertical ? "当前游戏尚未保存，确认要退出吗？"
                                         : "是否把当前游戏状态保存到槽位 1？");
            dialog_->SetIcon(Icons::Glyph(vertical ? Icons::Material::HelpOutline : Icons::Material::Save));
            overlay_->visible = true;
            overlay_->background = Theme::kScrim;
            ++open_count_;
            dialog_->Open();
        };
    }

    Box* info = helpers::Column(host, 8.0f);
    info->SetSize(0.0f, 90.0f);
    info->SetPadding(EdgeInsets::All(16.0f));
    info->SetBackground(Theme::kBgEditor);
    info->SetBorder(1.0f, Theme::kBorder);
    info->SetRadius(Theme::kRadiusSmall);
    info->align_x = Align::Start;
    info->Emplace<Label>("Dialog 关键点", Theme::kFontHeader, Theme::kTextBright);
    info->Emplace<Label>("· 模态 + 遮罩：打开时 Global::modal 指向自己，焦点被锁在对话框里", Theme::kFontSmall,
                         Theme::kTextPrimary);
    info->Emplace<Label>("· A 触发当前按钮 · B 等价于点「取消」· ← → 切换按钮", Theme::kFontSmall, Theme::kTextPrimary);
    status_ = info->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void DialogPage::BuildOverlay(Widget* overlay) {
    overlay_ = dynamic_cast<Box*>(overlay);
    if (overlay_ == nullptr) {
        return;
    }
    overlay_->background = 0;
    overlay_->visible = false;

    dialog_ = overlay_->Emplace<Dialog>("dialog");
    dialog_->SetName("overlay_dialog");
    dialog_->SetSize(520.0f, 260.0f);
    dialog_->anchor = ImVec2(0.5f, 0.5f);
    dialog_->pivot = ImVec2(0.5f, 0.5f);
    dialog_->on_result = [this](Dialog&, int result) {
        last_result_ = result < 0 ? "取消（-1）" : ("按钮 #" + std::to_string(result));
        if (overlay_ != nullptr) {
            overlay_->visible = false;
        }
        Global::modal = nullptr;
        Global::SetFocus(host() != nullptr ? host()->FirstFocusable() : nullptr);
    };
}

bool DialogPage::OnAction(InputAction action) {
    if (action == InputAction::Cancel && dialog_ != nullptr && dialog_->IsOpen()) {
        dialog_->Close(dialog_->cancel_button);
        return true;
    }
    return false;
}

void DialogPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr) {
        return;
    }
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "打开 %d 次 · 上次结果：%s · 焦点: %s", open_count_, last_result_.c_str(),
                  Global::modal != nullptr ? "对话框内（Focus Trap）" : "页面");
    status_->text = buffer;
}

std::vector<std::pair<Icons::Button, std::string>> DialogPage::Navigation() const {
    return {{Icons::Button::Left, "切换按钮"},
            {Icons::Button::A, "执行当前按钮"},
            {Icons::Button::B, "取消"},
            {Icons::Button::Down, "切换到打开按钮"}};
}

void DialogPage::FillProperties(std::vector<PropSection>& out) const {
    const bool open = dialog_ != nullptr && dialog_->IsOpen();
    PushSection(out, "Dialog", {
                               Row("Open", open ? "是" : "否", open),
                               Row("Modal", "是（Global::modal）"),
                               Row("Overlay", "kScrim 遮罩"),
                               Row("Focus Trap", open ? "生效中" : "未生效", open),
                               Row("Buttons", dialog_ != nullptr ? std::to_string(dialog_->buttons.size()) : "0"),
                               Row("Vertical", dialog_ != nullptr && dialog_->vertical_buttons ? "是" : "否"),
                           });
    PushSection(out, "State", {
                               Row("Focused Button", dialog_ != nullptr ? std::to_string(dialog_->FocusedButton() + 1) : "-",
                                   open),
                               Row("Last Result", last_result_, true),
                               Row("Open Count", std::to_string(open_count_)),
                           });
    PushSection(out, "Animation", {
                                   Row("Enter", "scale 0.9 → 1.0 + 上移 18px", true),
                                   Row("Exit", "反向播放（播完才隐藏）"),
                                   Row("Cancel", "B = cancel_button"),
                               });
    PushSection(out, "Navigation", {
                                     Row("← →", "切换按钮"),
                                     Row("A", "执行当前按钮"),
                                     Row("B", "取消并关闭"),
                                 });
}

} // namespace gui_dev::cv
