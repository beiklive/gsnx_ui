#include "gamemenu/GameMenuView.h"

#include <cmath>

#include "gamemenu/GameMenuElement.h"

namespace gui_dev::gamemenu {
namespace {

constexpr float kPi = 3.14159265358979323846f;

inline ImU32 WithAlpha(ImU32 col, float alpha) {
    const float a = Clamp01(alpha) * static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF);
    return (col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
}

} // namespace

GameMenuHost::GameMenuHost(const GameMenuTheme& theme) : theme_(&theme) {}

GameMenuHost::~GameMenuHost() {
    // 退出时把残留视图按 OnLeave 收尾，保证下次打开是干净状态
    for (auto& view : views_) {
        NotifyLeave(*view);
    }
}

void GameMenuHost::NotifyEnter(GameMenuView& view) { view.OnEnter(ctx_); }

void GameMenuHost::NotifyLeave(GameMenuView& view) { view.OnLeave(ctx_); }

void GameMenuHost::Reset() {
    for (auto& view : views_) {
        NotifyLeave(*view);
    }
    views_.clear();
    pending_push_.reset();
    pending_pop_ = false;
    shell_.Reset();
    state_ = State::Closed;
    hotkey_latched_ = false;
    time_ = 0.0f;
    exit_hold_ = 0.0f;
}

void GameMenuHost::SetRootViewFactory(std::function<std::unique_ptr<GameMenuView>()> factory) {
    root_factory_ = std::move(factory);
}

void GameMenuHost::OpenMenu() {
    if (state_ == State::Entering || state_ == State::Open) {
        return;
    }
    // 从关闭态重新打开：清掉上一次的动画残留（需求 §28）
    shell_.Reset();
    time_ = 0.0f;
    exit_hold_ = 0.0f;
    if (views_.empty() && root_factory_) {
        PushView(root_factory_());
    }
    state_ = State::Entering;
}

void GameMenuHost::CloseMenu() {
    if (state_ == State::Closed || state_ == State::Exiting) {
        return;
    }
    shell_.exit = 0.0f;
    state_ = State::Exiting;
}

void GameMenuHost::PushView(std::unique_ptr<GameMenuView> view) {
    if (view == nullptr) {
        return;
    }
    NotifyEnter(*view);
    views_.push_back(std::move(view));
}

void GameMenuHost::PopView() {
    if (views_.empty()) {
        return;
    }
    NotifyLeave(*views_.back());
    views_.pop_back();
}

void GameMenuHost::RequestPush(std::unique_ptr<GameMenuView> view) { pending_push_ = std::move(view); }

void GameMenuHost::ApplyPendingStackChanges() {
    if (pending_push_ != nullptr) {
        PushView(std::move(pending_push_));
        pending_push_.reset();
    }
    if (pending_pop_) {
        pending_pop_ = false;
        PopView();
    }
}

void GameMenuHost::FinishClose() {
    // 整层退出：所有视图都收尾（含对话框）
    for (auto it = views_.rbegin(); it != views_.rend(); ++it) {
        NotifyLeave(**it);
    }
    views_.clear();
    pending_push_.reset();
    pending_pop_ = false;
    state_ = State::Closed;
    shell_.Reset();
    time_ = 0.0f;
}

void GameMenuHost::UpdateInput(const PadState& pad) {
    // + 键：无论在第几层、甚至在全屏对话框里，都直接回到游戏（需求 §24）
    if (pad.Pressed(InputAction::Menu)) {
        CloseMenu();
        return;
    }
    if (views_.empty()) {
        return;
    }
    GameMenuView& top = *views_.back();

    if (pad.Pressed(InputAction::Cancel)) {
        if (views_.size() > 1) {
            pending_pop_ = true;
        } else {
            CloseMenu();
        }
        return;
    }

    static constexpr InputAction kRouted[] = {
        InputAction::Up,       InputAction::Down,      InputAction::Left,     InputAction::Right,
        InputAction::Confirm,  InputAction::PageLeft,  InputAction::PageRight,
    };
    for (InputAction action : kRouted) {
        if (pad.Pressed(action)) {
            top.OnAction(ctx_, action);
        }
    }
}

void GameMenuHost::Update(float dt, const PadState& pad) {
    if (dt < 0.0f) {
        dt = 0.0f;
    }
    ctx_.theme = theme_;
    ctx_.host = this;
    ctx_.dt = dt;

    // 画布 = 逻辑空间尺寸（后端已按 720p 基准换算好），布局按它自适应
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ctx_.screen = MakeRect(0.0f, 0.0f, display.x, display.y);
    ctx_.layout = ResolveGameMenuLayout(*theme_, ctx_.screen);

    const MenuAnimationConfig& cfg = theme_->animation;

    // 热键 ZL + ZR：同时按住为一次开关（边沿触发，避免连发）
    const bool both = pad.Held(InputAction::TriggerLeft) && pad.Held(InputAction::TriggerRight);
    if (both && !hotkey_latched_) {
        if (state_ == State::Closed) {
            OpenMenu();
        } else if (state_ == State::Open || state_ == State::Entering) {
            CloseMenu();
        }
    }
    hotkey_latched_ = both;

    switch (state_) {
    case State::Entering:
        shell_.enter = AdvanceOnce(shell_.enter, cfg.enter_duration, dt);
        if (shell_.enter >= 1.0f) {
            state_ = State::Open;
        }
        break;
    case State::Exiting:
        shell_.exit = AdvanceOnce(shell_.exit, cfg.exit_duration, dt);
        if (shell_.exit >= 1.0f) {
            FinishClose();
        }
        break;
    case State::Closed:
    case State::Open:
    default:
        break;
    }

    if (state_ == State::Closed) {
        ctx_.time = time_;
        return;
    }

    time_ += dt;
    ctx_.time = time_;
    ctx_.open_progress = state_ == State::Entering ? shell_.enter : 1.0f;

    // 关闭动画期间不接受操作，但视图仍需推进动画（退出观感）
    if (AcceptsInput()) {
        UpdateInput(pad);
    }

    for (auto& view : views_) {
        view->Update(ctx_);
    }

    ApplyPendingStackChanges();

    // 视图把自己抽空（理论上不会发生）：安全兜底，直接关闭
    if (views_.empty() && state_ != State::Exiting) {
        CloseMenu();
    }

    if (state_ == State::Exiting) {
        exit_hold_ += dt;
    }
}

void GameMenuHost::Draw(ImDrawList* draw_list, const Rect& screen) {
    if (state_ == State::Closed || draw_list == nullptr) {
        return;
    }
    ctx_.screen = screen;
    // 入场：整体位移用 EaseOutBack（弹性），透明度用 EaseOutCubic
    const float enter_pos = EaseOutBack(Clamp01(shell_.enter));
    // 退场：反向滑出
    const float exit_pos = EaseInOutCubic(Clamp01(shell_.exit));
    const float slide = enter_pos * (1.0f - exit_pos);
    const float alpha = Clamp01(shell_.exit > 0.0f ? 1.0f - exit_pos : enter_pos);

    // ---- 游戏画面遮罩（需求 §13：不要全黑，0.35~0.55）----
    const float dim = theme_->dim_alpha * (shell_.exit > 0.0f ? 1.0f - exit_pos : EaseOutCubic(shell_.enter));
    if (theme_->dim_alpha > 0.0f) {
        draw_list->AddRectFilled(screen.min, screen.max, WithAlpha(theme_->dim_color, dim));
    }

    // 覆盖层（对话框）自带的整屏遮罩
    if (!views_.empty() && views_.back()->HasScreenOverlay()) {
        views_.back()->DrawScreenOverlay(ctx_, draw_list, screen);
    }

    // ---- 面板几何：位置/尺寸都来自自适应布局，从屏幕**左侧**外滑入 ----
    const float panel_w = ctx_.layout.panel_width;
    const float panel_h = ctx_.layout.panel_height;
    const float offscreen = panel_w + theme_->skew * 2.0f + 90.0f;
    const float panel_x = ctx_.layout.panel_x - (1.0f - slide) * offscreen;

    const Rect panel = MakeRect(panel_x, ctx_.layout.panel_y, panel_w, panel_h);

    // 背板（红色斜切，略偏移，漫画切割感）
    if (slide > 0.02f) {
        const Rect back = panel.Offset(14.0f, 12.0f);
        AddSkewFilled(draw_list, back, theme_->skew, WithAlpha(theme_->red_dim, 0.9f * alpha));
        const Rect back2 = panel.Offset(-7.0f, 7.0f);
        AddSkewFilled(draw_list, back2, theme_->skew, WithAlpha(theme_->background, 0.7f * alpha));
    }

    // 内容区：标题条 + 副标题行都要让开（否则第一项会压住副标题）
    const float header_h = theme_->title_size + 34.0f;
    const float content_top = header_h + theme_->small_size + 20.0f;
    const Rect content = MakeRect(panel.min.x + theme_->panel_padding,
                                  panel.min.y + content_top,
                                  panel.Width() - theme_->panel_padding * 2.0f,
                                  panel.Height() - content_top - theme_->panel_padding);

    // 画哪些视图：覆盖层时把下层也画出来
    std::size_t first = views_.empty() ? 0 : views_.size() - 1;
    if (views_.size() >= 2 && views_.back()->IsOverlay()) {
        first = views_.size() - 2;
    }

    const char* title = views_.empty() ? "" : views_[first]->Title();
    const char* subtitle = views_.empty() ? "" : views_[first]->Subtitle();
    GameMenuPanel panel_draw;
    panel_draw.Draw(draw_list, *theme_, panel, title, subtitle, shell_.enter);

    for (std::size_t i = first; i < views_.size(); ++i) {
        views_[i]->Draw(ctx_, draw_list, content);
    }
}

} // namespace gui_dev::gamemenu
