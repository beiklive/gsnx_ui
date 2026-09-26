#include "component_view/components/FunctionBar.h"

#include <utility>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Button.h"

namespace gui_dev::cv {

FunctionBar::FunctionBar() : Box("function_bar") {
    // 条本身就是一块普通面板：圆角 / 边框 / 阴影都走全局约定，和弹窗、Tab 面板长得一致
    applyComponentStyle();
    background = Theme::U32(Theme::kBgPanel);
    background_follows_theme = false; // 显式设过色：切主题时由 OnThemeChanged 重新取
    layout = LayoutMode::Horizontal;
    align_y = Align::Center;
}

FunctionBar& FunctionBar::SetItems(std::vector<Item> items) {
    items_ = std::move(items);
    Rebuild();
    return *this;
}

FunctionBar& FunctionBar::AddItem(std::string icon, std::string label, std::function<void()> on_activate) {
    Item item;
    item.icon = std::move(icon);
    item.label = std::move(label);
    item.on_activate = std::move(on_activate);
    items_.push_back(std::move(item));
    Rebuild();
    return *this;
}

FunctionBar& FunctionBar::SetStyle(const Style& value) {
    style = value;
    padding = EdgeInsets::All(style.padding >= 0.0f ? style.padding : Global::component_style.content_padding);
    corner_radius = style.radius >= 0.0f ? style.radius : Global::component_style.corner_radius;
    gap = ImVec2(style.gap, 0.0f);
    for (Button* button : buttons_) {
        if (button != nullptr) {
            button->setIconCellSize(style.icon_size);
        }
    }
    return *this;
}

Button* FunctionBar::itemAt(int index) const {
    if (index < 0 || index >= static_cast<int>(buttons_.size())) {
        return nullptr;
    }
    return buttons_[index];
}

int FunctionBar::focusedIndex() const {
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i] != nullptr && buttons_[i]->hasFocus()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FunctionBar::Rebuild() {
    Clear();
    buttons_.clear();

    // 默认样式：内边距 / 圆角取全局约定，间距按 Style
    if (style.padding < 0.0f) {
        padding = EdgeInsets::All(Global::component_style.content_padding);
    }
    if (style.radius < 0.0f) {
        corner_radius = Global::component_style.corner_radius;
    }
    gap = ImVec2(style.gap, 0.0f);

    for (std::size_t i = 0; i < items_.size(); ++i) {
        // Button 的「纯图标 + 说明行」形态：图标居中在上，说明行在图标下方 —— 正好是功能项的样子
        Button* button = Emplace<Button>();
        button->SetName("function_item");
        button->setText("");
        button->setIcon(items_[i].icon);
        button->setIconCellSize(style.icon_size);
        button->setSubtitle(items_[i].label, true);
        button->showSubtitle(true);
        button->setFontSize(Theme::kFontBody, Theme::kFontSmall);
        buttons_.push_back(button);

        const std::function<void()> action = items_[i].on_activate;
        const int index = static_cast<int>(i);
        connect(button, &Widget::clicked, this, [this, action, index] {
            if (action) {
                action(); // 宿主自己的动作
            }
            emit activated(index);
        });
    }
    // 重建出来的项要跟条本身同一个焦点分区：宿主通常是「先 AddTo（设分区）再 SetItems」，
    // 新建的子节点默认分区是 0，不跟着走的话方向键在分区之间导航就找不到它们。
    SetFocusZone(focus_zone);
}

ImVec2 FunctionBar::MeasureContent(const ImVec2& available) {
    // 等分宽度：整条宽度减去内边距与间距后，N 项各拿一份（不足最小宽度就按最小宽度，条自己横向溢出）
    const float per_item_gap = static_cast<float>(buttons_.empty() ? 0 : buttons_.size() - 1) * style.gap;
    const float inner = available.x > 0.0f ? available.x : 0.0f;
    const float each =
        buttons_.empty()
            ? 0.0f
            : Maxf((inner - per_item_gap) / static_cast<float>(buttons_.size()), style.item_min_width);
    for (Button* button : buttons_) {
        if (button != nullptr) {
            button->resize(each, style.item_height);
        }
    }
    const float width = each * static_cast<float>(buttons_.size()) + per_item_gap;
    return ImVec2(buttons_.empty() ? 0.0f : width, style.item_height);
}

void FunctionBar::OnThemeChanged() {
    applyComponentStyle();                       // 边框 / 阴影跟随全局约定
    background = Theme::U32(Theme::kBgPanel);    // 面板底色跟主题
    background_follows_theme = false;
    corner_radius = style.radius >= 0.0f ? style.radius : Global::component_style.corner_radius;
}

} // namespace gui_dev::cv
