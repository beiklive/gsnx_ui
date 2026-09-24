#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void ButtonPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    // 第一行：五种状态
    Box* states = helpers::Row(host, 14.0f);
    states->SetSize(0.0f, 52.0f);

    primary_ = states->Emplace<Button>("NORMAL");
    primary_->SetName("btn_normal");
    primary_->FitContent(20.0f, 48.0f);
    primary_->on_click = [this](Widget&) { ++confirm_count_; };
    primary_->on_aux = [this](Widget&) { ++aux_count_; };
    primary_->on_aux2 = [this](Widget&) { ++aux_count_; };

    Button* focused_demo = states->Emplace<Button>("FOCUSED");
    focused_demo->SetName("btn_focused");
    focused_demo->Secondary().FitContent(20.0f, 48.0f);
    focused_demo->focus_scale = 1.08f;
    focused_demo->focus_frame = true;

    Button* pressed = states->Emplace<Button>("PRESSED");
    pressed->SetName("btn_pressed");
    pressed->Secondary().FitContent(20.0f, 48.0f);
    pressed->press_scale = 0.9f;      // 按住 A 时明显缩小
    pressed->press_translate = 4.0f;

    selected_ = states->Emplace<Button>("SELECTED");
    selected_->SetName("btn_selected");
    selected_->Secondary().FitContent(20.0f, 48.0f);
    selected_->selected = true;
    selected_->on_click = [this](Widget& widget) {
        widget.selected = !widget.selected;
        ++confirm_count_;
    };

    disabled_ = states->Emplace<Button>("DISABLED");
    disabled_->SetName("btn_disabled");
    disabled_->Secondary().FitContent(20.0f, 48.0f);
    disabled_->SetEnabled(false);

    // 第二行：图标 + 文本（键盘风格）
    Box* icons = helpers::Row(host, 14.0f);
    icons->SetSize(0.0f, 52.0f);

    struct Item {
        const char* text;
        Icons::Button glyph;
    };
    const Item items[] = {
        {"确定", Icons::Button::A}, {"返回", Icons::Button::B}, {"辅助", Icons::Button::X},
        {"辅助", Icons::Button::Y}, {"页左", Icons::Button::L}, {"页右", Icons::Button::R},
        {"ZL", Icons::Button::ZL},  {"ZR", Icons::Button::ZR}, {"+", Icons::Button::Plus},
        {"-", Icons::Button::Minus},
    };
    for (const Item& item : items) {
        Button* button = icons->Emplace<Button>(item.text);
        button->Ghost().FitContent(12.0f, 44.0f);
        button->SetIcon(Icons::Glyph(item.glyph));
        button->icon_gap = 6.0f;
        button->on_click = [this](Widget&) { ++confirm_count_; };
    }

    // 第三行：提示与状态
    Box* footer = helpers::Row(host, 16.0f);
    footer->SetSize(0.0f, 40.0f);
    footer->Emplace<Label>("A 确认 · X / Y 辅助 · 方向键导航", Theme::kFontSmall, Theme::kTextMuted);
    status_ = footer->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void ButtonPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr) {
        return;
    }
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "A 触发 %d 次 · X/Y 触发 %d 次 · 焦点：%s", confirm_count_, aux_count_,
                  Global::focused != nullptr ? Global::focused->name.c_str() : "(无)");
    status_->text = buffer;
}

std::vector<std::pair<Icons::Button, std::string>> ButtonPage::Navigation() const {
    return {{Icons::Button::Left, "切换按钮"},
            {Icons::Button::A, "确认"},
            {Icons::Button::X, "辅助操作"},
            {Icons::Button::Y, "辅助操作"},
            {Icons::Button::B, "返回标签"}};
}

void ButtonPage::FillProperties(std::vector<PropSection>& out) const {
    const char* focus_name = Global::focused != nullptr ? Global::focused->name.c_str() : "(无)";
    PushSection(out, "Visual", {
                               Row("Background", "kButton / kBgWidget / 透明"),
                               Row("Radius", "4"),
                               Row("Border", "0 / 1 px"),
                               Row("Icon + Text", "A / B / X / Y / L / R / ZL / ZR / + / -"),
                               Row("Disabled", "kTextDisabled + 0.45 透明度"),
                           });
    PushSection(out, "State", {
                               Row("Normal", "kButton"),
                               Row("Focused", "焦点框 + scale 1.03", true),
                               Row("Pressed", "scale 0.90 / translate +4"),
                               Row("Selected", "SELECTED 已选中", true),
                               Row("Disabled", "DISABLED 不可聚焦"),
                           });
    PushSection(out, "Events", {
                                Row("on_click", std::to_string(confirm_count_) + " 次"),
                                Row("on_aux (X)", std::to_string(aux_count_) + " 次"),
                                Row("on_aux2 (Y)", std::to_string(aux_count_) + " 次"),
                            });
    PushSection(out, "Navigation", {
                                     Row("焦点位置", focus_name, true),
                                     Row("← →", "在按钮之间移动"),
                                     Row("A", "确认（按住不放看按压缩放）"),
                                     Row("B", "回到左侧标签列"),
                                 });
}

} // namespace gui_dev::cv
