#include "component_view/pages/Page.h"

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {

Page::Page() = default;
Page::~Page() = default;

Box& Page::Root() {
    if (root_ == nullptr) {
        root_ = std::make_unique<Box>("page_root");
        root_->background = 0;                  // 背景由 Page::Render 统一铺
        root_->background_follows_theme = false; // 透明根节点不跟着主题铺底
        root_->border.width = 0.0f;             // 整页容器不画边框
        root_->shadow.enabled = false;          // 阴影会铺满整块矩形，透明根节点会把页面压暗
        root_->corner_radius = 0.0f;
        root_->padding = EdgeInsets{};          // 不留边距：(0,0) 就是页面左上角
        root_->layout = LayoutMode::Free;       // 子节点用 position 自己定位
    }
    return *root_;
}

void Page::RefreshTheme() {
    Global::ApplyTheme();
    popups_.RefreshTheme(); // 弹窗自己有独立的树，主题刷新要单独走一遍
    if (root_ != nullptr) {
        root_->RefreshThemeTree();
        // 注意顺序：RefreshThemeTree() 会走到根节点的 Box::OnThemeChanged() →
        // applyComponentStyle() 又把边框/阴影打开，所以根节点的「不画装饰」要最后复位。
        root_->background = 0;
        root_->background_follows_theme = false;
        root_->border.width = 0.0f;
        root_->shadow.enabled = false;
    }
}

void Page::Update(float dt) {
    if (!built_) {
        built_ = true;
        OnBuild();
    }
    if (root_ == nullptr || ui_ == nullptr) {
        return;
    }

    // 焦点作用域交给本页：弹窗打开后只有弹窗内部的控件可聚焦（Focus Trap）
    Global::focus_manager = &focus_;

    // 根节点铺满画布
    root_->position = ImVec2(0.0f, 0.0f);
    root_->size = Global::canvas_size;
    root_->LayoutTree(Global::canvas_pos, Global::canvas_size);

    // 弹窗布局（栈内每一层都摆好）：画在页面之上，见 Render 的层级顺序
    popups_.Layout();

    focusables_.clear();
    root_->CollectFocusables(focusables_);

    // ---- 输入优先级：Popup → 普通 UI ------------------------------------
    // 模态弹窗打开时，页面拿不到 hover/点击（命中测试在弹窗这一层就结束了）
    Widget* hit = popups_.HitTest(Global::mouse);
    if (hit == nullptr && !popups_.BlocksBackground()) {
        hit = root_->HitTest(Global::mouse);
    }
    Global::hovered = Global::mouse_available ? hit : nullptr;

    // 触摸/鼠标拖动滚动：从命中控件向上寻找最近的滚动容器，超过阈值后才开始
    // 改变 scroll_target。阈值以内仍然是普通点击，避免轻微抖动误触。
    if (Global::mouse_pressed[0]) {
        Global::pointer_drag_host = Global::hovered != nullptr ? Global::hovered->ScrollHost() : nullptr;
    }
    if (Global::mouse_down[0] && Global::pointer_drag_host != nullptr && Global::mouse_available) {
        const ImVec2 from_start(Global::mouse.x - Global::pointer_drag_origin.x,
                                Global::mouse.y - Global::pointer_drag_origin.y);
        const float distance_sq = from_start.x * from_start.x + from_start.y * from_start.y;
        constexpr float kDragThreshold = 8.0f;
        if (!Global::pointer_dragging && distance_sq >= kDragThreshold * kDragThreshold) {
            Global::pointer_dragging = true;
        }
        if (Global::pointer_dragging) {
            Widget* host = Global::pointer_drag_host;
            host->scroll_target.x = Clampf(host->scroll_target.x - Global::mouse_delta.x, 0.0f, host->scroll_max.x);
            host->scroll_target.y = Clampf(host->scroll_target.y - Global::mouse_delta.y, 0.0f, host->scroll_max.y);
        }
    }

    // ---- 焦点层级：手上有弹窗时只有弹窗内的控件参与方向键导航 ----------------
    nav_focusables_.clear();
    popups_.CollectFocusables(focusables_, nav_focusables_);
    Global::NavigateFocus(nav_focusables_);

    root_->UpdateTree(dt);
    popups_.UpdateTree(dt); // 只有栈顶弹窗收输入

    // 释放帧要让 Widget::UpdateInteraction 先看到 pointer_dragging，以便取消点击，
    // 然后再结束本次手势。
    if (Global::mouse_released[0]) {
        Global::pointer_drag_host = nullptr;
        Global::pointer_dragging = false;
    }

    // 关闭请求（B 键 / 点遮罩）：必须在控件拿到按键之后，否则会抢掉弹窗内控件的 B 键
    popups_.HandleDismiss();

    // 焦点自动滚动：焦点变了就把它滚进所在的滚动容器（面板/列表/弹窗富文本都能用）。
    if (Global::focused != last_focused_) {
        if (Global::focused != nullptr) {
            if (!root_->EnsureVisible(Global::focused)) {
                popups_.EnsureVisible(Global::focused);
            }
        }
        last_focused_ = Global::focused;
    }

    OnInput();
    OnUpdate(dt);

    // Toast：生命周期 + 动画每帧推进（不参与焦点/输入）
    toasts_.Update(dt);
    // 弹窗生命周期最后推进：关闭动画播完才真正移除，同时弹出焦点作用域并恢复焦点
    popups_.Advance(dt);
}

void Page::Render() {
    ImDrawList* dl = Global::draw_list;
    if (dl == nullptr) {
        return;
    }
    const Rect canvas = Global::CanvasRect();

    // 渲染层级严格按 UILayer 从下往上画（见 component_view/UILayer.h）：
    //   Background(0) → Content(1000) → Popup(5000~9000) → Focus(9900) → Toast(9999)
    // 顺序在这里集中维护：控件内部不要自己决定谁在上面。

    // ---- UILayer::Background ----
    dl->AddRectFilled(canvas.min, canvas.max, Theme::U32(Theme::kBgEditor)); // VSCode 底色，不用纯黑

    // ---- UILayer::Content（普通 UI）----
    if (root_ != nullptr) {
        root_->DrawTree(dl);
    }
    OnOverlay(dl);

    // ---- UILayer::Popup（模态弹窗，栈底 → 栈顶）----
    popups_.Draw(dl);

    // 下面两层画到 **前景** draw list：UI 主体在背景层，ImGui 窗口（含 imgui_markdown
    // 的排版子窗口）夹在两者之间，所以焦点框与 Toast 仍然是最高层。
    ImDrawList* overlay = ImGui::GetForegroundDrawList();

    // ---- UILayer::Focus（焦点框 Overlay）----
    // 画在弹窗之上：弹窗里的按钮有焦点时焦点框不会被弹窗盒子压住（需求 §5/§13）。
    // 焦点框只是绘制，不参与命中测试，所以不会挡鼠标/触摸。
    focus_ring_.Draw(overlay, Global::focused);

    // ---- UILayer::Toast（全局通知，视觉最顶层，默认不拦输入）----
    toasts_.Draw(overlay);
}

} // namespace gui_dev::cv
