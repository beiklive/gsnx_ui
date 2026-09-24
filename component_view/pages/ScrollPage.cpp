#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Label.h"
#include "component_view/components/ScrollBox.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void ScrollPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 18.0f);
    row->SetSize(0.0f, 232.1f);

    // 可滚动区域：里面放可聚焦的卡片，焦点移动时自动滚动
    scroll_ = row->Emplace<ScrollBox>("scroll_vertical");
    scroll_->SetSize(460.0f, 227.9f);
    scroll_->SetBackground(Theme::kBgEditor);
    scroll_->SetBorder(1.0f, Theme::kBorderStrong);
    scroll_->SetRadius(Theme::kRadiusSmall);
    scroll_->layout = LayoutMode::Vertical;
    scroll_->gap = ImVec2(0.0f, 6.2f);
    scroll_->align_x = Align::Stretch;
    scroll_->padding = EdgeInsets::All(9.4f);

    cards_.clear();
    for (int i = 0; i < 10; ++i) {
        Button* card = scroll_->Emplace<Button>("滚动内容 " + std::to_string(i + 1));
        card->SetName("scroll_card_" + std::to_string(i));
        card->Secondary();
        card->FitContent(12.5f, 37.4f);
        card->size.x = 0.0f; // 撑满滚动容器宽度
        card->icon = Icons::Glyph(i % 2 == 0 ? Icons::Material::Storage : Icons::Material::Memory);
        connect(card, &Button::clicked, this, [this, i] {
            if (status_ != nullptr) {
                status_->text = "点击了第 " + std::to_string(i + 1) + " 项";
            }
        });
        cards_.push_back(card);
    }

    Box* column = helpers::Column(row, 12.0f);
    column->SetSize(0.0f, 227.9f);

    // 水平滚动
    ScrollBox* horizontal = column->Emplace<ScrollBox>("scroll_horizontal");
    horizontal->SetSize(343.2f, 85.8f);
    horizontal->SetBackground(Theme::kBgEditor);
    horizontal->SetBorder(1.0f, Theme::kBorderStrong);
    horizontal->SetRadius(Theme::kRadiusSmall);
    horizontal->layout = LayoutMode::Horizontal;
    horizontal->gap = ImVec2(7.8f, 0.0f);
    horizontal->padding = EdgeInsets::All(7.8f);
    for (int i = 0; i < 12; ++i) {
        Box* tile = helpers::Tile(horizontal, "横向 " + std::to_string(i + 1), 108.0f, 86.0f, Theme::kRadiusSmall,
                                  Theme::kBgWidget, Theme::kBorder);
        tile->SetFocusable(true);
        tile->focus_frame = true;
        tile->focus_scale = 1.05f;
    }

    // 吸附滚动
    ScrollBox* snapped = column->Emplace<ScrollBox>("scroll_snap");
    snapped->SetSize(343.2f, 82.7f);
    snapped->SetBackground(Theme::kBgEditor);
    snapped->SetBorder(1.0f, Theme::kBorderStrong);
    snapped->SetRadius(Theme::kRadiusSmall);
    snapped->layout = LayoutMode::Vertical;
    snapped->gap = ImVec2(0.0f, 4.7f);
    snapped->padding = EdgeInsets::All(7.8f);
    snapped->scroll_overscroll = false;
    for (int i = 0; i < 5; ++i) {
        Label* line = snapped->Emplace<Label>("吸附滚动示例行 " + std::to_string(i + 1), Theme::kFontSmall,
                                             Theme::kTextPrimary);
        line->SetSize(0.0f, 17.2f);
    }

    status_ = helpers::Caption(host, "方向键移动焦点：容器会把焦点项自动滚进可见区", Theme::kFontSmall, Theme::kTeal);
}

void ScrollPage::OnUpdate(float dt) {
    (void)dt;
    if (status_ == nullptr || scroll_ == nullptr) {
        return;
    }
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "滚动 %.0f / %.0f px · 滚动条 %s · 焦点: %s", scroll_->scroll.y,
                  scroll_->scroll_max.y, scroll_->scroll_bar_auto_hide ? "自动隐藏" : "常驻",
                  Global::focused != nullptr ? Global::focused->name.c_str() : "(无)");
    if (std::string(status_->text).rfind("点击了", 0) != 0) {
        status_->text = buffer;
    }
}

std::vector<std::pair<Icons::Button, std::string>> ScrollPage::Navigation() const {
    return {{Icons::Button::Up, "焦点上移（自动滚动）"},
            {Icons::Button::Down, "焦点下移（自动滚动）"},
            {Icons::Button::L, "上一页"},
            {Icons::Button::R, "下一页"},
            {Icons::Button::ZL, "快速翻页"},
            {Icons::Button::A, "确定"},
            {Icons::Button::Plus, "回到顶部"}};
}

void ScrollPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Layout", {
                               Row("Direction", "Vertical / Horizontal"),
                               Row("Padding", "9 / 7"),
                               Row("Gap", "8"),
                               Row("Clip", "overflow = scroll（自动裁剪）"),
                           });
    PushSection(out, "Scroll", {
                                Row("Smooth Scroll", "指数平滑 14", true),
                                Row("Overscroll", "开（越界回弹 32px）"),
                                Row("Page / Fast", "L/R 85% 屏 · ZL/ZR 200% 屏"),
                                Row("Scroll Bar", "自动隐藏", true),
                            });
    PushSection(out, "Focus", {
                               Row("Focus Auto Scroll", "开（Page 统一调用 EnsureVisible）"),
                               Row("Current", Global::focused != nullptr ? Global::focused->name : "(无)", true),
                               Row("Offset", scroll_ != nullptr ? std::to_string(static_cast<int>(scroll_->scroll.y)) + " px" : "0"),
                           });
    PushSection(out, "Navigation", {
                                     Row("↑ ↓", "移动焦点并自动滚动"),
                                     Row("L / R", "按页滚动"),
                                     Row("ZL / ZR", "快速翻页"),
                                     Row("+", "回到顶部"),
                                 });
}

} // namespace gui_dev::cv
