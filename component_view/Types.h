// 组件库公共类型：矩形、边距、边框、阴影、布局枚举。
// 只依赖 imgui，不依赖任何平台头文件，保证 mac / switch 共用。
#pragma once

#include <cstdint>

#include <imgui.h>

namespace gui_dev::cv {

// ---- 组件唯一 ID ----------------------------------------------------------
using WidgetId = std::uint64_t;

// ---- 数值工具（不引 imgui_internal.h，保持只用公开头文件） ------------------
inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Maxf(float a, float b) { return a > b ? a : b; }
inline float Minf(float a, float b) { return a < b ? a : b; }
inline float Absf(float v) { return v < 0.0f ? -v : v; }

// ---- 矩形（屏幕坐标，左上角为原点） ----------------------------------------
struct Rect {
    ImVec2 min{0.0f, 0.0f};
    ImVec2 max{0.0f, 0.0f};

    static Rect FromPosSize(const ImVec2& pos, const ImVec2& size) {
        return Rect{pos, ImVec2(pos.x + size.x, pos.y + size.y)};
    }
    float Width() const { return max.x - min.x; }
    float Height() const { return max.y - min.y; }
    ImVec2 Pos() const { return min; }
    ImVec2 Size() const { return ImVec2(Width(), Height()); }
    ImVec2 Center() const { return ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f); }
    bool Contains(const ImVec2& p) const {
        return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y;
    }
    bool Valid() const { return max.x > min.x && max.y > min.y; }
    Rect Inset(float l, float t, float r, float b) const {
        return Rect{ImVec2(min.x + l, min.y + t), ImVec2(max.x - r, max.y - b)};
    }
    Rect Expanded(float v) const { return Inset(-v, -v, -v, -v); }
    Rect Translate(const ImVec2& d) const {
        return Rect{ImVec2(min.x + d.x, min.y + d.y), ImVec2(max.x + d.x, max.y + d.y)};
    }
};

// ---- 边距（外 margin / 内 padding 共用同一结构） ---------------------------
struct EdgeInsets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    static EdgeInsets All(float v) { return EdgeInsets{v, v, v, v}; }
    static EdgeInsets Symmetric(float h, float v) { return EdgeInsets{h, v, h, v}; }
    static EdgeInsets XY(float x, float y) { return EdgeInsets{x, y, x, y}; }

    float Horizontal() const { return left + right; }
    float Vertical() const { return top + bottom; }
    ImVec2 TopLeft() const { return ImVec2(left, top); }
};

// ---- 边框 ----------------------------------------------------------------
struct BorderStyle {
    float width = 0.0f;
    ImU32 color = 0;
    float inset = 0.0f; // 向矩形内部收缩的像素（画在内部而不是骑在边上）

    bool Visible() const { return width > 0.0f && ((color >> IM_COL32_A_SHIFT) & 0xFF) != 0; }
};

// ---- 阴影 ----------------------------------------------------------------
struct ShadowStyle {
    bool enabled = false;
    ImVec2 offset{0.0f, 4.0f};
    float blur = 18.0f;   // 柔化半径（0 = 硬阴影）
    float spread = 0.0f;  // 额外扩张
    ImU32 color = IM_COL32(0, 0, 0, 110);

    static ShadowStyle Soft(float blur_value = 18.0f) {
        ShadowStyle s;
        s.enabled = true;
        s.blur = blur_value;
        return s;
    }
};

// ---- 布局 ----------------------------------------------------------------
enum class LayoutMode {
    Free,      // 子节点用自身 position/anchor/pivot 定位
    Vertical,  // 从上往下排列，间距 gap.y
    Horizontal // 从左往右排列，间距 gap.x
};

enum class Align { Start, Center, End, Stretch };
enum class TextAlign { Left, Center, Right };
enum class VerticalAlign { Top, Middle, Bottom };

} // namespace gui_dev::cv
