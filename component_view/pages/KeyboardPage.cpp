#include "component_view/pages/ControlPage.h"

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/InputField.h"
#include "component_view/components/Label.h"
#include "component_view/components/VirtualKeyboard.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void KeyboardPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    // 键盘是内嵌在展示区里的（也可以放进弹层，见 INPUT / DIALOG 页）
    Box* column = helpers::Column(host, 10.0f);
    column->SetSize(920.0f, 272.0f);
    column->align_x = Align::Start;

    keyboard_ = column->Emplace<VirtualKeyboard>();
    keyboard_->SetName("keyboard_inline");
    keyboard_->SetSize(900.0f, 240.0f);
    keyboard_->key_height = 31.0f;
    keyboard_->key_gap = 5.0f;
    keyboard_->preview_height = 36.0f;
    keyboard_->padding = EdgeInsets::All(10.0f);
    keyboard_->max_length = 24;
    keyboard_->SetPrompt("键盘页 · 内嵌模式");
    keyboard_->SetInitial("Hello GUI_DEV");
    keyboard_->on_accept = [this](const std::string& value) { last_action_ = "确定：" + value; };
    keyboard_->on_cancel = [this]() {
        keyboard_->SetInitial("Hello GUI_DEV");
        last_action_ = "取消：已还原到初始值";
    };

    log_ = helpers::Caption(host, "", Theme::kFontSmall, Theme::kTeal);
}

void KeyboardPage::OnUpdate(float dt) {
    (void)dt;
    if (log_ == nullptr || keyboard_ == nullptr) {
        return;
    }
    if (!last_action_.empty()) {
        log_->text = last_action_ + " · 缓冲 = " + keyboard_->buffer;
    } else {
        log_->text = "缓冲 = " + keyboard_->buffer + " · 页 = " + keyboard_->PageName() +
                     (keyboard_->shift ? " · SHIFT" : "");
    }
}

bool KeyboardPage::OnAction(InputAction action) {
    (void)action;
    return false;
}

std::vector<std::pair<Icons::Button, std::string>> KeyboardPage::Navigation() const {
    return {{Icons::Button::Up, "移动焦点（行内错位也对）"},
            {Icons::Button::A, "输入当前键"},
            {Icons::Button::Left, "移动焦点 / 切换字符页"},
            {Icons::Button::L, "上一字符页"},
            {Icons::Button::R, "下一字符页"},
            {Icons::Button::ZL, "切换 Shift"},
            {Icons::Button::ZR, "循环字符集"},
            {Icons::Button::X, "退格"},
            {Icons::Button::Y, "空格"},
            {Icons::Button::Plus, "确定"},
            {Icons::Button::Minus, "退格"},
            {Icons::Button::B, "取消"}};
}

void KeyboardPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Layout", {
                               Row("QWERTY", "字母页（带错位行）", true),
                               Row("Symbols", "符号页"),
                               Row("Numbers", "数字页"),
                               Row("Columns", "10"),
                               Row("Key Size", "42px（可调）"),
                           });
    PushSection(out, "Editing", {
                                Row("Buffer", keyboard_ != nullptr ? keyboard_->buffer : ""),
                                Row("Caret / Selection",
                                    (keyboard_ != nullptr ? std::to_string(keyboard_->Caret()) : "0") +
                                        (keyboard_ != nullptr && keyboard_->HasSelection() ? " · 有选区" : "")),
                                Row("Shift", keyboard_ != nullptr && keyboard_->shift ? "开" : "关", true),
                                Row("Page", keyboard_ != nullptr ? keyboard_->PageName() : "-", true),
                            });
    PushSection(out, "Function Keys", {
                                       Row("SHIFT / SPACE", "大小写（ZL）· 空格（Y）"),
                                       Row("Left / Right", "光标移动（< > 键）"),
                                       Row("BKSP / DEL", "向前 / 向后删除"),
                                       Row("SEL / CLR", "全选（可被输入替换）· 清空"),
                                   });
    PushSection(out, "Navigation", {
                                     Row("方向键", "最近邻键位导航"),
                                     Row("A", "输入 · L/R 切页 · ZL/ZR Shift"),
                                     Row("+ / -", "确定 / 退格"),
                                     Row("B", "取消"),
                                 });
}

} // namespace gui_dev::cv
