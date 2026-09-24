// 抽象输入：后端把平台事件（键盘/手柄/触摸）翻译成这里的动作，
// src/ui 与 src/core 只认这些动作，不认 scancode，因此换平台不影响上层。
#pragma once

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
