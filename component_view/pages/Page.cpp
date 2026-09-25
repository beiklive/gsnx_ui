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

    // 根节点铺满画布
    root_->position = ImVec2(0.0f, 0.0f);
    root_->size = Global::canvas_size;
    root_->LayoutTree(Global::canvas_pos, Global::canvas_size);

    focusables_.clear();
    root_->CollectFocusables(focusables_);

    // 鼠标/触摸命中（控件在 UpdateTree 里读 Global::hovered）
    Global::hovered = Global::mouse_available ? root_->HitTest(Global::mouse) : nullptr;

    // 手柄/键盘焦点导航（自己消费方向键的控件会被跳过）
    Global::NavigateFocus(focusables_);

    root_->UpdateTree(dt);
    OnInput();
    OnUpdate(dt);
}

void Page::Render() {
    ImDrawList* dl = Global::draw_list;
    if (dl == nullptr) {
        return;
    }
    const Rect canvas = Global::CanvasRect();
    dl->AddRectFilled(canvas.min, canvas.max, Theme::U32(Theme::kBgEditor)); // VSCode 底色，不用纯黑

    if (root_ != nullptr) {
        root_->DrawTree(dl);
    }
    OnOverlay(dl);
}

} // namespace gui_dev::cv
