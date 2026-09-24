// DrawList 绘制原语：斜切几何、不规则边缘、扫描、文字排版。
//
// 整个菜单不提交任何 ImGui 控件（Button/Selectable/TabItem 一个都不用），
// 全部通过 DrawList 自绘；因此也不会有 ImGui 默认视觉与 Nav 焦点环。
// AA 依赖 ImDrawList 自带的 AntiAliasedFill / AntiAliasedLines。
#pragma once

#include <imgui.h>

#include <cstdint>

#include "gamemenu/GameMenuTheme.h"

namespace gui_dev::gamemenu {

struct Rect {
    ImVec2 min{};
    ImVec2 max{};

    float Width() const { return max.x - min.x; }
    float Height() const { return max.y - min.y; }
    ImVec2 Size() const { return ImVec2(Width(), Height()); }
    ImVec2 Center() const { return ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f); }
    bool Contains(const ImVec2& p) const {
        return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y;
    }
    Rect Inflated(float x, float y) const { return Rect{ImVec2(min.x - x, min.y - y), ImVec2(max.x + x, max.y + y)}; }
    Rect Offset(float x, float y) const { return Rect{ImVec2(min.x + x, min.y + y), ImVec2(max.x + x, max.y + y)}; }
};

inline Rect MakeRect(float x, float y, float w, float h) {
    return Rect{ImVec2(x, y), ImVec2(x + w, y + h)};
}

// ---- 颜色 ------------------------------------------------------------------

// 乘 alpha（保留原 alpha 的比例）
ImU32 ColorWithAlpha(ImU32 col, float alpha);
// 线性混色（t=0 取 a，t=1 取 b）
ImU32 ColorLerp(ImU32 a, ImU32 b, float t);

// ---- 斜切几何 --------------------------------------------------------------

// 平行四边形：左上右移、右下左移（需求 §7 的 p0..p3）。
// 返回的 4 点顺序为 顺时针：左上, 右上, 右下, 左下。
int SkewQuadPoints(const Rect& rect, float skew, ImVec2 out[4]);

// 只斜左边缘（右边缘垂直），用于红色焦点层从左侧扩张。
int SkewQuadLeftPoints(const Rect& rect, float skew, ImVec2 out[4]);

void AddSkewFilled(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col);
void AddSkewFilledLeft(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col);
void AddSkewBorder(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col, float thickness);

// ---- 装饰 ------------------------------------------------------------------

// 不规则（锯齿）分隔线：分组之间用它代替文字标题。
void AddJaggedRule(ImDrawList* draw_list, float x0, float x1, float y, float amplitude, float tooth,
                   ImU32 col);

// 扫描高光：progress 0->1 时一条窄斜条从左边扫到右边。
void AddSweep(ImDrawList* draw_list, const Rect& rect, float skew, float progress, float band_width,
              ImU32 col);

// 四角 L 形装饰（焦点框）
void AddCornerTicks(ImDrawList* draw_list, const Rect& rect, float length, float thickness, ImU32 col);

// 菱形强调块（◆），用于标题与分类前缀。
void AddDiamond(ImDrawList* draw_list, ImVec2 center, float size, ImU32 col);

// ---- 文字 ------------------------------------------------------------------

float TextWidth(float size, const char* text);
float TextHeight(float size);

void AddTextLeft(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float size, const char* text);
void AddTextRight(ImDrawList* draw_list, ImVec2 right_pos, ImU32 col, float size, const char* text);
void AddTextCentered(ImDrawList* draw_list, const Rect& rect, ImU32 col, float size, const char* text);

// 垂直居中的左对齐文字（pos.y 视为该行的垂直中心）
void AddTextLeftVCentered(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float size, const char* text);

} // namespace gui_dev::gamemenu
