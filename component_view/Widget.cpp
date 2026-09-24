#include "component_view/Widget.h"

#include <algorithm>
#include <cmath>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 指数平滑：与帧率无关，不会过冲。
float SmoothTo(float current, float target, float speed, float dt) {
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

} // namespace

Widget::Widget() {
    id = Global::next_widget_id++;
}

Widget::Widget(std::string widget_name) : name(std::move(widget_name)) {
    id = Global::next_widget_id++;
}

Widget& Widget::Add(std::unique_ptr<Widget> child) {
    if (child != nullptr) {
        child->parent = this;
        children.push_back(std::move(child));
    }
    return *this;
}

Widget* Widget::Find(const std::string& widget_name) {
    if (!widget_name.empty() && name == widget_name) {
        return this;
    }
    for (auto& child : children) {
        if (Widget* found = child->Find(widget_name)) {
            return found;
        }
    }
    return nullptr;
}

bool Widget::ContainsDescendant(const Widget* target) const {
    if (target == nullptr) {
        return false;
    }
    for (const auto& child : children) {
        if (child.get() == target || child->ContainsDescendant(target)) {
            return true;
        }
    }
    return false;
}

void Widget::Remove(Widget* child) {
    children.erase(std::remove_if(children.begin(), children.end(),
                                  [child](const std::unique_ptr<Widget>& item) { return item.get() == child; }),
                   children.end());
}

void Widget::Clear() {
    children.clear();
}

// ---------------------------------------------------------------- 布局 ----

void Widget::LayoutTree(const ImVec2& parent_content_pos, const ImVec2& parent_content_size) {
    Measure(parent_content_size);
    Place(parent_content_pos, parent_content_size);
}

void Widget::Measure(const ImVec2& available) {
    // padding / border 占掉的固定空间：size 是外框尺寸，所以内容可用空间要先扣掉。
    const float chrome_x = padding.Horizontal() + (border.width + border.inset) * 2.0f;
    const float chrome_y = padding.Vertical() + (border.width + border.inset) * 2.0f;
    const ImVec2 content_available(Maxf(0.0f, available.x - margin.Horizontal() - chrome_x),
                                   Maxf(0.0f, available.y - margin.Vertical() - chrome_y));

    ImVec2 content_desired = MeasureContent(content_available);
    if (!children.empty() && (size.x <= 0.0f || size.y <= 0.0f)) {
        const ImVec2 flow = FlowChildrenSize(content_available);
        content_desired.x = Maxf(content_desired.x, flow.x);
        content_desired.y = Maxf(content_desired.y, flow.y);
    }

    ImVec2 outer(chrome_x + content_desired.x, chrome_y + content_desired.y);
    // 显式尺寸优先（size 是外框尺寸）
    if (size.x > 0.0f) {
        outer.x = size.x;
    }
    if (size.y > 0.0f) {
        outer.y = size.y;
    }
    if (aspect_ratio > 0.0f) {
        if (size.x > 0.0f && size.y <= 0.0f) {
            outer.y = outer.x / aspect_ratio;
        } else if (size.y > 0.0f && size.x <= 0.0f) {
            outer.x = outer.y * aspect_ratio;
        }
    }
    if (min_size.x > 0.0f) {
        outer.x = Maxf(outer.x, min_size.x);
    }
    if (min_size.y > 0.0f) {
        outer.y = Maxf(outer.y, min_size.y);
    }
    if (max_size.x > 0.0f) {
        outer.x = Minf(outer.x, max_size.x);
    }
    if (max_size.y > 0.0f) {
        outer.y = Minf(outer.y, max_size.y);
    }

    // 自适应轴不能超过父给的空间（显式尺寸允许溢出，由父节点决定是否裁剪）
    if (size.x <= 0.0f) {
        outer.x = Minf(outer.x, Maxf(available.x - margin.Horizontal(), 0.0f));
    }
    if (size.y <= 0.0f) {
        outer.y = Minf(outer.y, Maxf(available.y - margin.Vertical(), 0.0f));
    }

    measured_size = ImVec2(Maxf(outer.x, 0.0f), Maxf(outer.y, 0.0f));
}

