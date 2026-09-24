// 统一视觉规范：所有核心的 UI 都从这里取色与尺寸，保证「一套前端」。
#pragma once

#include <imgui.h>

namespace gui_dev::Theme {

// ---- 颜色 ----------------------------------------------------------------
inline constexpr ImU32 kAccent        = IM_COL32(0x4F, 0xA3, 0xFF, 0xFF);
inline constexpr ImU32 kAccentDim     = IM_COL32(0x2C, 0x5C, 0x91, 0xFF);
inline constexpr ImU32 kSuccess       = IM_COL32(0x3F, 0xC3, 0x80, 0xFF);
inline constexpr ImU32 kWarning       = IM_COL32(0xFF, 0xB0, 0x3A, 0xFF);
inline constexpr ImU32 kDanger        = IM_COL32(0xE0, 0x5A, 0x5A, 0xFF);
inline constexpr ImU32 kTextPrimary   = IM_COL32(0xEC, 0xEE, 0xF1, 0xFF);
inline constexpr ImU32 kTextSecondary = IM_COL32(0x9A, 0xA1, 0xAC, 0xFF);
inline constexpr ImU32 kPanelBg       = IM_COL32(0x1A, 0x1D, 0x22, 0xF2);

// ---- 尺寸（逻辑像素，与 UiScale 相乘由 ImGui 样式统一处理）-----------------
inline constexpr float kRowHeight          = 42.0f;
inline constexpr float kHeaderHeight       = 52.0f;
inline constexpr float kFooterHeight       = 40.0f;
inline constexpr float kGapSmall           = 4.0f;
inline constexpr float kGap                = 8.0f;
inline constexpr float kGapLarge           = 16.0f;
inline constexpr float kPanelRounding      = 6.0f;
inline constexpr float kCompactBreakpoint  = 520.0f;

// 组件里用到的堆压栈键，集中在 .cpp 内部实现，这里只暴露语义化取值。
inline ImVec4 ToVec4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

// 把统一主题套到 ImGui 上。每个后端初始化 ImGui 后调用一次。
void Apply();

// 单独套用颜色（保留后端默认的尺寸/圆角）。
void ApplyColors();

} // namespace gui_dev::Theme
