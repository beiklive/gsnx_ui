// 一级菜单：高频操作 / 配置 / 危险操作三组，用不规则分隔线分开。
#pragma once

#include "gamemenu/GameMenuElement.h"
#include "gamemenu/GameMenuView.h"

namespace gui_dev::gamemenu {

// 一级菜单的动作出口。
// UI 不直接调用模拟核心/存档系统，全部通过这个接口出去（需求 §30/§35）。
class MainMenuDelegate {
public:
    virtual ~MainMenuDelegate() = default;

    // ctx 用于 RequestPush（改栈必须延迟到下一帧）
    virtual const char* GameTitle() const = 0;
    virtual void OnResume(GameMenuContext& ctx) = 0;
    virtual void OnOpenStateSlots(GameMenuContext& ctx, bool saving) = 0;
    virtual void OnOpenSettings(GameMenuContext& ctx, bool per_game) = 0;
    virtual void OnRequestReset(GameMenuContext& ctx) = 0;
    virtual void OnRequestExit(GameMenuContext& ctx) = 0;
};

class MainMenuView final : public GameMenuView {
public:
    static constexpr int kItemCount = 7;

    explicit MainMenuView(MainMenuDelegate& delegate);

    const char* Title() const override { return "暂停菜单"; }
    const char* Subtitle() const override { return delegate_.GameTitle(); }

    void OnEnter(GameMenuContext& ctx) override;
    void Update(GameMenuContext& ctx) override;
    void Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) override;
    bool OnAction(GameMenuContext& ctx, InputAction action) override;

private:
    void Activate(GameMenuContext& ctx, int index);

    MainMenuDelegate& delegate_;
    GameMenuButton items_[kItemCount];
    GameMenuFocusFrame focus_frame_;
    Rect item_rects_[kItemCount]{}; // 上一帧布局，用于焦点框跟随与鼠标命中
    int focus_ = 0;
};

} // namespace gui_dev::gamemenu