ImVec2 Widget::FlowChildrenSize(const ImVec2& content_available) {
    ImVec2 total(0.0f, 0.0f);
    if (layout == LayoutMode::Vertical) {
        float cursor_y = 0.0f;
        float max_width = 0.0f;
        for (auto& pointer : children) {
            Widget* child = pointer.get();
            child->Measure(ImVec2(content_available.x, Maxf(0.0f, content_available.y - cursor_y)));
            max_width = Maxf(max_width, child->measured_size.x + child->margin.Horizontal());
            cursor_y += child->measured_size.y + child->margin.Vertical() + gap.y + child->position.y;
        }
        total.x = max_width;
        total.y = Maxf(0.0f, cursor_y - gap.y);
    } else if (layout == LayoutMode::Horizontal) {
        float cursor_x = 0.0f;
        float max_height = 0.0f;
        for (auto& pointer : children) {
            Widget* child = pointer.get();
            child->Measure(ImVec2(Maxf(0.0f, content_available.x - cursor_x), content_available.y));
            max_height = Maxf(max_height, child->measured_size.y + child->margin.Vertical());
            cursor_x += child->measured_size.x + child->margin.Horizontal() + gap.x + child->position.x;
        }
        total.x = Maxf(0.0f, cursor_x - gap.x);
        total.y = max_height;
    } else {
        for (auto& pointer : children) {
            Widget* child = pointer.get();
            child->Measure(content_available);
            total.x =
                Maxf(total.x, child->position.x + child->margin.left + child->measured_size.x + child->margin.right);
            total.y =
                Maxf(total.y, child->position.y + child->margin.top + child->measured_size.y + child->margin.bottom);
        }
    }
    return total;
}

void Widget::Place(const ImVec2& parent_content_pos, const ImVec2& parent_content_size) {
    // anchor 是「锚点」，pivot 是「自身哪个点贴到锚点上」：
    //   锚点 = 父内容区去掉 margin 后的轨道上按 anchor 比例取点
    //   矩形左上 = 锚点 - 自身尺寸 * pivot
    const ImVec2 track(Maxf(parent_content_size.x - margin.Horizontal(), 0.0f),
                       Maxf(parent_content_size.y - margin.Vertical(), 0.0f));
    const ImVec2 anchor_point(parent_content_pos.x + margin.left + position.x + track.x * anchor.x,
                              parent_content_pos.y + margin.top + position.y + track.y * anchor.y);

    const ImVec2 pos(anchor_point.x - measured_size.x * pivot.x + offset.x,
                     anchor_point.y - measured_size.y * pivot.y + offset.y);

    rect = Rect::FromPosSize(pos, measured_size);
    margin_rect = rect.Inset(-margin.left, -margin.top, -margin.right, -margin.bottom);

    const float bleed = border.width + border.inset;
    content_rect = rect.Inset(padding.left + bleed, padding.top + bleed, padding.right + bleed, padding.bottom + bleed);

    PlaceChildren();
    OnAfterLayout();

    // 滚动上限：内容比内容区大多少就能滚多少
    const ImVec2 view(content_rect.Width(), content_rect.Height());
    scroll_max = ImVec2(Maxf(0.0f, content_extent.x - view.x), Maxf(0.0f, content_extent.y - view.y));
    if (overflow != Overflow::Scroll || !scroll_enabled) {
        scroll = ImVec2(0.0f, 0.0f);
        scroll_target = ImVec2(0.0f, 0.0f);
        scroll_max = ImVec2(0.0f, 0.0f);
    } else {
        if (!scroll_overscroll) {
            scroll.x = Clampf(scroll.x, -0.0f, scroll_max.x);
            scroll.y = Clampf(scroll.y, -0.0f, scroll_max.y);
        }
        scroll_target.x = Clampf(scroll_target.x, 0.0f, scroll_max.x);
        scroll_target.y = Clampf(scroll_target.y, 0.0f, scroll_max.y);
    }
}

