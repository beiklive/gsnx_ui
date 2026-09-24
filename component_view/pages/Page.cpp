#include "component_view/pages/Page.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {

Page::Page() = default;

Page::~Page() = default;

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

    focusables_.clear();
    root_->CollectFocusables(focusables_);
    OnInput();
    Global::NavigateFocus(focusables_);

    Global::hovered = Global::mouse_available ? root_->HitTest(Global::mouse) : nullptr;
    root_->UpdateTree(dt);
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
    OnOverlay(dl);
}

void Page::DrawHud(ImDrawList* dl) {
    const Rect canvas = Global::CanvasRect();
    const float bar_height = 46.0f;
    const Rect bar =
        Rect::FromPosSize(ImVec2(canvas.min.x, canvas.max.y - bar_height), ImVec2(canvas.Width(), bar_height));

    Draw::RoundedRectFilled(dl, bar, Theme::Alpha(Theme::kBgSideBar, 0.97f), 0.0f, 0.0f, 0.0f, 0.0f);
    dl->AddLine(ImVec2(bar.min.x, bar.min.y), ImVec2(bar.max.x, bar.min.y), Theme::kBorder, 1.0f);

    float cursor_x = bar.min.x + 28.0f;
    for (const auto& hint : hints_) {
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
    Draw::Text(dl, nullptr, Theme::kFontSmall,
               ImVec2(bar.max.x - 28.0f - info_extent.x, bar.Center().y - info_extent.y * 0.5f), Theme::kTextMuted,
               info);
}

} // namespace gui_dev::cv
