// 组件库调色板与尺寸规范：VSCode Dark+ 配色（背景不使用纯黑）。
//
// 颜色写法跟 CSS 一样，分量都是 **0..255**：
//   rgb(r, g, b)        不透明
//   rgba(r, g, b, a)    带 alpha（a=255 时等同于 rgb）
// 返回类型是 ImVec4（分量 0..1），和 ImGui 的样式系统一致；要交给
// ImDrawList / Widget 的 ImU32 属性时用 Theme::U32(color) 转换。
#pragma once

#include <imgui.h>

namespace gui_dev::cv::Theme {

// ---- 颜色工具（都能在编译期求值） ------------------------------------------
inline constexpr int ClampByte(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// rgba(r, g, b, a)：分量 0..255（越界会被夹到 0..255），返回 ImVec4（0..1）
inline constexpr ImVec4 rgba(int r, int g, int b, int a = 255) {
    return ImVec4(static_cast<float>(ClampByte(r)) / 255.0f, static_cast<float>(ClampByte(g)) / 255.0f,
                  static_cast<float>(ClampByte(b)) / 255.0f, static_cast<float>(ClampByte(a)) / 255.0f);
}

// rgb(r, g, b)：不透明版本
inline constexpr ImVec4 rgb(int r, int g, int b) { return rgba(r, g, b, 255); }

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

// ---- VSCode Dark+ 调色板 ---------------------------------------------------
// 编辑器底色 rgb(30, 30, 30) 而不是纯黑：纯黑在大屏上对比过强、边缘有光晕感。
inline constexpr ImVec4 kBgEditor = rgb(189, 186, 186); // #BDBABA（原 VSCode 底色是 rgb(30,30,30) #1E1E1E）
inline constexpr ImVec4 kBgSideBar = rgb(37, 37, 38); // #252526
inline constexpr ImVec4 kBgPanel = rgb(37, 37, 38); // #252526
inline constexpr ImVec4 kBgActivity = rgb(51, 51, 51); // #333333
inline constexpr ImVec4 kBgWidget = rgb(45, 45, 48); // #2D2D30
inline constexpr ImVec4 kBgWidgetHi = rgb(55, 55, 58); // #37373A
inline constexpr ImVec4 kBgInput = rgb(60, 60, 60); // #3C3C3C
inline constexpr ImVec4 kBorder = rgb(60, 60, 60); // #3C3C3C
inline constexpr ImVec4 kBorderStrong = rgb(84, 84, 88); // #545458
inline constexpr ImVec4 kTextPrimary = rgb(212, 212, 212); // #D4D4D4
inline constexpr ImVec4 kTextBright = rgb(255, 255, 255); // #FFFFFF
inline constexpr ImVec4 kTextMuted = rgb(133, 133, 133); // #858585
inline constexpr ImVec4 kTextDisabled = rgb(106, 106, 106); // #6A6A6A
inline constexpr ImVec4 kAccent = rgb(0, 122, 204); // #007ACC  // VSCode 焦点蓝
inline constexpr ImVec4 kAccentHover = rgb(17, 119, 187); // #1177BB
inline constexpr ImVec4 kButton = rgb(14, 99, 156); // #0E639C
inline constexpr ImVec4 kButtonActive = rgb(10, 75, 119); // #0A4B77
inline constexpr ImVec4 kSelection = rgb(38, 79, 120); // #264F78
inline constexpr ImVec4 kError = rgb(241, 76, 76); // #F14C4C
inline constexpr ImVec4 kWarning = rgb(204, 167, 0); // #CCA700
inline constexpr ImVec4 kTeal = rgb(78, 201, 176); // #4EC9B0
inline constexpr ImVec4 kOrange = rgb(206, 145, 120); // #CE9178
inline constexpr ImVec4 kPurple = rgb(197, 134, 192); // #C586C0
inline constexpr ImVec4 kYellow = rgb(220, 220, 170); // #DCDCAA
inline constexpr ImVec4 kBlue = rgb(86, 156, 214); // #569CD6
inline constexpr ImVec4 kGreen = rgb(106, 153, 85); // #6A9955
inline constexpr ImVec4 kWhite = rgb(255, 255, 255); // #FFFFFF
inline constexpr ImVec4 kSuccess = kTeal;
inline constexpr ImVec4 kTrack = rgb(58, 58, 61); // #3A3A3D  // 滑条 / 进度条底色
inline constexpr ImVec4 kTrackFill = rgb(0, 122, 204); // #007ACC
inline constexpr ImVec4 kScrim = rgba(8, 8, 10, 200); // #08080AC8  // 弹层遮罩
inline constexpr ImVec4 kListRow = rgb(42, 42, 45); // #2A2A2D
inline constexpr ImVec4 kListRowAlt = rgb(36, 36, 39); // #242427
inline constexpr ImVec4 kListRowFocus = rgb(38, 79, 120); // #264F78
inline constexpr ImVec4 kKeyBg = rgb(51, 51, 55); // #333337  // 虚拟键盘按键
inline constexpr ImVec4 kKeyBgTop = rgb(61, 61, 66); // #3D3D42
inline constexpr ImVec4 kKeyBgActive = rgb(14, 99, 156); // #0E639C
inline constexpr ImVec4 kShadow = rgba(0, 0, 0, 140); // #0000008C
inline constexpr ImVec4 kShadowSoft = rgba(0, 0, 0, 80); // #00000050

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

// ---- 自检：只验证 rgb()/rgba()/U32 这套换算本身是对的 ------------------------
// 注意：这里**不**钉死调色板的具体颜色 —— 配色是你随时要改的东西，
// 改了颜色不该编译不过。改颜色时记得同步后面的十六进制注释
// （或者直接写成 rgb(0xBD, 0xBA, 0xBA)，值本身就是注释）。
static_assert(U32(rgb(30, 30, 30)) == IM_COL32(0x1E, 0x1E, 0x1E, 0xFF), "rgb() -> ImU32");
static_assert(U32(rgba(8, 8, 10, 200)) == IM_COL32(0x08, 0x08, 0x0A, 0xC8), "rgba() -> ImU32（含 alpha）");
static_assert(rgba(300, -5, 30, 999).x == 1.0f && rgba(300, -5, 30, 999).y == 0.0f, "分量越界会夹到 0..255");
static_assert(U32(rgb(0, 122, 204), 0.5f) == IM_COL32(0x00, 0x7A, 0xCC, 0x80), "U32(color, alpha) 按倍率改 alpha");

} // namespace gui_dev::cv::Theme