void Widget::PlaceChildren() {
    if (children.empty()) {
        content_extent = ImVec2(0.0f, 0.0f);
        return;
    }
    const bool scroll_layout = overflow == Overflow::Scroll && scroll_enabled;
    const ImVec2 origin = scroll_layout ? ImVec2(content_rect.min.x - scroll.x, content_rect.min.y - scroll.y)
                                        : content_rect.min;
    const ImVec2 area(content_rect.Width(), content_rect.Height());
    ImVec2 cursor(0.0f, 0.0f);
    ImVec2 extent(0.0f, 0.0f);

    for (auto& pointer : children) {
        Widget* child = pointer.get();

        if (layout == LayoutMode::Vertical) {
            const ImVec2 child_area(area.x, Maxf(0.0f, area.y - cursor.y));
            const float saved_x = child->size.x;
            // 交叉轴 Stretch：临时撑满宽度，测量/定位完再还原，避免把自适应尺寸固化下来。
            if (align_x == Align::Stretch && saved_x <= 0.0f) {
                child->size.x = Maxf(0.0f, area.x - child->margin.Horizontal());
            }
            child->Measure(child_area);
            child->Place(ImVec2(origin.x, origin.y + cursor.y), child_area);
            child->size.x = saved_x;

            const float cross_slack = area.x - child->margin.Horizontal() - child->rect.Width();
            if (cross_slack > 0.0f) {
                if (align_x == Align::Center) {
                    child->Move(ImVec2(cross_slack * 0.5f, 0.0f));
                } else if (align_x == Align::End) {
                    child->Move(ImVec2(cross_slack, 0.0f));
                }
            }
            cursor.y += child->rect.Height() + child->margin.Vertical() + gap.y + child->position.y;
            extent.x = Maxf(extent.x, child->rect.max.x - origin.x);
            extent.y = Maxf(extent.y, child->rect.max.y - origin.y);
        } else if (layout == LayoutMode::Horizontal) {
            const ImVec2 child_area(Maxf(0.0f, area.x - cursor.x), area.y);
            const float saved_y = child->size.y;
            if (align_y == Align::Stretch && saved_y <= 0.0f) {
                child->size.y = Maxf(0.0f, area.y - child->margin.Vertical());
            }
            child->Measure(child_area);
            child->Place(ImVec2(origin.x + cursor.x, origin.y), child_area);
            child->size.y = saved_y;

            const float cross_slack = area.y - child->margin.Vertical() - child->rect.Height();
            if (cross_slack > 0.0f) {
                if (align_y == Align::Center) {
                    child->Move(ImVec2(0.0f, cross_slack * 0.5f));
                } else if (align_y == Align::End) {
                    child->Move(ImVec2(0.0f, cross_slack));
                }
            }
            cursor.x += child->rect.Width() + child->margin.Horizontal() + gap.x + child->position.x;
            extent.x = Maxf(extent.x, child->rect.max.x - origin.x);
            extent.y = Maxf(extent.y, child->rect.max.y - origin.y);
        } else {
            child->Measure(area);
            child->Place(origin, area);
            extent.x = Maxf(extent.x, child->rect.max.x - origin.x);
            extent.y = Maxf(extent.y, child->rect.max.y - origin.y);
        }
    }

    // 主轴对齐：排列完成后内容没占满时，整体平移子节点（Start/Center/End）。
    if (layout == LayoutMode::Vertical && align_y != Align::Start) {
        const float used = Maxf(cursor.y - gap.y, 0.0f);
        const float slack = area.y - used;
        if (slack > 0.0f) {
            const float delta = align_y == Align::Center ? slack * 0.5f : slack;
            for (auto& child : children) {
                child->Move(ImVec2(0.0f, delta));
            }
            extent.y += delta;
        }
    } else if (layout == LayoutMode::Horizontal && align_x != Align::Start) {
        const float used = Maxf(cursor.x - gap.x, 0.0f);
        const float slack = area.x - used;
        if (slack > 0.0f) {
            const float delta = align_x == Align::Center ? slack * 0.5f : slack;
            for (auto& child : children) {
                child->Move(ImVec2(delta, 0.0f));
            }
            extent.x += delta;
        }
    }

    if (layout == LayoutMode::Vertical) {
        extent.x = Maxf(extent.x, cursor.x);
        extent.y = Maxf(extent.y, Maxf(cursor.y - gap.y, 0.0f));
    } else if (layout == LayoutMode::Horizontal) {
        extent.x = Maxf(extent.x, Maxf(cursor.x - gap.x, 0.0f));
    }
    content_extent = extent;
}

