// GameMenu 外壳：视图栈、开关状态机、输入分发。
//
// 分工（对应需求 §35）：
//   GameMenuHost  = 菜单系统：什么时候开/关、当前是哪一层、输入给谁
//   GameMenuView  = 一层页面：自己有什么项、焦点在哪、怎么画
//   Element       = 单项的动画与绘制
// ImGui 只作为 DrawList 提供者，业务逻辑不写进绘制代码。
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <imgui.h>

#include "gamemenu/GameMenuDraw.h"
#include "gamemenu/GameMenuTheme.h"
#include "platform/Input.h"

namespace gui_dev::gamemenu {

class GameMenuHost;

// 视图上下文：由 Host 每帧填好后交给视图。
// 视图不接触 ImGui 之外的东西（不碰 SDL、不碰模拟核心）。
struct GameMenuContext {
    const GameMenuTheme* theme = nullptr;
    GameMenuHost* host = nullptr;
    float time = 0.0f;        // 菜单自己的累计时间（打开时从 0 开始）
    float dt = 0.0f;
    float open_progress = 0.0f; // 0..1 整体入场进度，用于逐项 stagger
    Rect screen{};              // 当前画布矩形（对话框居中用）
    GameMenuLayout layout{};    // 按当前画布自适应出来的布局
};

class GameMenuView {
public:
    virtual ~GameMenuView() = default;

    virtual const char* Title() const = 0;
    virtual const char* Subtitle() const { return ""; }

    // 进入（含从子页返回）时调用：**必须**在这里 Reset 所有元素动画，
    // 否则会出现「再次打开时动画状态残留」（需求 §28）。
    virtual void OnEnter(GameMenuContext& ctx) = 0;
    virtual void OnLeave(GameMenuContext& ctx) { (void)ctx; }

    virtual void Update(GameMenuContext& ctx) = 0;
    virtual void Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) = 0;

    // 返回 true 表示已消费。方向键/确认/取消/L/R 都会送进来。
    virtual bool OnAction(GameMenuContext& ctx, InputAction action) = 0;

    // 覆盖层（对话框）：下层视图仍然绘制
    virtual bool IsOverlay() const { return false; }
    // 覆盖层自己的遮罩（对话框把整屏压暗）
    virtual void DrawScreenOverlay(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& screen) {
        (void)ctx;
        (void)draw_list;
        (void)screen;
    }
    virtual bool HasScreenOverlay() const { return false; }
};

// 菜单外壳
class GameMenuHost {
public:
    explicit GameMenuHost(const GameMenuTheme& theme = DefaultGameMenuTheme());
    ~GameMenuHost();

    GameMenuHost(const GameMenuHost&) = delete;
    GameMenuHost& operator=(const GameMenuHost&) = delete;

    // 关掉菜单并清空视图栈（OnLeave 会被调用）。用于「退出游戏」这类硬关闭。
    void Reset();

    bool Visible() const { return state_ != State::Closed; }
    // 菜单可见即视为暂停：游戏逻辑与时间推进应停在这里
    bool Paused() const { return state_ != State::Closed; }
    bool AcceptsInput() const { return state_ == State::Entering || state_ == State::Open; }

    // 根视图工厂：每次打开菜单都创建一个新的根视图（动画状态才干净）。
    // 必须在 OpenMenu 之前设置，否则菜单会被「栈空即关闭」的兜底逻辑立刻关掉。
    void SetRootViewFactory(std::function<std::unique_ptr<GameMenuView>()> factory);

    void OpenMenu();
    // + 键 / 根视图按 B：请求关闭（整层退出，不等子页）
    void CloseMenu();

    // 每帧调用一次。pad 由调用方提供（与输入系统解耦）。
    void Update(float dt, const PadState& pad);

    // 把菜单画到 draw_list 上。screen 是整个画布矩形。
    void Draw(ImDrawList* draw_list, const Rect& screen);

    // 视图/委托改栈必须用 Request*（下一帧应用），
    // 否则会在 Host 遍历视图时销毁正在使用的对象。
    void RequestPush(std::unique_ptr<GameMenuView> view);
    void RequestPop() { pending_pop_ = true; }

    int Depth() const { return static_cast<int>(views_.size()); }
    bool AtRoot() const { return views_.size() <= 1; }

    const GameMenuTheme& Theme() const { return *theme_; }
    GameMenuContext& Context() { return ctx_; }

    bool HasPendingStackChange() const { return pending_push_ != nullptr || pending_pop_; }

private:
    enum class State : std::uint8_t { Closed, Entering, Open, Exiting };

    void PushView(std::unique_ptr<GameMenuView> view);
    void PopView();

    void UpdateInput(const PadState& pad);
    void FinishClose();
    void ApplyPendingStackChanges();
    void NotifyEnter(GameMenuView& view);
    void NotifyLeave(GameMenuView& view);

    const GameMenuTheme* theme_ = nullptr;
    std::function<std::unique_ptr<GameMenuView>()> root_factory_;
    std::vector<std::unique_ptr<GameMenuView>> views_;
    std::unique_ptr<GameMenuView> pending_push_;
    GameMenuContext ctx_{};
    MenuAnimationState shell_{};
    State state_ = State::Closed;
    bool hotkey_latched_ = false;
    bool pending_pop_ = false;
    float time_ = 0.0f;
    float exit_hold_ = 0.0f;
};

} // namespace gui_dev::gamemenu
