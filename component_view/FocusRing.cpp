#include "component_view/FocusRing.h"

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/Widget.h"

namespace gui_dev::cv {
namespace {

// 跟随速度（1/s）：和 gamemenu 焦点框的 follow_smoothing 同一量级
constexpr float kFollowSpeed = 26.0f;
// 目标中心跳得比这个远就直接对齐（不横穿屏幕）
constexpr float kSnapDistance = 180.0f;
constexpr float kFadeSpeed = 22.0f;

} // namespace

void FocusRing::Reset() {
    rect_ = Rect{};
    has_rect_ = false;
    alpha_ = 0.0f;
    pause_style_ = false;
}

void FocusRing::Draw(ImDrawList* dl, Widget* target) {
    if (dl == nullptr) {
        return;
    }
    const float dt = Global::delta_time;
    FocusVisual visual = target != nullptr ? target->BuildFocusVisual() : FocusVisual{};

    if (visual.enabled) {
        if (!has_rect_) {
            rect_ = visual.rect;
            radius_ = visual.radius;
            has_rect_ = true;
        } else {
            const float jump = Absf(visual.rect.Center().x - rect_.Center().x) +
                               Absf(visual.rect.Center().y - rect_.Center().y);
            if (jump > kSnapDistance) {
                rect_ = visual.rect;
                radius_ = visual.radius;
            } else {
                rect_.min.x = Anim::SmoothTo(rect_.min.x, visual.rect.min.x, kFollowSpeed, dt);
                rect_.min.y = Anim::SmoothTo(rect_.min.y, visual.rect.min.y, kFollowSpeed, dt);
                rect_.max.x = Anim::SmoothTo(rect_.max.x, visual.rect.max.x, kFollowSpeed, dt);
                rect_.max.y = Anim::SmoothTo(rect_.max.y, visual.rect.max.y, kFollowSpeed, dt);
                radius_ = Anim::SmoothTo(radius_, visual.radius, kFollowSpeed, dt);
            }
        }
        flowing_ = visual.flowing;
        width_ = visual.width;
        color_ = visual.color;
        phase_ = visual.phase;
        saturation_ = visual.saturation;
        brightness_ = visual.brightness;
        pause_style_ = visual.pause_style;
        alpha_ = Anim::SmoothTo(alpha_, visual.alpha, kFadeSpeed, dt);
    } else {
        // 没有目标：原地淡出，并把基准矩形清掉（下次换目标直接对齐）
        alpha_ = Anim::SmoothTo(alpha_, 0.0f, kFadeSpeed, dt);
        has_rect_ = false;
        if (alpha_ <= 0.02f) {
            return;
        }
    }

    if (alpha_ <= 0.02f || !rect_.Valid()) {
        return;
    }

    // 焦点框现在画在最上层，不跟着控件被裁剪；但控件如果在滚动容器里，
    // 框仍然要裁到那个容器，否则会画到面板外面。
    Widget* host = target != nullptr ? target->ScrollHost() : nullptr;
    const bool clip = host != nullptr && host->overflow == Overflow::Scroll;
    if (clip) {
        const Rect view = host->DrawRect();
        dl->PushClipRect(view.min, view.max, true);
    }

    if (flowing_) {
        Draw::FlowingRing(dl, rect_, width_, phase_, saturation_, brightness_, alpha_, 3.0f, radius_);
    } else if (pause_style_) {
        // pause_menu 风格：红色强调外框 + 四角白色 L 形标记。
        Draw::RoundedRectOutline(dl, rect_, Theme::Alpha(Theme::U32(Theme::kError), alpha_ * 0.55f),
                                 Maxf(width_, 1.0f), radius_, radius_, radius_, radius_);
        const float tick = Minf(12.0f, rect_.Width() * 0.18f);
        const ImU32 white = Theme::Alpha(Theme::U32(Theme::kTextBright), alpha_ * 0.9f);
        const float x0 = rect_.min.x;
        const float x1 = rect_.max.x;
        const float y0 = rect_.min.y;
        const float y1 = rect_.max.y;
        dl->AddLine(ImVec2(x0, y0), ImVec2(x0 + tick, y0), white, 2.5f);
        dl->AddLine(ImVec2(x0, y0), ImVec2(x0, y0 + tick), white, 2.5f);
        dl->AddLine(ImVec2(x1, y0), ImVec2(x1 - tick, y0), white, 2.5f);
        dl->AddLine(ImVec2(x1, y0), ImVec2(x1, y0 + tick), white, 2.5f);
        dl->AddLine(ImVec2(x0, y1), ImVec2(x0 + tick, y1), white, 2.5f);
        dl->AddLine(ImVec2(x0, y1), ImVec2(x0, y1 - tick), white, 2.5f);
        dl->AddLine(ImVec2(x1, y1), ImVec2(x1 - tick, y1), white, 2.5f);
        dl->AddLine(ImVec2(x1, y1), ImVec2(x1, y1 - tick), white, 2.5f);
    } else {
        Draw::RoundedRectOutline(dl, rect_, Theme::Alpha(color_, alpha_), width_, radius_, radius_, radius_, radius_);
    }

    if (clip) {
        dl->PopClipRect();
    }
}

} // namespace gui_dev::cv