void Widget::Move(const ImVec2& delta) {
    rect = rect.Translate(delta);
    margin_rect = margin_rect.Translate(delta);
    content_rect = content_rect.Translate(delta);
    for (auto& child : children) {
        child->Move(delta);
    }
}

// ------------------------------------------------------------ 滚动 ----

Widget* Widget::ScrollHost() {
    Widget* node = this;
    while (node != nullptr) {
        if (node->overflow == Overflow::Scroll && node->scroll_enabled) {
            return node;
        }
        node = node->parent;
    }
    return nullptr;
}

void Widget::UpdateScroll(float dt) {
    if (overflow != Overflow::Scroll || !scroll_enabled) {
        return;
    }
    scroll.x = SmoothTo(scroll.x, scroll_target.x, scroll_smoothing, dt);
    scroll.y = SmoothTo(scroll.y, scroll_target.y, scroll_smoothing, dt);
    if (scroll_overscroll) {
        // 越界回弹：允许短暂超出，靠目标值把它拉回来
        scroll.x = Clampf(scroll.x, -48.0f, scroll_max.x + 48.0f);
        scroll.y = Clampf(scroll.y, -48.0f, scroll_max.y + 48.0f);
    } else {
        scroll.x = Clampf(scroll.x, 0.0f, scroll_max.x);
        scroll.y = Clampf(scroll.y, 0.0f, scroll_max.y);
    }

    // 滚动条自动隐藏：有滚动动作时亮起，静置一段时间后淡出
    const bool active = Absf(scroll.x - scroll_target.x) > 0.5f || Absf(scroll.y - scroll_target.y) > 0.5f;
    const float target_alpha = scroll_bar_auto_hide ? (active ? 1.0f : 0.18f) : 1.0f;
    scroll_bar_alpha_ = SmoothTo(scroll_bar_alpha_, target_alpha, 6.0f, dt);
}

void Widget::EnsureRectVisible(const Rect& target_rect) {
    if (overflow != Overflow::Scroll || !scroll_enabled) {
        return;
    }
    const float margin = 10.0f;
    ImVec2 next = scroll_target;
    const ImVec2 view_min = content_rect.min;
    const ImVec2 view_max = content_rect.max;

    if (target_rect.max.y > view_max.y - margin) {
        next.y = scroll.y + (target_rect.max.y - (view_max.y - margin));
    } else if (target_rect.min.y < view_min.y + margin) {
        next.y = scroll.y - ((view_min.y + margin) - target_rect.min.y);
    }

    if (target_rect.max.x > view_max.x - margin) {
        next.x = scroll.x + (target_rect.max.x - (view_max.x - margin));
    } else if (target_rect.min.x < view_min.x + margin) {
        next.x = scroll.x - ((view_min.x + margin) - target_rect.min.x);
    }

    scroll_target = ImVec2(Clampf(next.x, 0.0f, scroll_max.x), Clampf(next.y, 0.0f, scroll_max.y));
    if (scroll_snap) {
        const float page = Maxf(view_max.x - view_min.x, 1.0f);
        scroll_target.x = Clampf(std::round(scroll_target.x / page) * page, 0.0f, scroll_max.x);
    }
}

bool Widget::EnsureVisible(Widget* target) {
    if (target == nullptr || target == this) {
        return target == this;
    }
    Widget* branch = nullptr;
    for (auto& child : children) {
        if (child.get() == target || child->ContainsDescendant(target)) {
            branch = child.get();
            break;
        }
    }
    if (branch == nullptr) {
        return false;
    }
    branch->EnsureVisible(target);
    if (overflow == Overflow::Scroll && scroll_enabled) {
        EnsureRectVisible(branch->rect);
    }
    return true;
}

