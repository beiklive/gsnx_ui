// 确认对话框：重置游戏 / 退出游戏 / 未保存提示（需求 §22/§23）。
// 与菜单同一套斜切视觉，覆盖在菜单之上。
#pragma once

#include "gamemenu/GameMenuElement.h"
#include "gamemenu/GameMenuView.h"

namespace gui_dev::gamemenu {

class DialogDelegate {
public:
    virtual ~DialogDelegate() = default;
    // 返回 true 表示对话框关闭（demo 里总是关闭）
    virtual bool OnDialogResult(GameMenuContext& ctx, int button_index) = 0;
};

class DialogView final : public GameMenuView {
public:
    struct ButtonDef {
        const char* label;
        bool danger;
    };

    DialogView(const char* title, const char* const* lines, int line_count,
               const ButtonDef* buttons, int button_count, int default_focus,
               DialogDelegate& delegate);

    const char* Title() const override { return title_; }
    bool IsOverlay() const override { return true; }
    bool HasScreenOverlay() const override { return true; }
    void DrawScreenOverlay(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& screen) override;

    void OnEnter(GameMenuContext& ctx) override;
    void Update(GameMenuContext& ctx) override;
    void Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) override;
    bool OnAction(GameMenuContext& ctx, InputAction action) override;

private:
    static constexpr int kMaxButtons = 3;
    static constexpr int kMaxLines = 4;

    const char* title_ = "";
    const char* lines_[kMaxLines]{};
    int line_count_ = 0;
    GameMenuButton buttons_[kMaxButtons];
    GameMenuFocusFrame focus_frame_;
    Rect button_rects_[kMaxButtons]{};
    int button_count_ = 0;
    int focus_ = 0;
    float enter_ = 0.0f;
    DialogDelegate& delegate_;
};

} // namespace gui_dev::gamemenu
