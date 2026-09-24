#include "component_view/pages/Page.h"

#include <cstdio>
#include <string>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {

Page::Page() = default;

Page::~Page() = default;

Box& Page::Overlay() {
    if (overlay_ == nullptr) {
        overlay_ = std::make_unique<Box>("page_overlay");
        overlay_->background = 0;
        overlay_->padding = EdgeInsets{};
        overlay_->layout = LayoutMode::Free;
        overlay_->interactive = false;
        overlay_->size = Global::canvas_size;
        overlay_->visible = false;
    }
    return *overlay_;
}

Box& Page::Root() {
    if (root_ == nullptr) {
        root_ = std::make_unique<Box>("page_root");
        root_->background = 0; // 背景由 Page::Render 统一铺，避免每页重复设置
        root_->padding = EdgeInsets::All(24.0f);
        root_->layout = LayoutMode::Free;
    }
    return *root_;
}

void Page::AddHint(Icons::Button button, std::string label) {
    hints_.emplace_back(button, std::move(label));
}

void Page::SetPageInfo(int index, int total) {
    page_index_ = index;
    page_total_ = total;
}

void Page::Update(float dt) {
    if (!built_) {
        built_ = true;
        OnBuild();
    }
    if (root_ == nullptr || ui_ == nullptr) {
        return;
    }

    // 根节点铺满画布，子节点用相对坐标摆放。
    root_->position = ImVec2(0.0f, 0.0f);
    root_->size = Global::canvas_size;
    root_->LayoutTree(Global::canvas_pos, Global::canvas_size);

    // 弹层始终参与布局：即使当前不可见，也要保证第一次显形时矩形是新鲜的
    if (overlay_ != nullptr) {
        overlay_->position = ImVec2(0.0f, 0.0f);
        overlay_->size = Global::canvas_size;
        overlay_->LayoutTree(Global::canvas_pos, Global::canvas_size);
    }

    focusables_.clear();
    if (Global::modal != nullptr) {
        // 模态打开时焦点只在这个子树里流动（Focus Trap）
        Global::modal->CollectFocusables(focusables_);
    } else {
        root_->CollectFocusables(focusables_);
    }

    // 鼠标命中（在控件更新之前，控件自己读 Global::hovered）
    Global::hovered = Global::mouse_available ? root_->HitTest(Global::mouse) : nullptr;
    if (Global::hovered != nullptr && Global::modal != nullptr && Global::modal != Global::hovered &&
        !Global::modal->ContainsDescendant(Global::hovered)) {
        Global::hovered = nullptr; // 模态之外不接受鼠标
    }

    // 焦点导航：自己消费方向键的控件（列表/滑条/键盘）会跳过
    Global::NavigateFocus(focusables_);

    // 焦点自动滚动：把聚焦组件滚进最近的滚动容器
    if (Global::focused != nullptr) {
        if (Widget* host = Global::focused->ScrollHost()) {
            host->EnsureVisible(Global::focused);
        }
    }

    // 控件自己处理按键（A/B/X/Y/方向/扳机……），处理过的按键会被消费
    root_->UpdateTree(dt);
    if (overlay_ != nullptr && overlay_->visible) {
        // 可见性可能就在这一帧被控件改掉（打开弹窗/键盘），重新布局一次再更新
        overlay_->LayoutTree(Global::canvas_pos, Global::canvas_size);
        overlay_->UpdateTree(dt);
    }

    // 页面级兜底输入：只处理控件都没消费的按键
    OnInput();
    OnUpdate(dt);
}

void Page::Render() {
    ImDrawList* dl = Global::draw_list;
    if (dl == nullptr) {
        return;
    }
    const Rect canvas = Global::CanvasRect();
    // VSCode 编辑器底色（#1E1E1E，不用纯黑）
    dl->AddRectFilled(canvas.min, canvas.max, Theme::kBgEditor);

    if (root_ != nullptr) {
        root_->DrawTree(dl);
    }
    if (show_hud_) {
        DrawHud(dl);
    }
    if (overlay_ != nullptr && overlay_->visible) {
        overlay_->DrawTree(dl);
    }
    OnOverlay(dl);
}

void Page::DrawHud(ImDrawList* dl) {
    const Rect canvas = Global::CanvasRect();
    const float bar_height = 46.0f;
    const Rect bar =
        Rect::FromPosSize(ImVec2(canvas.min.x, canvas.max.y - bar_height), ImVec2(canvas.Width(), bar_height));

    Draw::RoundedRectFilled(dl, bar, Theme::Alpha(Theme::kBgSideBar, 0.97f), 0.0f, 0.0f, 0.0f, 0.0f);
    dl->AddLine(ImVec2(bar.min.x, bar.min.y), ImVec2(bar.max.x, bar.min.y), Theme::kBorder, 1.0f);

    // 右侧信息先算好宽度，左侧提示区不能压到它
    char info[192];
    if (page_total_ > 0) {
        std::snprintf(info, sizeof(info), "%s · %s · %dx%d · %.0f FPS · %d/%d", Title(), Global::platform_name,
                      static_cast<int>(Global::canvas_size.x), static_cast<int>(Global::canvas_size.y),
                      ImGui::GetIO().Framerate, page_index_ + 1, page_total_);
    } else {
        std::snprintf(info, sizeof(info), "%s · %s · %dx%d · %.0f FPS", Title(), Global::platform_name,
                      static_cast<int>(Global::canvas_size.x), static_cast<int>(Global::canvas_size.y),
                      ImGui::GetIO().Framerate);
    }
    const ImVec2 info_extent = Draw::MeasureText(nullptr, Theme::kFontSmall, info, 0.0f);
    const float hint_limit = bar.max.x - 28.0f - info_extent.x - 24.0f;

    float cursor_x = bar.min.x + 28.0f;
    for (const auto& hint : hints_) {
        if (cursor_x > hint_limit) {
            break;
        }
        const char* glyph = Icons::Glyph(hint.first);
        const float glyph_size = 20.0f;
        const ImVec2 glyph_extent = Draw::MeasureText(nullptr, glyph_size, glyph, 0.0f);
        Draw::Text(dl, nullptr, glyph_size, ImVec2(cursor_x, bar.Center().y - glyph_extent.y * 0.5f),
                   Theme::kTextPrimary, glyph);
        cursor_x += glyph_extent.x + 8.0f;

        const float label_size = Theme::kFontSmall;
        const ImVec2 label_extent = Draw::MeasureText(nullptr, label_size, hint.second.c_str(), 0.0f);
        Draw::Text(dl, nullptr, label_size, ImVec2(cursor_x, bar.Center().y - label_extent.y * 0.5f),
                   Theme::kTextMuted, hint.second.c_str());
        cursor_x += label_extent.x + 28.0f;
    }

    Draw::Text(dl, nullptr, Theme::kFontSmall,
               ImVec2(bar.max.x - 28.0f - info_extent.x, bar.Center().y - info_extent.y * 0.5f), Theme::kTextMuted,
               info);
}

} // namespace gui_dev::cv
