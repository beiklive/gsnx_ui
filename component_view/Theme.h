// 组件库调色板与尺寸规范：VSCode Dark+ 配色（背景不使用纯黑）。
//
// 颜色统一用 **RGBA**（ImVec4，分量 0..1，w = alpha），和 ImGui 的样式系统一致：
//   * 直接写字面量：Theme::kBgEditor = ImVec4{0.118, 0.118, 0.118, 1.0}
//   * 想按十六进制写：Theme::RGBA(0x1E1E1EFF)（#RRGGBBAA）
//   * 交给 ImDrawList / Widget 的 ImU32 属性时：Theme::U32(color) 或 Theme::U32(color, alpha)
#pragma once

#include <imgui.h>

namespace gui_dev::cv::Theme {

// ---- 颜色工具（都能在编译期求值） ------------------------------------------
// #RRGGBBAA -> ImVec4。写配色时可以用它，避免 IM_COL32 的端序打包差异。
inline constexpr ImVec4 RGBA(unsigned int rgba) {
    return ImVec4(static_cast<float>((rgba >> 24) & 0xFFu) / 255.0f,
                  static_cast<float>((rgba >> 16) & 0xFFu) / 255.0f,
                  static_cast<float>((rgba >> 8) & 0xFFu) / 255.0f,
                  static_cast<float>(rgba & 0xFFu) / 255.0f);
}

// ImVec4 -> ImU32（ImDrawList 与 Widget 的 ImU32 属性用的打包值）
inline constexpr ImU32 U32(const ImVec4& c, float alpha_mul = 1.0f) {
    return IM_COL32(static_cast<int>(c.x * 255.0f + 0.5f), static_cast<int>(c.y * 255.0f + 0.5f),
                    static_cast<int>(c.z * 255.0f + 0.5f),
                    static_cast<int>(c.w * alpha_mul * 255.0f + 0.5f));
}

// 取同色不同透明度（alpha 是 0..1 的倍率），返回同类型
inline constexpr ImVec4 Alpha(const ImVec4& color, float alpha) {
    return ImVec4(color.x, color.y, color.z, color.w * alpha);
}
ImU32 Alpha(ImU32 color, float alpha);

// 颜色插值：t=0 → a，t=1 → b
inline constexpr ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}
ImU32 Mix(ImU32 a, ImU32 b, float t);

// ---- VSCode Dark+ 调色板（RGBA） ------------------------------------------
// 编辑器底色 #1E1E1E 而不是 #000000：纯黑在大屏上对比过强、边缘有光晕感。
inline constexpr ImVec4 kBgEditor{0.118, 0.118, 0.118, 1.000}; // #1E1E1E
inline constexpr ImVec4 kBgSideBar{0.145, 0.145, 0.149, 1.000}; // #252526
inline constexpr ImVec4 kBgPanel{0.145, 0.145, 0.149, 1.000}; // #252526
inline constexpr ImVec4 kBgActivity{0.200, 0.200, 0.200, 1.000}; // #333333
inline constexpr ImVec4 kBgWidget{0.176, 0.176, 0.188, 1.000}; // #2D2D30
inline constexpr ImVec4 kBgWidgetHi{0.216, 0.216, 0.227, 1.000}; // #37373A
inline constexpr ImVec4 kBgInput{0.235, 0.235, 0.235, 1.000}; // #3C3C3C
inline constexpr ImVec4 kBorder{0.235, 0.235, 0.235, 1.000}; // #3C3C3C
inline constexpr ImVec4 kBorderStrong{0.329, 0.329, 0.345, 1.000}; // #545458
inline constexpr ImVec4 kTextPrimary{0.831, 0.831, 0.831, 1.000}; // #D4D4D4
inline constexpr ImVec4 kTextBright{1.000, 1.000, 1.000, 1.000}; // #FFFFFF
inline constexpr ImVec4 kTextMuted{0.522, 0.522, 0.522, 1.000}; // #858585
inline constexpr ImVec4 kTextDisabled{0.416, 0.416, 0.416, 1.000}; // #6A6A6A
inline constexpr ImVec4 kAccent{0.000, 0.478, 0.800, 1.000}; // #007ACC  // VSCode 焦点蓝
inline constexpr ImVec4 kAccentHover{0.067, 0.467, 0.733, 1.000}; // #1177BB
inline constexpr ImVec4 kButton{0.055, 0.388, 0.612, 1.000}; // #0E639C
inline constexpr ImVec4 kButtonActive{0.039, 0.294, 0.467, 1.000}; // #0A4B77
inline constexpr ImVec4 kSelection{0.149, 0.310, 0.471, 1.000}; // #264F78
inline constexpr ImVec4 kError{0.945, 0.298, 0.298, 1.000}; // #F14C4C
inline constexpr ImVec4 kWarning{0.800, 0.655, 0.000, 1.000}; // #CCA700
inline constexpr ImVec4 kTeal{0.306, 0.788, 0.690, 1.000}; // #4EC9B0
inline constexpr ImVec4 kOrange{0.808, 0.569, 0.471, 1.000}; // #CE9178
inline constexpr ImVec4 kPurple{0.773, 0.525, 0.753, 1.000}; // #C586C0
inline constexpr ImVec4 kYellow{0.863, 0.863, 0.667, 1.000}; // #DCDCAA
inline constexpr ImVec4 kBlue{0.337, 0.612, 0.839, 1.000}; // #569CD6
inline constexpr ImVec4 kGreen{0.416, 0.600, 0.333, 1.000}; // #6A9955
inline constexpr ImVec4 kWhite{1.000, 1.000, 1.000, 1.000}; // #FFFFFF
inline constexpr ImVec4 kSuccess = kTeal;
inline constexpr ImVec4 kTrack{0.227, 0.227, 0.239, 1.000}; // #3A3A3D  // 滑条 / 进度条底色
inline constexpr ImVec4 kTrackFill{0.000, 0.478, 0.800, 1.000}; // #007ACC
inline constexpr ImVec4 kScrim{0.031, 0.031, 0.039, 0.784}; // #08080AC8  // 弹层遮罩
inline constexpr ImVec4 kListRow{0.165, 0.165, 0.176, 1.000}; // #2A2A2D
inline constexpr ImVec4 kListRowAlt{0.141, 0.141, 0.153, 1.000}; // #242427
inline constexpr ImVec4 kListRowFocus{0.149, 0.310, 0.471, 1.000}; // #264F78
inline constexpr ImVec4 kKeyBg{0.200, 0.200, 0.216, 1.000}; // #333337  // 虚拟键盘按键
inline constexpr ImVec4 kKeyBgTop{0.239, 0.239, 0.259, 1.000}; // #3D3D42
inline constexpr ImVec4 kKeyBgActive{0.055, 0.388, 0.612, 1.000}; // #0E639C
inline constexpr ImVec4 kShadow{0.000, 0.000, 0.000, 0.549}; // #0000008C
inline constexpr ImVec4 kShadowSoft{0.000, 0.000, 0.000, 0.314}; // #00000050

