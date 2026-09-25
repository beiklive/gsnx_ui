#include "component_view/components/TabColumn.h"

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Button.h"

namespace gui_dev::cv {

TabColumn::TabColumn() : Widget("tab_column") {
    layout = LayoutMode::Vertical;
    align_x = Align::Stretch; // 每项撑满列宽（子项自适应轴不会被固化）
    overflow = Overflow::Scroll;
    scroll_bar_auto_hide = true;
    focusable = false; // 焦点停在 item 上，容器自己不占停靠点
    background = 0;    // 列本身不画底/边框（要底色的话由页面套 Box）
    // 留出一点内边距：溢出裁剪是按本节点 rect 裁的，item 的流光焦点框会往外扩 2~3px，
    // 不留边距的话焦点框会被切掉（只剩一条底边）。
    padding = EdgeInsets::All(6.0f);
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
    focus_anim_.assign(item_buttons_.size(), 0.0f);
    gap.y = style.item_gap;
    SetFocusZone(style.focus_zone); // 整列（含 item）同一分区
    PlayEnter();                    // 建好就播一次入场
    return *this;
}

void TabColumn::PlayEnter() {
    enter_time_ = 0.0f;
}

float TabColumn::TotalEnterTime() const {
    return Anim::StaggerTotal(count(), style.enter_stagger, style.enter_duration);
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

float TabColumn::ItemEnter(int index) const {
    const float raw = Anim::StaggerElapsed(enter_time_, index, style.enter_stagger, style.enter_duration);
    return Anim::EaseOutCubic(raw);
}

float TabColumn::ItemFocus(int index) const {
    if (index < 0 || index >= static_cast<int>(focus_anim_.size())) {
        return 0.0f;
    }
    return Anim::EaseOutBack(focus_anim_[static_cast<std::size_t>(index)]);
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
    const float enter = ItemEnter(index);
    const float focus = ItemFocus(index);

    // 平铺：没有底色 / 边框 / 阴影，选中底由容器在 OnDrawContent 里画（画在文字下面）
    item->background = 0;
    item->background_follows_theme = false;
    item->border.width = 0.0f;
    item->border.color = 0;
    item->shadow.enabled = false;
    item->padding = EdgeInsets{style.content_padding, style.padding_y, 12.0f, style.padding_y};
    item->size.y = style.item_height;
    // 圆角跟选中底一致：按钮的流光焦点框按 corner_radius + margin 画，
    // 不跟着改的话焦点框的圆角会和选中底对不上。
    item->corner_radius =
        style.item_radius > 0.0f ? style.item_radius : style.item_height * 0.5f;
    // 文字色每帧写死（选中 = 亮、未选中 = 常规；聚焦再往亮色靠一点），切主题下一帧自动覆盖
    const ImVec4 base = selected ? Theme::kTextBright : Theme::kTextPrimary;
    item->text_color = Theme::Mix(base, Theme::kTextBright, focus * 0.6f);
    item->text_color_follows_theme = false;

    // 入场：从左侧滑入 + 淡入（opacity 现在对文字/图标也生效）；焦点响应：轻微右移
    item->visual_translate = ImVec2(style.enter_offset * (1.0f - enter) + style.focus_offset * focus, 0.0f);
    item->opacity = enter;
}

ImVec2 TabColumn::MeasureContent(const ImVec2& available) {
    (void)available;
    return ImVec2(0.0f, 0.0f); // 宽度/高度都由页面显式给
}

void TabColumn::OnUpdate(float dt) {
    if (item_buttons_.empty()) {
        return;
    }
    enter_time_ = Minf(enter_time_ + Maxf(dt, 0.0f), TotalEnterTime());
    if (focus_anim_.size() != item_buttons_.size()) {
        focus_anim_.assign(item_buttons_.size(), 0.0f);
    }
    for (int i = 0; i < static_cast<int>(item_buttons_.size()); ++i) {
        const bool focused = item_buttons_[static_cast<std::size_t>(i)]->focused;
        focus_anim_[static_cast<std::size_t>(i)] =
            Anim::MoveTowards(focus_anim_[static_cast<std::size_t>(i)], focused ? 1.0f : 0.0f, style.focus_duration, dt);
        ApplyItemLook(i);
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
    // 选中底 + 左侧色条：画在子项之前（子项是它的文字/图标，必须盖在上面）。
    // 底不做位移动画：直接用选中项当前这一帧的矩形（含它自己的入场/焦点位移），
    // 所以切到哪一项就落在哪一项上，不会从上一项滑过去。
    const int selected = ClampIndex(index_);
    const float enter = ItemEnter(selected);
    const float focus = ItemFocus(selected);
    const float dx = style.enter_offset * (1.0f - enter) + style.focus_offset * focus;
    const Rect item = item_buttons_[static_cast<std::size_t>(selected)]->rect.Translate(ImVec2(dx, 0.0f));
    const Rect base = MapFromSelf(item).Inset(0.0f, 0.0f, 0.0f, 0.0f);
    const float alpha = enter;

    const float radius = style.item_radius > 0.0f ? style.item_radius : base.Height() * 0.5f;
    Draw::RoundedRectFilled(dl, base, Theme::Alpha(Theme::U32(Theme::kSelection), alpha), radius, radius, radius,
                            radius);

    if (style.indicator_width > 0.0f) {
        const float bar_h = Maxf(base.Height() - style.indicator_margin_y * 2.0f, 8.0f);
        const float bar_r = style.indicator_width * 0.5f;
        const Rect bar = Rect::FromPosSize(
            ImVec2(base.min.x + style.indicator_inset, base.Center().y - bar_h * 0.5f),
            ImVec2(style.indicator_width, bar_h));
        Draw::RoundedRectFilled(dl, bar, Theme::Alpha(Theme::U32(Theme::kAccent), alpha), bar_r, bar_r, bar_r, bar_r);
    }
}

} // namespace gui_dev::cv
