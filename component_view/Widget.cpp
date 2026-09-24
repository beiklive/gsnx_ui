#include "component_view/Widget.h"

#include <algorithm>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {

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
            total.x = Maxf(total.x, child->position.x + child->margin.left + child->measured_size.x + child->margin.right);
            total.y = Maxf(total.y, child->position.y + child->margin.top + child->measured_size.y + child->margin.bottom);
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
}

void Widget::PlaceChildren() {
    if (children.empty()) {
        return;
    }
    const ImVec2 origin = content_rect.min;
    const ImVec2 area(content_rect.Width(), content_rect.Height());
    ImVec2 cursor(0.0f, 0.0f);

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
        } else {
            child->Measure(area);
            child->Place(origin, area);
        }
    }

    // 主轴对齐：排列完成后内容没占满时，整体平移子节点（Start/Center/End）。
    if (layout == LayoutMode::Vertical && align_y != Align::Start && !children.empty()) {
        const float used = Maxf(cursor.y - gap.y, 0.0f);
        const float slack = area.y - used;
        if (slack > 0.0f) {
            const float delta = align_y == Align::Center ? slack * 0.5f : slack;
            for (auto& child : children) {
                child->Move(ImVec2(0.0f, delta));
            }
        }
    } else if (layout == LayoutMode::Horizontal && align_x != Align::Start && !children.empty()) {
        const float used = Maxf(cursor.x - gap.x, 0.0f);
        const float slack = area.x - used;
        if (slack > 0.0f) {
            const float delta = align_x == Align::Center ? slack * 0.5f : slack;
            for (auto& child : children) {
                child->Move(ImVec2(delta, 0.0f));
            }
        }
    }
}

void Widget::Move(const ImVec2& delta) {
    rect = rect.Translate(delta);
    margin_rect = margin_rect.Translate(delta);
    content_rect = content_rect.Translate(delta);
    for (auto& child : children) {
        child->Move(delta);
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

void Widget::UpdateInteraction() {
    const bool is_hovered = (Global::hovered == this);
    if (is_hovered != hovered) {
        hovered = is_hovered;
        if (hovered) {
            if (on_hover_enter) {
                on_hover_enter(*this);
            }
        } else if (on_hover_leave) {
            on_hover_leave(*this);
        }
    }
    focused = (Global::focused == this);
    clicked = false;

    if (hovered && focus_on_hover && focusable && enabled && Global::focused != this) {
        RequestFocus();
        focused = true;
    }

    if (hovered && enabled && Global::mouse_pressed[0]) {
        Global::pressed = this;
        Global::active = this;
        if (on_press) {
            on_press(*this);
        }
    }
    pressed = (Global::pressed == this) && Global::mouse_down[0];

    if (Global::pressed == this && Global::mouse_released[0]) {
        Global::pressed = nullptr;
        Global::active = nullptr;
        pressed = false;
        if (hovered && enabled) {
            clicked = true;
            Activate();
            if (on_click) {
                on_click(*this);
            }
        }
    }

    if (focusable && focused && enabled && Global::pad.Pressed(InputAction::Confirm)) {
        clicked = true;
        Activate();
        if (on_click) {
            on_click(*this);
        }
    }
}

void Widget::UpdateTree(float dt) {
    UpdateInteraction();
    for (auto& child : children) {
        if (child->visible) {
            child->UpdateTree(dt);
        }
    }
    if (visible) {
        OnUpdate(dt);
    }
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

void Widget::CollectFocusables(std::vector<Widget*>& out) {
    if (!visible || !enabled) {
        return;
    }
    if (focusable) {
        out.push_back(this);
    }
    for (auto& child : children) {
        child->CollectFocusables(out);
    }
}

// ---------------------------------------------------------------- 绘制 ----

void Widget::DrawBackground(ImDrawList* dl) {
    const float tl = CornerTL();
    const float tr = CornerTR();
    const float bl = CornerBL();
    const float br = CornerBR();

    if (shadow.enabled) {
        Draw::SoftShadow(dl, rect, shadow, tl, tr, bl, br);
    }
    if (((background >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
        Draw::RoundedRectFilled(dl, rect, Tint(background), tl, tr, bl, br);
    }
    if (border.Visible()) {
        const Rect outline = rect.Expanded(-border.inset);
        Draw::RoundedRectOutline(dl, outline, Tint(border.color), border.width, tl, tr, bl, br);
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
        child->DrawTree(dl);
    }
}

void Widget::DrawTree(ImDrawList* dl) {
    if (!visible || opacity <= 0.001f) {
        return;
    }
    DrawBackground(dl);
    if (clip_children) {
        dl->PushClipRect(rect.min, rect.max, true);
    }
    OnDrawContent(dl, content_rect);
    DrawChildren(dl);
    OnDrawOverlay(dl, content_rect);
    if (clip_children) {
        dl->PopClipRect();
    }
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

} // namespace gui_dev::cv