void Widget::ScrollPage(int direction, float scale) {
    Widget* host = ScrollHost();
    if (host == nullptr || direction == 0) {
        return;
    }
    const float page_y = (host->content_rect.Height() - 16.0f) * scale;
    const float page_x = (host->content_rect.Width() - 16.0f) * scale;
    host->scroll_target.y = Clampf(host->scroll_target.y + static_cast<float>(direction) * page_y, 0.0f,
                                   host->scroll_max.y);
    host->scroll_target.x = Clampf(host->scroll_target.x + static_cast<float>(direction) * page_x, 0.0f,
                                   host->scroll_max.x);
    if (host->scroll_overscroll) {
        // 顶到边界时给一点越界冲量，让回弹可见
        if (host->scroll_max.y > 0.5f) {
            if (direction > 0 && host->scroll_target.y >= host->scroll_max.y - 0.5f) {
                host->scroll.y = Minf(host->scroll_max.y + 32.0f, host->scroll.y + 32.0f);
            } else if (direction < 0 && host->scroll_target.y <= 0.5f) {
                host->scroll.y = Maxf(-32.0f, host->scroll.y - 32.0f);
            }
        }
    }
}

// ---------------------------------------------------------------- 交互 ----

Widget* Widget::HitTest(const ImVec2& p) {
    if (!visible || !enabled) {
        return nullptr;
    }
    // 子节点优先：按 z_order 从高到低测试
    std::vector<Widget*> ordered;
    ordered.reserve(children.size());
    for (auto& child : children) {
        ordered.push_back(child.get());
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Widget* a, const Widget* b) { return a->z_order > b->z_order; });
    for (Widget* child : ordered) {
        if (Widget* hit = child->HitTest(p)) {
            return hit;
        }
    }
    if (!interactive) {
        return nullptr;
    }
    return rect.Contains(p) ? this : nullptr;
}

void Widget::UpdateInteraction(float dt) {
    const bool is_hovered = (Global::hovered == this);
    if (is_hovered != hovered) {
        hovered = is_hovered;
        if (hovered) {
            emit hoverEntered();
        } else {
            emit hoverLeft();
        }
    }
    const bool was_focused = focused;
    focused = (Global::focused == this);
    if (focused != was_focused) {
        if (focused) {
            emit focusIn();
        } else {
            emit focusOut();
        }
    }

    // 焦点动画：Focus 是核心状态，缩放/位移/焦点框都从 focus_mix 推出来
    const float focus_target = (focused && enabled) ? 1.0f : 0.0f;
    focus_mix = SmoothTo(focus_mix, focus_target, focus_animation_speed, dt);

    if (hovered && focus_on_hover && focusable && enabled && Global::focused != this) {
        RequestFocus();
        focused = true;
    }

    if (hovered && enabled && Global::mouse_pressed[0]) {
        Global::pressed = this;
        Global::active = this;
        emitWidgetPressed();
    }
    down = (Global::pressed == this) && Global::mouse_down[0];

    if (Global::pressed == this && Global::mouse_released[0]) {
        Global::pressed = nullptr;
        Global::active = nullptr;
        down = false;
        if (hovered && enabled) {
            if (!OnPadAction(InputAction::Confirm)) {
                Activate();
                emit clicked();
            }
        }
        emit released();
    }

    // 手柄按键：只有持有焦点的组件才收键；Confirm 先给子类消费，没人要才当点击。
    if (focused && enabled) {
        static const InputAction kDispatched[] = {
            InputAction::Left,       InputAction::Right,       InputAction::Up,        InputAction::Down,
            InputAction::ActionX,    InputAction::ActionY,     InputAction::Menu,      InputAction::Minus,
            InputAction::PageLeft,   InputAction::PageRight,   InputAction::TriggerLeft, InputAction::TriggerRight,
        };
        for (InputAction action : kDispatched) {
            if (Global::pad.Pressed(action) && Global::Available(action)) {
                OnPadAction(action);
                Global::MarkConsumed(action);
            }
        }
        if (Global::pad.Pressed(InputAction::Confirm) && Global::Available(InputAction::Confirm)) {
            if (!OnPadAction(InputAction::Confirm)) {
                Activate();
                emit clicked();
            }
            Global::MarkConsumed(InputAction::Confirm);
        }
        // B 键：控件可以自己消费（关闭弹层/取消编辑），没人消费就交回页面
        if (Global::pad.Pressed(InputAction::Cancel) && Global::Available(InputAction::Cancel)) {
            if (OnPadAction(InputAction::Cancel)) {
                Global::MarkConsumed(InputAction::Cancel);
            }
        }
    }
}