// ---- 尺寸（720p 设计空间，后端按 UiScale 统一放大） -------------------------
// 基准是「720p 手持屏」：字号/行高/间距都按在 6 寸屏上握着看设计，
// 不是桌面显示器上看的比例。改这里就能整体调整密度。
inline constexpr float kFontTitle  = 26.0f;
inline constexpr float kFontHeader = 19.0f;
inline constexpr float kFontBody   = 17.0f;
inline constexpr float kFontSmall  = 13.0f;
inline constexpr float kFontTiny   = 11.0f;

inline constexpr float kRadiusNone  = 0.0f;
inline constexpr float kRadiusSmall = 3.0f;
inline constexpr float kRadius      = 6.0f;
inline constexpr float kRadiusLarge = 12.0f;

inline constexpr float kGapSmall = 6.0f;
inline constexpr float kGap      = 10.0f;
inline constexpr float kGapLarge = 16.0f;

inline constexpr float kControlHeight = 34.0f;
inline constexpr float kListRowHeight = 32.0f;
inline constexpr float kKeySize       = 30.0f;

// 720p 手持基准下几个常用的「页面骨架」尺寸
inline constexpr float kHudHeight      = 36.0f;
inline constexpr float kTabColumnWidth = 186.0f;
inline constexpr float kPagePadding    = 18.0f;

// 把调色板套到 ImGui 默认样式上（用到标准控件时视觉一致）。
void ApplyToImGui();

// ---- 自检：RGBA 字面量必须精确还原成原来的十六进制 ---------------------------
// （换表示形式不改颜色，编译期就能发现手抖写错的分量）
static_assert(U32(RGBA(0x1E1E1EFF)) == IM_COL32(0x1E, 0x1E, 0x1E, 0xFF), "RGBA/U32 换算要与 IM_COL32 一致");
static_assert(U32(kBgEditor) == IM_COL32(0x1E, 0x1E, 0x1E, 0xFF), "kBgEditor");
static_assert(U32(kBgWidget) == IM_COL32(0x2D, 0x2D, 0x30, 0xFF), "kBgWidget");
static_assert(U32(kTextPrimary) == IM_COL32(0xD4, 0xD4, 0xD4, 0xFF), "kTextPrimary");
static_assert(U32(kAccent) == IM_COL32(0x00, 0x7A, 0xCC, 0xFF), "kAccent");
static_assert(U32(kScrim) == IM_COL32(0x08, 0x08, 0x0A, 0xC8), "kScrim（alpha 也要一致）");
static_assert(U32(kShadow) == IM_COL32(0x00, 0x00, 0x00, 0x8C), "kShadow（alpha）");
static_assert(U32(kAccent, 0.5f) == IM_COL32(0x00, 0x7A, 0xCC, 0x80), "U32(color, alpha) 按倍率改 alpha");

} // namespace gui_dev::cv::Theme
