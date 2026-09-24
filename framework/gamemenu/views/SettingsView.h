// 设置页：左侧分类 + 右侧选项（独立设置 / 全局设置共用，靠 scope 区分，需求 §18~§21）。
#pragma once

#include "gamemenu/GameMenuElement.h"
#include "gamemenu/GameMenuView.h"

namespace gui_dev::gamemenu {

// 设置项变更的出口（demo 里只记录；接真实配置系统时换这一层）
class SettingsDelegate {
public:
    virtual ~SettingsDelegate() = default;
    virtual void OnSettingChanged(bool per_game, const char* category, const char* option,
                                  int value) = 0;
    virtual void OnSettingCommand(bool per_game, const char* category, const char* option) = 0;
};

class SettingsView final : public GameMenuView {
public:
    static constexpr int kMaxCategories = 6;
    static constexpr int kMaxOptions = 5;

    SettingsView(SettingsDelegate& delegate, bool per_game);

    const char* Title() const override { return per_game_ ? "独立设置" : "全局设置"; }
    const char* Subtitle() const override {
        return per_game_ ? "仅影响当前游戏 / 当前核心" : "影响整个 GBAStation";
    }

    void OnEnter(GameMenuContext& ctx) override;
    void Update(GameMenuContext& ctx) override;
    void Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) override;
    bool OnAction(GameMenuContext& ctx, InputAction action) override;

private:
    void LoadCategory(int index, float direction);
    void SetStatus(const char* text);

    SettingsDelegate& delegate_;
    bool per_game_;
    int category_ = 0;
    int focus_ = 0;
    int category_count_ = 0;
    int option_count_ = 0;
    GameMenuTab tabs_[kMaxCategories];
    MenuOptionRow rows_[kMaxOptions];
    GameMenuFocusFrame focus_frame_;
    Rect row_rects_[kMaxOptions]{};
    Rect tab_rects_[kMaxCategories]{};
    float category_anim_ = 0.0f; // 1 -> 0，切分类时整列内容滑动
    int category_direction_ = 0;
    float status_timer_ = 0.0f;
    char status_text_[64] = {};
};

} // namespace gui_dev::gamemenu
