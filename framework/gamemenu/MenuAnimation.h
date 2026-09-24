// 菜单动画基础：帧率无关的插值与缓动。
//
// 两条原则（对应需求 §4/§5）：
//   1. 一切动画都吃 DeltaTime，禁止 sleep / 阻塞 / 固定帧计数；
//   2. 需要「落在指定毫秒内」的进度用 MoveTowards（线性定时长），
//      需要「平滑跟随」的量用 SmoothTo（指数趋近）。
//      两者都天然与帧率无关：30/60/120 FPS 下时间表现一致。
#pragma once

#include <cstdint>

namespace gui_dev::gamemenu {

inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// 指数趋近：1 - exp(-speed * dt)。
// speed 越大越快；不会过冲，适合透明度、跟随类插值。
float SmoothTo(float current, float target, float speed, float dt);

// 定时长线性推进：在 duration 秒内从当前值走到 target，然后夹住。
// 用于必须有明确时长上限的进度（焦点 120~220ms、按压 80~150ms）。
float MoveTowards(float current, float target, float duration, float dt);

// 1 -> 0 的一次性衰减（扫描高光、闪白），走完停在 0。
float DecayOnce(float current, float duration, float dt);

// 0 -> 1 的一次性推进，走完停在 1。
float AdvanceOnce(float current, float duration, float dt);

float EaseOutCubic(float t);
float EaseOutQuad(float t);
float EaseInOutCubic(float t);
// 回弹：1 附近小幅过冲，用于焦点获得的「轻微弹性」
float EaseOutBack(float t);

struct MenuAnimationState {
    float focus = 0.0f; // 0..1 焦点强度
    float press = 0.0f; // 0..1 按压
    float enter = 0.0f; // 0..1 入场
    float exit = 0.0f;  // 0..1 退场
    float sweep = 0.0f; // 1..0 扫描高光（一次性）
    float flash = 0.0f; // 1..0 闪白（一次性）

    void Reset() { focus = press = enter = exit = sweep = flash = 0.0f; }
};

// 动画时长集中在这里，绘制与逻辑里都不许再出现魔法数字。
struct MenuAnimationConfig {
    float focus_duration = 0.16f; // 焦点切换（需求建议 120~220ms）
    float press_duration = 0.10f; // 按压反馈（需求建议 80~150ms）
    float sweep_duration = 0.22f; // 扫描高光（需求建议 150~300ms）
    float flash_duration = 0.26f;
    float enter_duration = 0.28f; // 菜单整体入场
    float exit_duration = 0.16f;  // 菜单整体退场
    float item_stagger = 0.025f;  // 菜单项逐个出现（需求建议 25ms 步进）
    float follow_smoothing = 22.0f; // 焦点框跟随用的指数速度
};

// 逐项出现（stagger）：第 index 项相对整体入场进度的延迟。
// open_progress 是 0..1 的整体进度，返回该项自己的 0..1 出现进度。
inline float StaggerProgress(float open_progress, int index, const MenuAnimationConfig& cfg) {
    const float delay = static_cast<float>(index) * cfg.item_stagger;
    const float span = cfg.enter_duration > 0.0f ? cfg.enter_duration : 0.2f;
    return Clamp01((open_progress * span - delay) / span);
}

} // namespace gui_dev::gamemenu
