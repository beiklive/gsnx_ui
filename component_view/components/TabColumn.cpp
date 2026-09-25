#include "component_view/components/TabColumn.h"

#include <cmath>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Button.h"

namespace gui_dev::cv {
namespace {

// 指数平滑：与帧率无关，不会过冲（和 Widget.cpp 里那个同款）
float SmoothTo(float current, float target, float speed, float dt) {
    if (dt <= 0.0f) {
        return current;
    }
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

TabColumn::TabColumn() : Widget("tab_column") {
    layout = LayoutMode::Vertical;
    align_x = Align::Stretch; // 每项撑满列宽（子项自适应轴不会被固化）
    overflow = Overflow::Scroll;
    scroll_bar_auto_hide = true;
    focusable = false; // 焦点停在 item 上，容器自己不占停靠点
    background = 0;    // 列本身不画底/边框（要底色的话由页面套 Box）
}

TabColumn& TabColumn::setItems(std::vector<Item> items) {
    Clear();
    items_ = std::move(items);
    item_buttons_.clear();
    item_buttons_.reserve(items_.size());

    for (std::size_t i = 0; i < items_.size(); ++i) {
        Button* item = Emplace<TextButton>();
        item->SetName("tab_item");
        item->setText(items_[i].text);
        item->setIcon(items_[i].icon);
        item->setTextAlign(TextAlign::Left);
        item->size = ImVec2(0.0f, style.item_height); // 宽交给 Stretch，高固定
        item->focus_phase_offset = 0.13f * static_cast<float>(i); // 流光错开，一排看着不乱
        const int at = static_cast<int>(i);
        connect(item, &Widget::clicked, this, [this, at] { SelectAt(at); });
        item_buttons_.push_back(item);
    }

    index_ = ClampIndex(index_);
    indicator_y_ = -1.0f; // 换了数据不播滑动动画
    gap.y = style.item_gap;
    SetFocusZone(style.focus_zone); // 整列（含 item）同一分区
    return *this;
}

TabColumn& TabColumn::setIndex(int value, bool notify) {
    if (items_.empty()) {
        return *this;
    }
    const int next = ClampIndex(value);
    if (next == index_) {
        return *this;
    }
    index_ = next;
    if (notify) {
        emit selectionChanged(index_);
    }
    return *this;
}

Button* TabColumn::itemAt(int index) const {
    if (index < 0 || index >= static_cast<int>(item_buttons_.size())) {
        return nullptr;
    }
    return item_buttons_[static_cast<std::size_t>(index)];
}

TabColumn& TabColumn::setFocusTarget(Widget* target) {
    focus_target_ = target;
    return *this;
}

int TabColumn::ClampIndex(int value) const {
    if (items_.empty()) {
        return 0;
    }
    const int last = static_cast<int>(items_.size()) - 1;
    return value < 0 ? 0 : (value > last ? last : value);
}

void TabColumn::SelectAt(int index) {
    if (index == index_) {
        emit activated(index); // 对已选中项再点一次 / 再按 A
        return;
    }
    setIndex(index);
}

void TabColumn::ApplyItemLook(int index) {
    Button* item = item_buttons_[static_cast<std::size_t>(index)];
    const bool selected = (index == index_);

    // 平铺：没有底色 / 边框 / 阴影，选中底由容器在 OnDrawContent 里画（画在文字下面）
    item->background = 0;
    item->background_follows_theme = false;
    item->border.width = 0.0f;
    item->border.color = 0;
    item->shadow.enabled = false;
    item->padding = EdgeInsets{style.content_padding, style.padding_y, 12.0f, style.padding_y};
    item->size.y = style.item_height;
    // 文字色每帧写死（选中 = 亮、未选中 = 常规），切主题也会被下一帧覆盖
    item->text_color = selected ? Theme::kTextBright : Theme::kTextPrimary;
    item->text_color_follows_theme = false;
}

ImVec2 TabColumn::MeasureContent(const ImVec2& available) {
    (void)available;
    return ImVec2(0.0f, 0.0f); // 宽度/高度都由页面显式给
}

void TabColumn::OnUpdate(float dt) {
    if (item_buttons_.empty()) {
        return;
    }
    for (int i = 0; i < static_cast<int>(item_buttons_.size()); ++i) {
        ApplyItemLook(i);
    }

    // 选中底：平滑滑到新选中项（第一帧直接对齐）
    const Rect& target = item_buttons_[static_cast<std::size_t>(ClampIndex(index_))]->rect;
    if (indicator_y_ < 0.0f) {
        indicator_y_ = target.min.y;
        indicator_h_ = target.Height();
    } else {
        indicator_y_ = SmoothTo(indicator_y_, target.min.y, style.slide_speed, dt);
        indicator_h_ = SmoothTo(indicator_h_, target.Height(), style.slide_speed, dt);
    }

    // 焦点自动滚动：焦点项变了、而且落在本列里，就把它滚进可见区
    Widget* focused = Global::focused;
    if (focused != last_focus_) {
        if (focused != nullptr && ContainsDescendant(focused)) {
            EnsureVisible(focused);
        }
        last_focus_ = focused;
    }
}

bool TabColumn::OnPadAction(InputAction action) {
    // → / R：进内容区；← 由内容区自己按导航规则跨分区回来（Global::NavigateFocus）
    if (action == InputAction::PageRight) {
        if (focus_target_ != nullptr) {
            focus_target_->RequestFocus();
            return true;
        }
        return false;
    }
    return false;
}

Rect TabColumn::MapFromSelf(const Rect& r) const {
    // 自身绘制变换是「绕矩形中心缩放 + 平移」的仿射变换，所以直接按自身 rect.min 做差即可
    const float scale = DrawScale();
    const ImVec2 origin = DrawRect().min;
    return Rect{ImVec2(origin.x + (r.min.x - rect.min.x) * scale, origin.y + (r.min.y - rect.min.y) * scale),
                ImVec2(origin.x + (r.max.x - rect.min.x) * scale, origin.y + (r.max.y - rect.min.y) * scale)};
}

void TabColumn::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)content;
    if (dl == nullptr || item_buttons_.empty()) {
        return;
    }
    // 选中底 + 左侧色条：画在子项之前（子项是它的文字/图标，必须盖在上面）
    const Rect base = MapFromSelf(Rect{ImVec2(rect.min.x, indicator_y_),
                                       ImVec2(rect.max.x, indicator_y_ + indicator_h_)});
    const float radius = style.item_radius > 0.0f ? style.item_radius : base.Height() * 0.5f;
    Draw::RoundedRectFilled(dl, base, Theme::U32(Theme::kSelection), radius, radius, radius, radius);

    if (style.indicator_width > 0.0f) {
        const float bar_h = Maxf(base.Height() - style.indicator_margin_y * 2.0f, 8.0f);
        const float bar_r = style.indicator_width * 0.5f;
        const Rect bar = Rect::FromPosSize(
            ImVec2(base.min.x + style.indicator_inset, base.Center().y - bar_h * 0.5f),
            ImVec2(style.indicator_width, bar_h));
        Draw::RoundedRectFilled(dl, bar, Theme::U32(Theme::kAccent), bar_r, bar_r, bar_r, bar_r);
    }
}

} // namespace gui_dev::cv
