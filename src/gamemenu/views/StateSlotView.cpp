#include "gamemenu/views/StateSlotView.h"

#include <cstdio>

#include <imgui.h>

namespace gui_dev::gamemenu {

StateSlotView::StateSlotView(StateSlotDelegate& delegate, bool saving)
    : delegate_(delegate), saving_(saving) {}

int StateSlotView::PageCount() const {
    const int total = delegate_.SlotCount();
    return total > 0 ? (total + kPerPage - 1) / kPerPage : 1;
}

void StateSlotView::RefreshSlots() {
    for (int i = 0; i < kPerPage; ++i) {
        const int slot = SlotIndex(i);
        if (slot < delegate_.SlotCount()) {
            delegate_.FillSlot(slot, slot_data_[i]);
        } else {
            slot_data_[i] = GameMenuSlotData{};
        }
    }
}

void StateSlotView::SetHint(const char* text) {
    std::snprintf(hint_text_, sizeof(hint_text_), "%s", text != nullptr ? text : "");
    hint_timer_ = 1.6f;
}

void StateSlotView::OnEnter(GameMenuContext& ctx) {
    (void)ctx;
    // 每次进入重置：槽位动画、焦点、翻页、提示（需求 §28）
    for (GameMenuSaveSlot& slot : slots_) {
        slot.Reset();
    }
    focus_frame_.Reset();
    focus_ = 0;
    page_ = 0;
    hint_timer_ = 0.0f;
    hint_text_[0] = '\0';
    RefreshSlots();
}

void StateSlotView::Update(GameMenuContext& ctx) {
    if (hint_timer_ > 0.0f) {
        hint_timer_ -= ctx.dt;
        if (hint_timer_ < 0.0f) {
            hint_timer_ = 0.0f;
        }
    }

    // 鼠标辅助
    const ImVec2 mouse = ImGui::GetMousePos();
    bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    for (int i = 0; i < kPerPage; ++i) {
        const bool hovered = slot_rects_[i].Contains(mouse);
        if (hovered) {
            focus_ = i;
        }
        if (hovered && clicked) {
            slots_[i].Update(ctx.dt, true, ctx.theme->animation);
            OnAction(ctx, InputAction::Confirm);
            clicked = false;
        } else {
            slots_[i].Update(ctx.dt, i == focus_, ctx.theme->animation);
        }
    }
    focus_frame_.Update(ctx.dt, slot_rects_[focus_], ctx.theme->animation);
}

bool StateSlotView::OnAction(GameMenuContext& ctx, InputAction action) {
    switch (action) {
    case InputAction::Left:
        focus_ = focus_ > 0 ? focus_ - 1 : focus_;
        return true;
    case InputAction::Right:
        focus_ = focus_ < kPerPage - 1 ? focus_ + 1 : focus_;
        return true;
    case InputAction::Up:
        focus_ = focus_ >= 2 ? focus_ - 2 : focus_;
        return true;
    case InputAction::Down:
        focus_ = focus_ < kPerPage - 2 ? focus_ + 2 : focus_;
        return true;
    case InputAction::PageLeft:
        page_ = (page_ + PageCount() - 1) % PageCount();
        RefreshSlots();
        return true;
    case InputAction::PageRight:
        page_ = (page_ + 1) % PageCount();
        RefreshSlots();
        return true;
    case InputAction::Confirm: {
        slots_[focus_].TriggerPress();
        const int slot = SlotIndex(focus_);
        if (slot >= delegate_.SlotCount()) {
            return true;
        }
        if (saving_) {
            const bool ok = delegate_.SaveState(slot);
            SetHint(ok ? "已保存" : "保存失败");
            if (ok) {
                RefreshSlots();
            }
            hint_danger_ = !ok;
        } else if (!slot_data_[focus_].exists) {
            // 空槽不执行读取，也不弹系统级错误窗口（需求 §17）
            SetHint("空存档槽");
            hint_danger_ = true;
        } else {
            const bool ok = delegate_.LoadState(slot);
            SetHint(ok ? "已读取" : "读取失败");
            hint_danger_ = !ok;
        }
        return true;
    }
    default:
        return false;
    }
}

void StateSlotView::Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) {
    const GameMenuTheme& theme = *ctx.theme;
    const MenuAnimationConfig& cfg = theme.animation;

    constexpr int kColumns = 2;
    constexpr int kRows = kPerPage / kColumns;
    const float gap = 12.0f;
    const float col_w = (area.Width() - gap * (kColumns - 1)) / static_cast<float>(kColumns);
    const float row_h = (area.Height() - 34.0f - gap * (kRows - 1)) / static_cast<float>(kRows);

    for (int i = 0; i < kPerPage; ++i) {
        const int col = i % kColumns;
        const int row = i / kColumns;
        const float stagger = EaseOutCubic(StaggerProgress(ctx.open_progress, i, cfg));
        const float dx = (1.0f - stagger) * 40.0f;
        const float dy = (1.0f - stagger) * 16.0f;
        const GameMenuTheme item_theme = ThemeWithAlpha(theme, stagger);

        const Rect cell = MakeRect(area.min.x + col * (col_w + gap) + dx,
                                   area.min.y + row * (row_h + gap) + dy, col_w, row_h);
        slot_rects_[i] = slots_[i].Draw(draw_list, item_theme, cell, SlotIndex(i), slot_data_[i],
                                        ctx.time);
    }

    focus_frame_.Draw(draw_list, theme, EaseOutCubic(ctx.open_progress));

    // 页码 + 操作提示
    const float info_y = area.max.y - 22.0f;
    char page_text[32];
    std::snprintf(page_text, sizeof(page_text), "%d / %d", page_ + 1, PageCount());
    AddTextLeftVCentered(draw_list, ImVec2(area.min.x + 4.0f, info_y),
                         ColorWithAlpha(theme.white_dim, 0.9f), theme.small_size, "L / R 翻页");
    AddTextRight(draw_list, ImVec2(area.max.x - 4.0f, info_y - theme.small_size * 0.5f),
                 ColorWithAlpha(theme.white, 0.85f), theme.small_size, page_text);

    if (hint_timer_ > 0.0f) {
        const float a = Clamp01(hint_timer_ / 0.6f);
        const ImU32 col = hint_danger_ ? theme.red : theme.white;
        AddTextLeftVCentered(draw_list, ImVec2(area.min.x + 140.0f, info_y),
                             ColorWithAlpha(col, a), theme.small_size, hint_text_);
    }
}

} // namespace gui_dev::gamemenu
