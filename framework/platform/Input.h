// 抽象输入：后端把平台事件（键盘/手柄/触摸）翻译成这里的动作，
// src/ui 与 src/core 只认这些动作，不认 scancode，因此换平台不影响上层。
#pragma once

#include <cstddef>
#include <cstdint>

namespace gui_dev {

enum class InputAction : std::uint8_t {
    None = 0,
    Up,
    Down,
    Left,
    Right,
    Confirm,     // A / Enter / 确定
    Cancel,      // B / Esc / 返回
    ActionX,     // X / X 键 / 辅助操作
    ActionY,     // Y / Y 键 / 辅助操作
    Menu,        // + / Start / 菜单
    Minus,       // - / Select / 关闭
    PageLeft,    // L
    PageRight,   // R
    TriggerLeft, // ZL
    TriggerRight,// ZR
    Count,
};

inline constexpr std::size_t kInputActionCount = static_cast<std::size_t>(InputAction::Count);

// 按键状态。pressed = 本帧刚按下（边沿，每帧重置），held = 当前是否按住（电平，
// 由后端在帧之间维护，长按/连发类功能用它）。
struct PadState {
    bool pressed[kInputActionCount] = {};
    bool held[kInputActionCount] = {};

    bool Pressed(InputAction a) const {
        return pressed[static_cast<std::size_t>(a)];
    }
    bool Held(InputAction a) const {
        return held[static_cast<std::size_t>(a)];
    }
};

// 按键自动重复（长按 = 连续移动/连续调值）。
//
// 后端只给 pressed（边沿）和 held（电平），「按住多久后开始重复、重复多快」是 UI 语义，
// 放在这里统一实现一次，控件与焦点导航都复用，不要各写一套。
//
// 用法（每帧、每个要重复的动作调一次）：
//   if (repeat.Tick(pad, InputAction::Down, dt)) { ...移动一格... }
// Tick 返回 true = 本帧该动作「生效一次」：首次按下，或按住到达重复点。
struct PadRepeat {
    float delay = 0.35f;         // 按住多久后开始重复（秒）
    float interval = 0.14f;      // 起始重复间隔（秒）
    float min_interval = 0.05f;  // 最快重复间隔（长按到底时的上限）
    float accel_time = 1.2f;     // 从现在到最快间隔用多久

    // 清掉所有通道状态（切页/弹窗打开时调，避免刚打开就吃掉一次按住）
    void Reset() {
        for (std::size_t i = 0; i < kInputActionCount; ++i) {
            channels[i] = Channel{};
        }
    }

    // 该动作当前是否按住（held 电平，导航时用来判断要不要继续重复）
    bool Held(const PadState& pad, InputAction action) const { return pad.Held(action); }

    // 每帧推进一次。返回 true 表示本帧产生一次触发。
    //
    // 注意：先看 pressed 再看 held。一次「快速点按」可能在同一帧里就 down+up
    // （帧率低/手柄连发时很常见），这时 held 已经是 false，只看 held 会把这次点按吞掉
    // —— 表现为方向键偶尔没反应。所以 pressed 永远算一次触发。
    bool Tick(const PadState& pad, InputAction action, float dt) {
        const std::size_t index = static_cast<std::size_t>(action);
        if (index >= kInputActionCount) {
            return false;
        }
        Channel& channel = channels[index];
        if (pad.Pressed(action)) {
            channel = Channel{};
            channel.active = true;
            return true;
        }
        if (!pad.Held(action)) {
            channel = Channel{};
            return false;
        }
        if (!channel.active) {
            // 没见过它的按下（比如按住状态下切页）：从这一刻开始计时，别直接连发
            channel = Channel{};
            channel.active = true;
            return false;
        }
        channel.hold += dt;
        if (!channel.repeating) {
            if (channel.hold < delay) {
                return false;
            }
            channel.repeating = true;
            channel.timer = 0.0f;
            return true;
        }
        channel.timer += dt;
        bool fired = false;
        // 一帧最多补 4 次，避免掉帧时一次跳很远
        for (int guard = 0; guard < 4; ++guard) {
            const float progress = accel_time > 0.0f ? (channel.hold - delay) / accel_time : 1.0f;
            const float clamped = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
            const float current = interval + (min_interval - interval) * clamped;
            const float step = current > 0.001f ? current : 0.001f;
            if (channel.timer < step) {
                break;
            }
            channel.timer -= step;
            fired = true;
        }
        return fired;
    }

private:
    struct Channel {
        float hold = 0.0f;
        float timer = 0.0f;
        bool repeating = false;
        bool active = false; // 见过按下（或从按住中途开始接管）
    };
    Channel channels[kInputActionCount] = {};
};

struct TouchPoint {
    float x = 0.0f;
    float y = 0.0f;
    bool down = false;
};

// 一帧的输入快照。后续加鼠标/多点触控时只在这里加字段。
struct InputFrame {
    PadState pad;
    TouchPoint touch;
};

} // namespace gui_dev
