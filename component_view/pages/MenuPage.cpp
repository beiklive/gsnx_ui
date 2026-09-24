#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/components/Menu.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void MenuPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 22.0f);
    row->SetSize(0.0f, 236.0f);
    row->align_y = Align::Start;

    menu_ = row->Emplace<Menu>("menu_main");
    menu_->SetSize(420.0f, 232.0f);
    menu_->SetBackground(Theme::kBgEditor);
    menu_->SetBorder(1.0f, Theme::kBorderStrong);
    menu_->SetRadius(Theme::kRadiusSmall);
    menu_->SetTitle("游戏内菜单");

    menu_->AddEntry("返回游戏", Icons::Glyph(Icons::Material::Play), "B");
    menu_->AddEntry("保存状态", Icons::Glyph(Icons::Material::Save), "ZL");
    menu_->AddEntry("读取状态", Icons::Glyph(Icons::Material::Storage), "ZR");
    menu_->AddSeparator();
    menu_->AddEntry("独立设置", Icons::Glyph(Icons::Material::Settings));
    menu_->AddEntry("全局设置", Icons::Glyph(Icons::Material::Settings));
    menu_->AddSeparator();
    menu_->AddEntry("重置游戏", Icons::Glyph(Icons::Material::Update));
    menu_->AddEntry("金手指", Icons::Glyph(Icons::Material::Memory), "", "关");
    menu_->AddEntry("退出游戏", Icons::Glyph(Icons::Material::Close));
    menu_->SetEntryDisabled(6, true); // 重置游戏

    // 子菜单：独立设置
    menu_->EnterSubmenu(3, "独立设置");
    menu_->AddEntry("画面", Icons::Glyph(Icons::Material::ImagePlaceholder), "", "整数缩放");
    menu_->AddEntry("音频", Icons::Glyph(Icons::Material::Memory), "", "80%");
    menu_->AddEntry("按键映射", Icons::Glyph(Icons::Material::Settings));
    menu_->AddEntry("独立金手指", Icons::Glyph(Icons::Material::Memory));
    menu_->LeaveSubmenu();

    // 子菜单：全局设置
    menu_->EnterSubmenu(4, "全局设置");
    menu_->AddEntry("界面主题", Icons::Glyph(Icons::Material::Favorite), "", "Dark+");
    menu_->AddEntry("语言", Icons::Glyph(Icons::Material::Description), "", "中文");
    menu_->AddEntry("存储路径", Icons::Glyph(Icons::Material::Storage));
    menu_->LeaveSubmenu();

    menu_->on_activate = [this](Menu& menu, int index) {
        ++activate_count_;
        last_action_ = menu.EntryLabel(index);
    };

    Box* info = helpers::Column(row, 10.0f);
    info->SetSize(500.0f, 236.0f);
    info->SetPadding(EdgeInsets::All(16.0f));
    info->SetBackground(Theme::kBgEditor);
    info->SetBorder(1.0f, Theme::kBorder);
    info->SetRadius(Theme::kRadiusSmall);
    info->align_x = Align::Start;
    info->Emplace<Label>("Menu 关键点", Theme::kFontHeader, Theme::kTextBright);
    info->Emplace<Label>("· 分隔符 / 图标 / 右侧快捷键提示 / 状态值", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· A 进入子菜单，B 返回上一级（面包屑显示路径）", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· 禁用项会被跳过，不会被焦点选中", Theme::kFontSmall, Theme::kTextPrimary);
    info->Emplace<Label>("· L / R 切换菜单分区（本例只有一个分区）", Theme::kFontSmall, Theme::kTextPrimary);
    status_ = info->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void MenuPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr || menu_ == nullptr) {
        return;
    }
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "操作：%s · 共 %d 次 · 光标 %d/%d · 深度 %d · %s", last_action_.c_str(),
                  activate_count_, menu_->Cursor() + 1, menu_->EntryCount(), menu_->Depth(),
                  menu_->Breadcrumb().empty() ? "(根菜单)" : menu_->Breadcrumb().c_str());
    status_->text = buffer;
}

std::vector<std::pair<Icons::Button, std::string>> MenuPage::Navigation() const {
    return {{Icons::Button::Up, "上移"},
            {Icons::Button::Down, "下移"},
            {Icons::Button::A, "激活 / 进入子菜单"},
            {Icons::Button::B, "返回上一级"},
            {Icons::Button::L, "上一分区"},
            {Icons::Button::R, "下一分区"},
            {Icons::Button::ZL, "第一项"},
            {Icons::Button::ZR, "最后一项"}};
}

void MenuPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Menu", {
                             Row("Vertical", "是（Horizontal 也可）"),
                             Row("Entries / Separator",
                                 (menu_ != nullptr ? std::to_string(menu_->EntryCount()) : "0") + " / 2 处"),
                             Row("Disabled / Shortcut", "重置游戏（跳过）· B / ZL / ZR"),
                             Row("Value", "金手指 = 关"),
                         });
    PushSection(out, "State", {
                               Row("Cursor", menu_ != nullptr ? std::to_string(menu_->Cursor() + 1) : "-", true),
                               Row("Depth", menu_ != nullptr ? std::to_string(menu_->Depth()) : "1", true),
                               Row("Breadcrumb", menu_ != nullptr ? menu_->Breadcrumb() : "-"),
                               Row("Last Action", last_action_, true),
                               Row("Count", std::to_string(activate_count_)),
                           });
    PushSection(out, "Submenu", {
                                  Row("独立设置", "画面 / 音频 / 按键映射 / 独立金手指"),
                                  Row("全局设置", "界面主题 / 语言 / 存储路径"),
                                  Row("返回", "B 键（根菜单时不消费，交给页面）"),
                              });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "移动光标（循环、跳过禁用与分隔符）"),
                                     Row("A / B", "激活 / 进入子菜单 · 返回上一级"),
                                     Row("L / R · ZL / ZR", "切换分区 · 第一 / 最后一项"),
                                 });
}

} // namespace gui_dev::cv
