#include "component_view/pages/ControlPage.h"

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/InputField.h"
#include "component_view/components/Label.h"
#include "component_view/components/VirtualKeyboard.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void InputPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* column = helpers::Column(host, 16.0f);
    column->SetSize(760.0f, 200.0f);
    column->align_x = Align::Start;

    name_ = column->Emplace<InputField>("游戏名称", "Super Mario World");
    name_->SetName("input_name");
    name_->SetSize(600.0f, 68.0f);
    name_->SetPlaceholder("请输入名称");
    name_->hint = "A 打开虚拟键盘";
    name_->on_edit_requested = [this](InputField& field) { OpenKeyboard(&field); };

    password_ = column->Emplace<InputField>("金手指密钥", "");
    password_->SetName("input_password");
    password_->SetSize(600.0f, 68.0f);
    password_->SetPassword(true);
    password_->SetPlaceholder("（密码）");
    password_->hint = "A 编辑 · X 清空 · Y 退格";
    password_->on_edit_requested = [this](InputField& field) { OpenKeyboard(&field); };

    readonly_ = column->Emplace<InputField>("只读字段", "sdmc:/switch/GUI_DEV/");
    readonly_->SetName("input_readonly");
    readonly_->SetSize(600.0f, 68.0f);
    readonly_->SetReadOnly(true);
    readonly_->SetPrefix("路径 ");
    readonly_->SetSuffix(" (只读)");

    status_ = helpers::Caption(host, "A 编辑（走自绘键盘，不调用系统键盘）· X 清空 · Y 退格", Theme::kFontSmall,
                               Theme::kTeal);
}

void InputPage::OpenKeyboard(InputField* field) {
    if (field == nullptr || keyboard_ == nullptr || overlay_ == nullptr) {
        return;
    }
    target_ = field;
    keyboard_->SetPrompt(field->label.empty() ? "输入" : field->label);
    keyboard_->max_length = field->max_length;
    keyboard_->password = field->password;
    keyboard_->SetInitial(field->text);
    overlay_->visible = true;
    overlay_->background = Theme::kScrim;
    Global::SetFocus(keyboard_);
}

void InputPage::BuildOverlay(Widget* overlay) {
    overlay_ = dynamic_cast<Box*>(overlay);
    if (overlay_ == nullptr) {
        return;
    }
    overlay_->background = 0;
    overlay_->visible = false;

    keyboard_ = overlay_->Emplace<VirtualKeyboard>();
    keyboard_->SetName("overlay_keyboard");
    keyboard_->SetSize(900.0f, 420.0f);
    keyboard_->SetPosition(190.0f, 120.0f);
    keyboard_->on_accept = [this](const std::string& value) {
        if (target_ != nullptr) {
            target_->SetText(value, true);
            target_->editing = false;
        }
        if (overlay_ != nullptr) {
            overlay_->visible = false;
        }
        Global::modal = nullptr;
        if (target_ != nullptr) {
            Global::SetFocus(target_);
        }
    };
    keyboard_->on_cancel = [this]() {
        if (target_ != nullptr) {
            target_->editing = false;
            Global::SetFocus(target_);
        }
        if (overlay_ != nullptr) {
            overlay_->visible = false;
        }
        Global::modal = nullptr;
    };
    keyboard_->on_changed = [this](const std::string& value) {
        if (status_ != nullptr) {
            status_->text = "编辑中：" + value;
        }
    };
}

void InputPage::OnUpdate(float dt) {
    (void)dt;
    if (overlay_ != nullptr && overlay_->visible) {
        // 键盘弹出时锁住焦点（Focus Trap）
        Global::modal = keyboard_;
    }
    if (status_ != nullptr && (overlay_ == nullptr || !overlay_->visible)) {
        status_->text = "name = " + (name_ != nullptr ? name_->text : "") + " · password 长度 " +
                        std::to_string(password_ != nullptr ? password_->CharCount() : 0) +
                        " · A 编辑（自绘键盘）";
    }
}

std::vector<std::pair<Icons::Button, std::string>> InputPage::Navigation() const {
    return {{Icons::Button::Up, "切换输入框"},
            {Icons::Button::A, "编辑（自绘键盘）"},
            {Icons::Button::X, "清空"},
            {Icons::Button::Y, "退格"},
            {Icons::Button::B, "返回标签"}};
}

void InputPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Field", {
                              Row("Text", name_ != nullptr ? name_->text : ""),
                              Row("Placeholder", "请输入名称"),
                              Row("Max Length", "32"),
                              Row("Password", "第二个字段开启", true),
                              Row("Read Only", "第三个字段开启", true),
                            });
    PushSection(out, "Editing", {
                                Row("Cursor", name_ != nullptr ? std::to_string(name_->cursor) : "0"),
                                Row("Selection", "全选后输入/删除会替换"),
                                Row("Prefix / Suffix", "路径  /  (只读)"),
                                Row("Editing", (overlay_ != nullptr && overlay_->visible) ? "是（键盘已弹出）" : "否", true),
                            });
    PushSection(out, "Virtual Keyboard", {
                                           Row("系统键盘", "绝不调用", true),
                                           Row("A", "打开自绘虚拟键盘"),
                                           Row("B", "取消编辑"),
                                       });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "切换输入框"),
                                     Row("A", "打开键盘"),
                                     Row("X / Y", "清空 / 退格"),
                                     Row("B", "返回标签列"),
                                 });
}

} // namespace gui_dev::cv
