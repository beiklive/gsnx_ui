#include "component_view/components/FunctionBar.h"

#include <utility>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Button.h"

namespace gui_dev::cv {

FunctionBar::FunctionBar() : Box("function_bar") {
    // 容器 = 胶囊：圆角在布局里按「高度的一半」定（左右半圆、上下直线），
    // 边框 / 阴影仍走全局约定，底色比面板亮一档（Theme::kBgWidget）。
    applyComponentStyle();
    background = Theme::U32(Theme::kBgWidget);
    background_follows_theme = false; // 显式设过色：切主题时由 OnThemeChanged 重新取
    layout = LayoutMode::Free;        // 子按钮的位置自己算（等距摊开），不走 Box 的横向流式布局
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
    corner_radius = style.radius >= 0.0f ? style.radius : CapsuleHeight() * 0.5f;
    for (IconButton* button : buttons_) {
        if (button != nullptr) {
            button->setSide(style.item_size);
        }
    }
    return *this;
}

IconButton* FunctionBar::itemAt(int index) const {
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

float FunctionBar::CapsuleHeight() const {
    return style.item_size + style.capsule_padding * 2.0f;
}

float FunctionBar::NaturalCapsuleWidth() const {
    const float count = static_cast<float>(buttons_.size());
    if (count <= 0.0f) {
        return 0.0f;
    }
    return style.edge_padding * 2.0f + count * style.item_size + (count - 1.0f) * style.gap;
}

float FunctionBar::CapsuleWidth() const {
    const float available = Maxf(rect.Width(), 0.0f);
    const float natural = NaturalCapsuleWidth();
    return available > 0.0f ? Minf(natural, available) : natural;
}

void FunctionBar::Rebuild() {
    Clear();
    buttons_.clear();
    label_index_ = -1;
    label_alpha_ = 0.0f;

    for (std::size_t i = 0; i < items_.size(); ++i) {
        // 无边框圆形按钮：现成的 IconButton（圆形形态），去掉边框、底色与阴影，只留图标 + 流光焦点框
        IconButton* button = Emplace<IconButton>(items_[i].icon);
        button->SetName("function_item");
        button->setShape(IconButtonShape::Circle);
        button->setSide(style.item_size);
        button->border.width = 0.0f;
        button->shadow.enabled = false;
        button->background = 0;
        button->background_follows_theme = false;
        button->showSubtitle(false); // 名字改由本组件画在胶囊下方（只在聚焦时显示）
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
    // 重建出来的项要跟容器同一个焦点分区：宿主通常是「先 AddTo（设分区）再 SetItems」，
    // 新建的子节点默认分区是 0，不跟着走的话方向键在分区之间导航就找不到它们。
    SetFocusZone(focus_zone);
}

ImVec2 FunctionBar::MeasureContent(const ImVec2& available) {
    const float capsule_h = CapsuleHeight();
    const float natural = NaturalCapsuleWidth();
    const float width = size.x > 0.0f ? size.x : Maxf(Minf(natural, available.x), 1.0f);
    const float chrome_x = padding.Horizontal() + (border.width + border.inset) * 2.0f;
    const float chrome_y = padding.Vertical() + (border.width + border.inset) * 2.0f;
    const float inner_w = Maxf(width - chrome_x, 1.0f);
    // 高度 = 胶囊 + 名称行（名称行常驻占位，聚焦时布局不会跳）
    const float height = size.y > 0.0f ? size.y : (capsule_h + style.label_height + chrome_y);

    // 胶囊：宽度取「自然宽度」与可用宽度的小值，整体居中（GBAStation 的功能条也是居中的）
    const float capsule_w = Minf(natural, inner_w);
    capsule_rect_ = Rect{ImVec2((inner_w - capsule_w) * 0.5f, 0.0f), ImVec2((inner_w + capsule_w) * 0.5f, capsule_h)};
    corner_radius = style.radius >= 0.0f ? style.radius : capsule_h * 0.5f; // 左右半圆、上下直线

    // 按钮：在胶囊里等距摊开（两端各留 edge_padding）
    const int count = static_cast<int>(buttons_.size());
    if (count > 0) {
        const float first_x = capsule_rect_.min.x + style.edge_padding;
        const float last_x = capsule_rect_.max.x - style.edge_padding;
        const float step = count > 1 ? (last_x - first_x - style.item_size) / static_cast<float>(count - 1) : 0.0f;
        const float y = capsule_rect_.min.y + (capsule_h - style.item_size) * 0.5f;
        for (int i = 0; i < count; ++i) {
            buttons_[static_cast<std::size_t>(i)]->position = ImVec2(first_x + step * static_cast<float>(i), y);
        }
    }
    return ImVec2(width, height);
}

void FunctionBar::OnUpdate(float dt) {
    const int focused = focusedIndex();
    if (focused >= 0) {
        label_index_ = focused;
    }
    // 名称：有焦点就淡入，没焦点就淡出（淡出期间仍显示最后一次的名字，避免闪烁）
    label_alpha_ = Anim::SmoothTo(label_alpha_, focused >= 0 ? 1.0f : 0.0f, style.label_fade, dt);
}

// 胶囊容器画在 OnDrawContent（子按钮之前）：它必须垫在圆形按钮下面。
void FunctionBar::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr || count() == 0) {
        return;
    }
    const float opacity = EffectiveOpacity();
    // ---- 胶囊容器：底色 + 边框 + 阴影都按全局约定，圆角 = 高度一半（左右半圆、上下直线）----
    const Rect capsule = capsule_rect_.Translate(content.min);
    Draw::SoftShadow(dl, capsule, shadow, corner_radius, corner_radius, corner_radius, corner_radius);
    Draw::RoundedRectFilled(dl, capsule, Theme::Alpha(background, opacity), corner_radius, corner_radius,
                            corner_radius, corner_radius);
    if (border.width > 0.0f) {
        Draw::RoundedRectOutline(dl, capsule, Theme::Alpha(border.color, opacity), border.width, corner_radius,
                                 corner_radius, corner_radius, corner_radius);
    }
}

// 聚焦项名称画在 OnDrawOverlay（子按钮之后）：它落在胶囊下方，要盖住一切。
void FunctionBar::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr || count() == 0) {
        return;
    }
    const float opacity = EffectiveOpacity();
    const Rect capsule = capsule_rect_.Translate(content.min);

    // ---- 聚焦项名称：画在胶囊下方居中，只在有焦点时显示（淡入淡出）----
    if (label_index_ < 0 || label_index_ >= static_cast<int>(items_.size()) || label_alpha_ <= 0.02f) {
        return;
    }
    const std::string& text = items_[static_cast<std::size_t>(label_index_)].label;
    if (text.empty()) {
        return;
    }
    const ImVec2 extent = Draw::MeasureText(nullptr, style.label_size, text.c_str(), 0.0f);
    Draw::Text(dl, nullptr, style.label_size,
               ImVec2(capsule.Center().x - extent.x * 0.5f,
                      capsule.max.y + style.label_gap + (style.label_height - extent.y) * 0.5f - style.label_gap),
               Theme::U32(Theme::kTextPrimary, opacity * label_alpha_), text.c_str());
}

void FunctionBar::OnThemeChanged() {
    applyComponentStyle();                     // 边框 / 阴影跟随全局约定
    background = Theme::U32(Theme::kBgWidget); // 容器底色跟主题
    background_follows_theme = false;
    corner_radius = style.radius >= 0.0f ? style.radius : CapsuleHeight() * 0.5f;
}

} // namespace gui_dev::cv