void Widget::UpdateTree(float dt) {
    UpdateScroll(dt);
    UpdateInteraction(dt);
    for (auto& child : children) {
        if (child->visible) {
            child->UpdateTree(dt);
        }
    }
    if (visible) {
        OnUpdate(dt);
    }
}

void Widget::emitWidgetPressed() {
    emit pressed();
}

void Widget::RequestFocus() {
    if (focusable && enabled) {
        Global::SetFocus(this);
    }
}

void Widget::YieldFocus() {
    if (Global::focused == this) {
        Global::SetFocus(nullptr);
    }
}

Widget* Widget::FirstFocusable() {
    if (focusable && enabled && visible) {
        return this;
    }
    for (auto& child : children) {
        if (child->visible && child->enabled) {
            if (Widget* found = child->FirstFocusable()) {
                return found;
            }
        }
    }
    return nullptr;
}

void Widget::SetFocusZone(int zone) {
    focus_zone = zone;
    for (auto& child : children) {
        child->SetFocusZone(zone);
    }
}

void Widget::CollectFocusables(std::vector<Widget*>& out) {
    if (!visible || !enabled) {
        return;
    }
    if (focusable) {
        out.push_back(this);
    }
    // 复合控件只把自己当焦点停靠点（内部导航由它自己处理）；
    // 容器语义下继续往下收集子节点。
    if (focus_only_self) {
        return;
    }
    for (auto& child : children) {
        child->CollectFocusables(out);
    }
}

// ---------------------------------------------------------------- 绘制 ----

float Widget::EffectiveOpacity() const {
    return enabled ? opacity : opacity * disabled_opacity;
}

ImU32 Widget::Tint(ImU32 color) const {
    return Theme::Alpha(color, EffectiveOpacity());
}

