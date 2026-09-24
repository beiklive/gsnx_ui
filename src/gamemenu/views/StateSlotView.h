// 存档 / 读档：同一个槽位 UI，保存与读取只有确认行为不同（需求 §16/§17）。
#pragma once

#include "gamemenu/GameMenuElement.h"
#include "gamemenu/GameMenuView.h"

namespace gui_dev::gamemenu {

// 槽位数据与读写动作的出口（demo 用 Mock，接真实实现时只换这一层）
class StateSlotDelegate {
public:
    virtual ~StateSlotDelegate() = default;

    virtual int SlotCount() const = 0;
    static constexpr int kSlotsPerPage = 6;

    // 填充显示数据；不得在内部做每帧堆分配（用固定缓冲/缓存）
    virtual void FillSlot(int slot, GameMenuSlotData& out) const = 0;
    virtual bool SaveState(int slot) = 0;
    virtual bool LoadState(int slot) = 0;
};

class StateSlotView final : public GameMenuView {
public:
    StateSlotView(StateSlotDelegate& delegate, bool saving);

    const char* Title() const override { return saving_ ? "保存状态" : "读取状态"; }
    const char* Subtitle() const override { return "A 确认   L/R 翻页   B 返回"; }

    void OnEnter(GameMenuContext& ctx) override;
    void Update(GameMenuContext& ctx) override;
    void Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) override;
    bool OnAction(GameMenuContext& ctx, InputAction action) override;

private:
    static constexpr int kPerPage = StateSlotDelegate::kSlotsPerPage;

    void RefreshSlots();
    void SetHint(const char* text);
    int PageCount() const;
    int SlotIndex(int page_slot) const { return page_ * kPerPage + page_slot; }

    StateSlotDelegate& delegate_;
    bool saving_;
    GameMenuSaveSlot slots_[kPerPage];
    GameMenuSlotData slot_data_[kPerPage];
    GameMenuFocusFrame focus_frame_;
    Rect slot_rects_[kPerPage]{};
    int focus_ = 0;
    int page_ = 0;
    float hint_timer_ = 0.0f;
    bool hint_danger_ = false;
    char hint_text_[64] = {};
};

} // namespace gui_dev::gamemenu
