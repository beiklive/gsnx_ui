// 组件库的动画基础：帧率无关的插值与缓动。
//
// 语义与 framework/gamemenu/MenuAnimation.h 完全一致（那套是游戏内菜单专用的），
// 组件库这边需要一份自己的，避免 component_view 反向依赖 gamemenu：
//   * 一切动画都吃 dt，禁止 sleep / 固定帧计数；
//   * 有明确时长的进度（入场、焦点切换 120~220ms、按压 80~150ms）用 MoveTowards；
//   * 平滑跟随的量（焦点框、滑条）用 SmoothTo（指数趋近，不会过冲）。
#pragma once

#include <cmath>

#include "component_view/Types.h"

namespace gui_dev::cv::Anim {

inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// 指数趋近：1 - exp(-speed * dt)
inline float SmoothTo(float current, float target, float speed, float dt) {
    if (speed <= 0.0f || dt <= 0.0f) {
        return current;
    }
    const float t = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * t;
}

// 定时长线性推进：在 duration 秒内从当前值走到 target，然后夹住
inline float MoveTowards(float current, float target, float duration, float dt) {
    if (duration <= 0.0f) {
        return target;
    }
    const float step = dt / duration;
    if (current < target) {
        current += step;
        return current > target ? target : current;
    }
    current -= step;
    return current < target ? target : current;
}

// 一次性推进到 1（走完停住）：入场进度用
inline float AdvanceOnce(float current, float duration, float dt) {
    return current >= 1.0f ? 1.0f : MoveTowards(current, 1.0f, duration, dt);
}

inline float EaseOutCubic(float t) {
    const float x = 1.0f - Clamp01(t);
    return 1.0f - x * x * x;
}

// 回弹（1 附近小幅过冲）：焦点获得的「轻微弹性」用这个
inline float EaseOutBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float x = Clamp01(t) - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

// 逐项出现（stagger）：第 index 项相对整体起点的延迟。
// 注意：这里按「已经播了多少秒 elapsed」算，而不是按 0..1 的整体进度算 ——
// 一组 n 项真正走完的总时长是 StaggerTotal()，最后一项才会到 1；
// 如果拿 enter_duration 当整体进度直接乘，错开的尾巴会被截掉，最后几项永远停在半透明。
inline float StaggerElapsed(float elapsed, int index, float item_stagger, float item_duration) {
    const float delay = static_cast<float>(index) * item_stagger;
    const float span = item_duration > 0.0f ? item_duration : 0.2f;
    return Clamp01((elapsed - delay) / span);
}

// 一组 count 项从第一个到最后一个全部走完需要的时间（秒）
inline float StaggerTotal(int count, float item_stagger, float item_duration) {
    const int last = count > 0 ? count - 1 : 0;
    return item_duration + static_cast<float>(last) * item_stagger;
}

} // namespace gui_dev::cv::Anim