void Widget::DrawBackground(ImDrawList* dl) {
    const float radius_scale = draw_transform_.AverageScale();
    const float tl = CornerTL() * radius_scale;
    const float tr = CornerTR() * radius_scale;
    const float bl = CornerBL() * radius_scale;
    const float br = CornerBR() * radius_scale;

    if (shadow.enabled) {
        ShadowStyle scaled = shadow;
        scaled.blur *= radius_scale;
        scaled.offset = ImVec2(shadow.offset.x * radius_scale, shadow.offset.y * radius_scale);
        Draw::SoftShadow(dl, draw_rect_, scaled, tl, tr, bl, br);
    }
    if (((background >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
        Draw::RoundedRectFilled(dl, draw_rect_, Tint(background), tl, tr, bl, br);
    }
    if (border.Visible()) {
        const Rect outline = draw_rect_.Expanded(-border.inset * radius_scale);
        Draw::RoundedRectOutline(dl, outline, Tint(border.color), border.width * radius_scale, tl, tr, bl, br);
    }
}

void Widget::DrawFocusFrame(ImDrawList* dl) {
    if (!focus_frame || focus_mix <= 0.02f) {
        return;
    }
    const float scale = draw_transform_.AverageScale();
    const float offset = focus_frame_offset * scale * (0.6f + 0.4f * focus_mix);
    const Rect ring = draw_rect_.Expanded(offset);
    const float radius = (Maxf(Maxf(CornerTL(), CornerTR()), Maxf(CornerBL(), CornerBR())) * scale) + offset;
    Draw::RoundedRectOutline(dl, ring, Theme::Alpha(focus_frame_color, focus_mix), focus_frame_width * scale, radius,
                             radius, radius, radius);
}

void Widget::DrawScrollBar(ImDrawList* dl) {
    if (!scroll_bar || overflow != Overflow::Scroll || !scroll_enabled) {
        return;
    }
    if (scroll_bar_alpha_ <= 0.02f) {
        return;
    }
    const float thickness = scroll_bar_thickness;
    const float inset = 4.0f;

    if (scroll_max.y > 0.5f && draw_rect_.Height() > 1.0f) {
        const float view_h = draw_rect_.Height();
        const float total_h = view_h + scroll_max.y;
        const float ratio = Clampf(view_h / total_h, 0.08f, 1.0f);
        const float bar_h = view_h * ratio;
        const float travel = view_h - bar_h;
        const float progress = scroll_max.y > 0.0f ? Clampf(scroll.y / scroll_max.y, 0.0f, 1.0f) : 0.0f;
        const float x = draw_rect_.max.x - inset - thickness;
        const Rect bar = Rect::FromPosSize(ImVec2(x, draw_rect_.min.y + inset + travel * progress),
                                          ImVec2(thickness, bar_h));
        Draw::RoundedRectFilled(dl, bar, Theme::U32(Theme::Alpha(Theme::kTextMuted, scroll_bar_alpha_ * 0.9f)), thickness * 0.5f,
                                thickness * 0.5f, thickness * 0.5f, thickness * 0.5f);
    }
    if (scroll_max.x > 0.5f && draw_rect_.Width() > 1.0f) {
        const float view_w = draw_rect_.Width();
        const float total_w = view_w + scroll_max.x;
        const float ratio = Clampf(view_w / total_w, 0.08f, 1.0f);
        const float bar_w = view_w * ratio;
        const float travel = view_w - bar_w;
        const float progress = scroll_max.x > 0.0f ? Clampf(scroll.x / scroll_max.x, 0.0f, 1.0f) : 0.0f;
        const float y = draw_rect_.max.y - inset - thickness;
        const Rect bar = Rect::FromPosSize(ImVec2(draw_rect_.min.x + inset + travel * progress, y),
                                          ImVec2(bar_w, thickness));
        Draw::RoundedRectFilled(dl, bar, Theme::U32(Theme::Alpha(Theme::kTextMuted, scroll_bar_alpha_ * 0.9f)), thickness * 0.5f,
                                thickness * 0.5f, thickness * 0.5f, thickness * 0.5f);
    }
}

void Widget::DrawChildren(ImDrawList* dl) {
    if (children.empty()) {
        return;
    }
    std::vector<Widget*> ordered;
    ordered.reserve(children.size());
    for (auto& child : children) {
        if (child->visible) {
            ordered.push_back(child.get());
        }
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Widget* a, const Widget* b) { return a->z_order < b->z_order; });
    for (Widget* child : ordered) {
        child->DrawTree(dl, draw_transform_);
    }
}

void Widget::DrawTree(ImDrawList* dl) {
    DrawTree(dl, Transform2D{});
}

void Widget::DrawTree(ImDrawList* dl, const Transform2D& parent_transform) {
    if (!visible || EffectiveOpacity() <= 0.002f) {
        return;
    }

    // 自身视觉变换 = 即时变换(按压) ∘ 焦点动画(缩放 + 位移)，再叠加父节点的变换
    const float focus_scale_now = 1.0f + (focus_scale - 1.0f) * focus_mix;
    const float scale = visual_scale.x * focus_scale_now;
    const ImVec2 translate(visual_translate.x + focus_translate.x * focus_mix,
                           visual_translate.y + focus_translate.y * focus_mix);
    const Transform2D local = Transform2D::ScaleAbout(rect.Center(), scale, translate);
    draw_transform_ = local.Then(parent_transform);
    draw_rect_ = draw_transform_.Apply(rect);

    DrawBackground(dl);
    const bool clip = overflow != Overflow::Visible;
    if (clip) {
        dl->PushClipRect(draw_rect_.min, draw_rect_.max, true);
    }
    OnDrawContent(dl, draw_transform_.Apply(content_rect));
    DrawChildren(dl);
    OnDrawOverlay(dl, draw_transform_.Apply(content_rect));
    DrawFocusFrame(dl);
    if (clip) {
        dl->PopClipRect();
    }
    DrawScrollBar(dl);
}

// ------------------------------------------------------------ 子类默认 ----

ImVec2 Widget::MeasureContent(const ImVec2& available) {
    (void)available;
    return ImVec2(0.0f, 0.0f);
}

void Widget::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)dl;
    (void)content;
}

void Widget::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    (void)dl;
    (void)content;
}

void Widget::OnUpdate(float dt) {
    (void)dt;
}

void Widget::Activate() {}

bool Widget::OnPadAction(InputAction action) {
    (void)action;
    return false;
}

} // namespace gui_dev::cv
