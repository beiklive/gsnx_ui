#include "gamemenu/GameMenuDraw.h"

#include <cmath>
#include <cfloat>

namespace gui_dev::gamemenu {
namespace {

// 斜切量不能超过半宽，否则窄按钮会退化成自交多边形（AA 填充会画错）。
inline float SafeSkew(const Rect& rect, float skew) {
    const float half = rect.Width() * 0.5f;
    if (skew > half) {
        return half;
    }
    return skew < 0.0f ? 0.0f : skew;
}

inline ImVec2 Lerp2(const ImVec2& a, const ImVec2& b, float t) {
    return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

} // namespace

ImU32 ColorWithAlpha(ImU32 col, float alpha) {
    const float a = Clamp01(alpha) * static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF);
    return (col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
}

ImU32 ColorLerp(ImU32 a, ImU32 b, float t) {
    const float x = Clamp01(t);
    const auto mix = [x](ImU32 p, ImU32 q) {
        const float pf = static_cast<float>(p);
        const float qf = static_cast<float>(q);
        return static_cast<ImU32>(pf + (qf - pf) * x);
    };
    return IM_COL32(mix((a >> IM_COL32_R_SHIFT) & 0xFF, (b >> IM_COL32_R_SHIFT) & 0xFF),
                    mix((a >> IM_COL32_G_SHIFT) & 0xFF, (b >> IM_COL32_G_SHIFT) & 0xFF),
                    mix((a >> IM_COL32_B_SHIFT) & 0xFF, (b >> IM_COL32_B_SHIFT) & 0xFF),
                    mix((a >> IM_COL32_A_SHIFT) & 0xFF, (b >> IM_COL32_A_SHIFT) & 0xFF));
}

int SkewQuadPoints(const Rect& rect, float skew, ImVec2 out[4]) {
    const float s = SafeSkew(rect, skew);
    out[0] = ImVec2(rect.min.x + s, rect.min.y);
    out[1] = ImVec2(rect.max.x, rect.min.y);
    out[2] = ImVec2(rect.max.x - s, rect.max.y);
    out[3] = ImVec2(rect.min.x, rect.max.y);
    return 4;
}

int SkewQuadLeftPoints(const Rect& rect, float skew, ImVec2 out[4]) {
    const float s = SafeSkew(rect, skew);
    out[0] = ImVec2(rect.min.x + s, rect.min.y);
    out[1] = ImVec2(rect.max.x, rect.min.y);
    out[2] = ImVec2(rect.max.x, rect.max.y);
    out[3] = ImVec2(rect.min.x, rect.max.y);
    return 4;
}

void AddSkewFilled(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col) {
    ImVec2 points[4];
    const int count = SkewQuadPoints(rect, skew, points);
    draw_list->AddConvexPolyFilled(points, count, col);
}

void AddSkewFilledLeft(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col) {
    ImVec2 points[4];
    const int count = SkewQuadLeftPoints(rect, skew, points);
    draw_list->AddConvexPolyFilled(points, count, col);
}

void AddSkewBorder(ImDrawList* draw_list, const Rect& rect, float skew, ImU32 col, float thickness) {
    ImVec2 points[4];
    const int count = SkewQuadPoints(rect, skew, points);
    draw_list->AddPolyline(points, count, col, ImDrawFlags_Closed, thickness);
}

void AddJaggedRule(ImDrawList* draw_list, float x0, float x1, float y, float amplitude, float tooth,
                   ImU32 col) {
    if (tooth <= 0.5f || x1 - x0 < tooth) {
        draw_list->AddLine(ImVec2(x0, y), ImVec2(x1, y), col, 1.0f);
        return;
    }
    draw_list->PathClear();
    bool up = true;
    for (float x = x0; x <= x1; x += tooth) {
        draw_list->PathLineTo(ImVec2(x, up ? y - amplitude : y + amplitude));
        up = !up;
    }
    draw_list->PathLineTo(ImVec2(x1, y));
    draw_list->PathStroke(col, ImDrawFlags_None, 1.0f);
}

void AddSweep(ImDrawList* draw_list, const Rect& rect, float skew, float progress, float band_width,
              ImU32 col) {
    const float p = Clamp01(progress);
    const float travel = rect.Width() + band_width * 2.0f;
    const float x = rect.min.x - band_width + travel * p;
    Rect band = MakeRect(x, rect.min.y, band_width, rect.Height());
    // 裁到按钮范围内，避免扫出按钮外
    if (band.min.x < rect.min.x) {
        band.min.x = rect.min.x;
    }
    if (band.max.x > rect.max.x) {
        band.max.x = rect.max.x;
    }
    if (band.Width() <= 1.0f) {
        return;
    }
    AddSkewFilled(draw_list, band, skew * 0.6f, col);
}

void AddCornerTicks(ImDrawList* draw_list, const Rect& rect, float length, float thickness, ImU32 col) {
    const ImVec2 a = rect.min;
    const ImVec2 b = rect.max;
    draw_list->AddLine(ImVec2(a.x, a.y), ImVec2(a.x + length, a.y), col, thickness);
    draw_list->AddLine(ImVec2(a.x, a.y), ImVec2(a.x, a.y + length), col, thickness);
    draw_list->AddLine(ImVec2(b.x - length, a.y), ImVec2(b.x, a.y), col, thickness);
    draw_list->AddLine(ImVec2(b.x, a.y), ImVec2(b.x, a.y + length), col, thickness);
    draw_list->AddLine(ImVec2(a.x, b.y - length), ImVec2(a.x, b.y), col, thickness);
    draw_list->AddLine(ImVec2(a.x, b.y), ImVec2(a.x + length, b.y), col, thickness);
    draw_list->AddLine(ImVec2(b.x - length, b.y), ImVec2(b.x, b.y), col, thickness);
    draw_list->AddLine(ImVec2(b.x, b.y - length), ImVec2(b.x, b.y), col, thickness);
}

void AddDiamond(ImDrawList* draw_list, ImVec2 center, float size, ImU32 col) {
    const ImVec2 points[4] = {
        ImVec2(center.x, center.y - size),
        ImVec2(center.x + size, center.y),
        ImVec2(center.x, center.y + size),
        ImVec2(center.x - size, center.y),
    };
    draw_list->AddConvexPolyFilled(points, 4, col);
}

float TextWidth(float size, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return 0.0f;
    }
    return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
}

float TextHeight(float size) {
    return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, "Ag").y;
}

void AddTextLeft(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float size, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    draw_list->AddText(ImGui::GetFont(), size, pos, col, text);
}

void AddTextRight(ImDrawList* draw_list, ImVec2 right_pos, ImU32 col, float size, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const float width = TextWidth(size, text);
    draw_list->AddText(ImGui::GetFont(), size, ImVec2(right_pos.x - width, right_pos.y), col, text);
}

void AddTextCentered(ImDrawList* draw_list, const Rect& rect, ImU32 col, float size, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    const ImVec2 pos(rect.min.x + (rect.Width() - extent.x) * 0.5f,
                     rect.min.y + (rect.Height() - extent.y) * 0.5f);
    draw_list->AddText(ImGui::GetFont(), size, pos, col, text);
}

void AddTextLeftVCentered(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float size, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    draw_list->AddText(ImGui::GetFont(), size, ImVec2(pos.x, pos.y - extent.y * 0.5f), col, text);
}

} // namespace gui_dev::gamemenu
