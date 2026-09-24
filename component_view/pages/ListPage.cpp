#include "component_view/pages/ControlPage.h"

#include <cstdio>
#include <cstdlib>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/components/List.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void ListPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 18.0f);
    row->SetSize(0.0f, 214.0f);

    // 垂直列表（可滚动 + 循环 + 自动滚动）
    vertical_ = row->Emplace<List>("list_vertical");
    vertical_->SetSize(430.0f, 210.0f);
    vertical_->SetName("list_vertical");
    vertical_->orientation = List::Orientation::Vertical;
    vertical_->show_index = true;
    vertical_->loop = true;
    vertical_->item_size = ImVec2(0.0f, 40.0f);
    vertical_->item_gap = ImVec2(0.0f, 4.0f);
    vertical_->SetBackground(Theme::kBgEditor);
    vertical_->SetBorder(1.0f, Theme::kBorderStrong);
    vertical_->SetRadius(Theme::kRadiusSmall);
    static const char* const kGames[] = {"Game 01", "Game 02", "Game 03", "Game 04", "Game 05", "Game 06",
                                          "Game 07", "Game 08", "Game 09", "Game 10", "Game 11", "Game 12"};
    for (int i = 0; i < 12; ++i) {
        vertical_->AddItem(kGames[i], Icons::Glyph(Icons::Material::Games),
                           std::string(6 + (i * 3) % 9, ' ') + std::to_string(2048 + i * 128) + " KB");
    }
    vertical_->SetItemDisabled(3, true);
    vertical_->on_activate = [this](List& list, int index) {
        activated_ = index;
        list.SetSelectedIndex(index);
    };

    Box* column = helpers::Column(row, 12.0f);
    column->SetSize(0.0f, 210.0f);

    // 水平列表
    horizontal_ = column->Emplace<List>("list_horizontal");
    horizontal_->SetSize(430.0f, 74.0f);
    horizontal_->orientation = List::Orientation::Horizontal;
    horizontal_->item_size = ImVec2(132.0f, 68.0f);
    horizontal_->item_gap = ImVec2(8.0f, 0.0f);
    horizontal_->SetBackground(Theme::kBgEditor);
    horizontal_->SetBorder(1.0f, Theme::kBorderStrong);
    horizontal_->SetRadius(Theme::kRadiusSmall);
    for (int i = 0; i < 6; ++i) {
        horizontal_->AddItem("01/" + std::to_string(i + 1), "", "");
    }

    // 网格列表
    List* grid = column->Emplace<List>("list_grid");
    grid->SetSize(430.0f, 120.0f);
    grid->orientation = List::Orientation::Grid;
    grid->columns = 4;
    grid->item_size = ImVec2(0.0f, 52.0f);
    grid->item_gap = ImVec2(8.0f, 8.0f);
    grid->zebra = false;
    grid->SetBackground(Theme::kBgEditor);
    grid->SetBorder(1.0f, Theme::kBorderStrong);
    grid->SetRadius(Theme::kRadiusSmall);
    for (int i = 0; i < 8; ++i) {
        grid->AddItem("图块 " + std::to_string(i + 1));
    }

    if (std::getenv("GUI_DEV_ICON_DEBUG") != nullptr) {
        Box* probe = helpers::Row(host, 12.0f);
        probe->SetSize(0.0f, 32.0f);
        const Icons::Material samples[] = {Icons::Material::Save,   Icons::Material::Memory, Icons::Material::Storage,
                                           Icons::Material::Edit,   Icons::Material::Play,   Icons::Material::Search,
                                           Icons::Material::Games,  Icons::Material::Games};
        for (Icons::Material icon : samples) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%s U+%04X", Icons::Glyph(icon), Icons::Code(icon));
            probe->Emplace<Label>(buffer, Theme::kFontBody, Theme::kTextPrimary);
        }
    }
    status_ = helpers::Caption(host, "", Theme::kFontSmall, Theme::kTeal);
}

void ListPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr || vertical_ == nullptr) {
        return;
    }
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "垂直列表 焦点 %d/%d · 已激活 %s · 滚动 %.0f/%.0f px%s",
                  vertical_->FocusIndex() + 1, vertical_->ItemCount(),
                  activated_ >= 0 ? std::to_string(activated_ + 1).c_str() : "-", vertical_->scroll.y,
                  vertical_->scroll_max.y,
                  Global::focused != nullptr ? (" · 焦点: " + Global::focused->name).c_str() : "");
    status_->text = buffer;
}

std::vector<std::pair<Icons::Button, std::string>> ListPage::Navigation() const {
    return {{Icons::Button::Up, "上移（循环）"},
            {Icons::Button::Down, "下移（循环）"},
            {Icons::Button::Left, "切换列表"},
            {Icons::Button::L, "上一页"},
            {Icons::Button::R, "下一页"},
            {Icons::Button::ZL, "快速滚动"},
            {Icons::Button::A, "激活"},
            {Icons::Button::X, "跳到第一项"},
            {Icons::Button::Y, "跳到最后一项"}};
}

void ListPage::FillProperties(std::vector<PropSection>& out) const {
    const int focus = vertical_ != nullptr ? vertical_->FocusIndex() : 0;
    PushSection(out, "Layout", {
                               Row("Orientation", "Vertical / Horizontal / Grid"),
                               Row("Columns", "Grid = 4"),
                               Row("Item Size", "0x40 / 132x68 / 0x52"),
                               Row("Item Count", vertical_ != nullptr ? std::to_string(vertical_->ItemCount()) : "0"),
                           });
    PushSection(out, "State", {
                               Row("Focused 项", "第 " + std::to_string(focus + 1) + " 项", true),
                               Row("Selected 项", vertical_ != nullptr && vertical_->SelectedIndex() >= 0
                                                      ? "第 " + std::to_string(vertical_->SelectedIndex() + 1) + " 项"
                                                      : "(无)"),
                               Row("Disabled 项", "第 4 项"),
                               Row("Loop", "开（首尾循环）"),
                           });
    PushSection(out, "Scroll", {
                                Row("Focus Auto Scroll", "开（焦点项保持在可见区）", true),
                                Row("Offset / Max", vertical_ != nullptr ? std::to_string(static_cast<int>(vertical_->scroll.y)) + " / " + std::to_string(static_cast<int>(vertical_->scroll_max.y)) + " px" : "0"),
                                Row("Scroll Bar", "常驻"),
                                Row("Item Enter Animation", "开"),
                            });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "移动焦点（到边界后循环）"),
                                     Row("← →", "切换列表（水平/网格内移动）"),
                                     Row("L / R", "按页滚动 · ZL/ZR 快速滚动"),
                                     Row("A / X / Y", "激活 · 第一项 · 最后一项"),
                                 });
}

} // namespace gui_dev::cv
