#include "gamemenu/views/MainMenuView.h"

#include <imgui.h>

namespace gui_dev::gamemenu {
namespace {

struct ItemDef {
    const char* label;
    Icons::Material icon;
    MenuButtonKind kind;
    bool group_break; // 该项之前画一条不规则分隔线
};

// 分组：高频操作 / 配置 / 危险操作（需求 §14）。用视觉分隔，不写分组文字。
constexpr ItemDef kItems[MainMenuView::kItemCount] = {
    {"返回游戏", Icons::Material::Play, MenuButtonKind::Normal, false},
    {"保存状态", Icons::Material::Save, MenuButtonKind::Normal, false},
    {"读取状态", Icons::Material::Restore, MenuButtonKind::Normal, false},
    {"独立设置", Icons::Material::Settings, MenuButtonKind::Normal, true},
    {"全局设置", Icons::Material::Memory, MenuButtonKind::Normal, false},
    {"重置游戏", Icons::Material::Update, MenuButtonKind::Danger, true},
    {"退出游戏", Icons::Material::Close, MenuButtonKind::Danger, false},
};

} // namespace

MainMenuView::MainMenuView(MainMenuDelegate& delegate) : delegate_(delegate) {
    for (int i = 0; i < kItemCount; ++i) {
        items_[i].Configure(kItems[i].label, kItems[i].icon, kItems[i].kind);
    }
}

void MainMenuView::OnEnter(GameMenuContext& ctx) {
    (void)ctx;
    // 每次进入（含从子页返回）都清干净，避免动画残留（需求 §28）
    for (GameMenuButton& item : items_) {
        item.Reset();
    }
    focus_frame_.Reset();
}

void MainMenuView::Activate(GameMenuContext& ctx, int index) {
    switch (index) {
    case 0:
        delegate_.OnResume(ctx);
        ctx.host->CloseMenu();
        break;
    case 1:
        delegate_.OnOpenStateSlots(ctx, true);
        break;
    case 2:
        delegate_.OnOpenStateSlots(ctx, false);
        break;
    case 3:
        delegate_.OnOpenSettings(ctx, true);
        break;
    case 4:
        delegate_.OnOpenSettings(ctx, false);
        break;
    case 5:
        delegate_.OnRequestReset(ctx);
        break;
    case 6:
        delegate_.OnRequestExit(ctx);
        break;
    default:
        break;
    }
}

void MainMenuView::Update(GameMenuContext& ctx) {
    // 鼠标只作为辅助（需求 §29）：手柄/键盘永远是主路径
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        for (int i = 0; i < kItemCount; ++i) {
            if (item_rects_[i].Contains(mouse)) {
                Activate(ctx, i);
                return;
            }
        }
    }
    for (int i = 0; i < kItemCount; ++i) {
        bool focused = i == focus_;
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            item_rects_[i].Contains(ImGui::GetMousePos())) {
            focus_ = i; // 悬停把焦点吸过去
            focused = true;
        }
        items_[i].Update(ctx.dt, focused, ctx.theme->animation);
    }
    focus_frame_.Update(ctx.dt, item_rects_[focus_], ctx.theme->animation);
}

bool MainMenuView::OnAction(GameMenuContext& ctx, InputAction action) {
    switch (action) {
    case InputAction::Up:
        focus_ = (focus_ + kItemCount - 1) % kItemCount;
        return true;
    case InputAction::Down:
        focus_ = (focus_ + 1) % kItemCount;
        return true;
    case InputAction::Confirm:
        // 按压反馈先播，动作立即执行（动画不阻塞操作，需求 §11）
        items_[focus_].TriggerPress();
        Activate(ctx, focus_);
        return true;
    default:
        return false;
    }
}

void MainMenuView::Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) {
    const GameMenuTheme& theme = *ctx.theme;
    const MenuAnimationConfig& cfg = theme.animation;
    const float row_width = area.Width() - theme.focus_offset - 6.0f;

    float y = area.min.y + 6.0f;
    Rect focus_target = area;

    for (int i = 0; i < kItemCount; ++i) {
        if (kItems[i].group_break) {
            y += theme.group_gap * 0.5f;
            AddJaggedRule(draw_list, area.min.x + 8.0f, area.max.x - 14.0f, y, 2.6f, 9.0f,
                          ColorWithAlpha(theme.white, 0.30f));
            y += theme.group_gap * 0.5f;
        }

        // 逐项出现：延迟进入 + 从右滑入 + 淡入（需求 §12）
        const float stagger = EaseOutCubic(StaggerProgress(ctx.open_progress, i, cfg));
        const float dx = (1.0f - stagger) * 38.0f;
        const GameMenuTheme item_theme = ThemeWithAlpha(theme, stagger);

        const Rect rect = items_[i].Draw(draw_list, item_theme,
                                         ImVec2(area.min.x + dx, y), row_width, theme.row_height,
                                         ctx.time);
        item_rects_[i] = rect;
        if (i == focus_) {
            focus_target = rect;
        }
        y += theme.row_height + theme.row_gap;
    }

    focus_frame_.Draw(draw_list, theme, EaseOutCubic(ctx.open_progress));
}

} // namespace gui_dev::gamemenu
