// 组件库调色板与尺寸规范：VSCode Dark+ 配色（背景不使用纯黑）。
#pragma once

#include <imgui.h>

namespace gui_dev::cv::Theme {

// ---- VSCode Dark+ 调色板 --------------------------------------------------
// 编辑器底色 #1E1E1E 而不是 #000000：纯黑在大屏上对比过强、边缘有光晕感。
inline constexpr ImU32 kBgEditor     = IM_COL32(0x1E, 0x1E, 0x1E, 0xFF);
inline constexpr ImU32 kBgSideBar    = IM_COL32(0x25, 0x25, 0x26, 0xFF);
inline constexpr ImU32 kBgPanel      = IM_COL32(0x25, 0x25, 0x26, 0xFF);
inline constexpr ImU32 kBgActivity   = IM_COL32(0x33, 0x33, 0x33, 0xFF);
inline constexpr ImU32 kBgWidget     = IM_COL32(0x2D, 0x2D, 0x30, 0xFF);
inline constexpr ImU32 kBgWidgetHi   = IM_COL32(0x37, 0x37, 0x3A, 0xFF);
inline constexpr ImU32 kBgInput      = IM_COL32(0x3C, 0x3C, 0x3C, 0xFF);
inline constexpr ImU32 kBorder       = IM_COL32(0x3C, 0x3C, 0x3C, 0xFF);
inline constexpr ImU32 kBorderStrong = IM_COL32(0x54, 0x54, 0x58, 0xFF);

inline constexpr ImU32 kTextPrimary   = IM_COL32(0xD4, 0xD4, 0xD4, 0xFF);
inline constexpr ImU32 kTextBright    = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
inline constexpr ImU32 kTextMuted     = IM_COL32(0x85, 0x85, 0x85, 0xFF);
inline constexpr ImU32 kTextDisabled  = IM_COL32(0x6A, 0x6A, 0x6A, 0xFF);

inline constexpr ImU32 kAccent        = IM_COL32(0x00, 0x7A, 0xCC, 0xFF); // VSCode 焦点蓝
inline constexpr ImU32 kAccentHover   = IM_COL32(0x11, 0x77, 0xBB, 0xFF);
inline constexpr ImU32 kButton        = IM_COL32(0x0E, 0x63, 0x9C, 0xFF);
inline constexpr ImU32 kButtonActive  = IM_COL32(0x0A, 0x4B, 0x77, 0xFF);
inline constexpr ImU32 kSelection     = IM_COL32(0x26, 0x4F, 0x78, 0xFF);

inline constexpr ImU32 kError         = IM_COL32(0xF1, 0x4C, 0x4C, 0xFF);
inline constexpr ImU32 kWarning       = IM_COL32(0xCC, 0xA7, 0x00, 0xFF);
inline constexpr ImU32 kTeal          = IM_COL32(0x4E, 0xC9, 0xB0, 0xFF);
inline constexpr ImU32 kOrange        = IM_COL32(0xCE, 0x91, 0x78, 0xFF);
inline constexpr ImU32 kPurple        = IM_COL32(0xC5, 0x86, 0xC0, 0xFF);
inline constexpr ImU32 kYellow        = IM_COL32(0xDC, 0xDC, 0xAA, 0xFF);
inline constexpr ImU32 kBlue          = IM_COL32(0x56, 0x9C, 0xD6, 0xFF);
inline constexpr ImU32 kGreen         = IM_COL32(0x6A, 0x99, 0x55, 0xFF);

inline constexpr ImU32 kShadow        = IM_COL32(0x00, 0x00, 0x00, 0x8C);
inline constexpr ImU32 kShadowSoft    = IM_COL32(0x00, 0x00, 0x00, 0x50);

// ---- 尺寸（720p 设计空间，后端按 UiScale 统一放大） -------------------------
inline constexpr float kFontTitle  = 34.0f;
inline constexpr float kFontHeader = 26.0f;
inline constexpr float kFontBody   = 22.0f;
inline constexpr float kFontSmall  = 16.0f;
inline constexpr float kFontTiny   = 13.0f;

inline constexpr float kRadiusNone  = 0.0f;
inline constexpr float kRadiusSmall = 4.0f;
inline constexpr float kRadius      = 8.0f;
inline constexpr float kRadiusLarge = 16.0f;

inline constexpr float kGapSmall = 8.0f;
inline constexpr float kGap      = 14.0f;
inline constexpr float kGapLarge = 24.0f;

// ---- 颜色工具 ------------------------------------------------------------
// 取同色不同透明度：alpha 是 0..1 的倍率。
ImU32 Alpha(ImU32 color, float alpha);
// 颜色插值：t=0 → a，t=1 → b。
ImU32 Mix(ImU32 a, ImU32 b, float t);

// 把调色板套到 ImGui 默认样式上（用到标准控件时视觉一致）。
void ApplyToImGui();

} // namespace gui_dev::cv::Theme
