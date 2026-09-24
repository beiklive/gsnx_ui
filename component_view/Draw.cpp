#include "component_view/Draw.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "component_view/Theme.h"

namespace gui_dev::cv::Draw {
namespace {

// 四角圆角差距较大时 imgui 只能取一个统一半径，这里取最大值并只开需要圆角的角。
float ResolveRadius(float tl, float tr, float bl, float br, ImDrawFlags flags) {
    if (flags == ImDrawFlags_RoundCornersNone) {
        return 0.0f;
    }
    if (flags == 0) {
        return Maxf(Maxf(tl, tr), Maxf(bl, br));
    }
    float radius = 0.0f;
    if (flags & ImDrawFlags_RoundCornersTopLeft) {
        radius = Maxf(radius, tl);
    }
    if (flags & ImDrawFlags_RoundCornersTopRight) {
        radius = Maxf(radius, tr);
    }
    if (flags & ImDrawFlags_RoundCornersBottomLeft) {
        radius = Maxf(radius, bl);
    }
    if (flags & ImDrawFlags_RoundCornersBottomRight) {
        radius = Maxf(radius, br);
    }
    return radius;
}

} // namespace

ImDrawFlags CornerFlags(float tl, float tr, float bl, float br) {
    const float max_corner = MaxCorner(tl, tr, bl, br);
    if (max_corner <= 0.01f) {
        return ImDrawFlags_RoundCornersNone;
    }
    if (Absf(tl - max_corner) < 0.01f && Absf(tr - max_corner) < 0.01f && Absf(bl - max_corner) < 0.01f &&
        Absf(br - max_corner) < 0.01f) {
        return 0; // 0 = 四角全圆
    }
    ImDrawFlags flags = ImDrawFlags_RoundCornersNone;
    if (tl > 0.01f) {
        flags |= ImDrawFlags_RoundCornersTopLeft;
    }
    if (tr > 0.01f) {
        flags |= ImDrawFlags_RoundCornersTopRight;
    }
    if (bl > 0.01f) {
        flags |= ImDrawFlags_RoundCornersBottomLeft;
    }
    if (br > 0.01f) {
        flags |= ImDrawFlags_RoundCornersBottomRight;
    }
    return flags;
}

float MaxCorner(float tl, float tr, float bl, float br) {
    return Maxf(Maxf(Maxf(tl, tr), Maxf(bl, br)), 0.0f);
}

void RoundedRectFilled(ImDrawList* dl, const Rect& r, ImU32 color, float tl, float tr, float bl, float br) {
    if (dl == nullptr || !r.Valid() || ((color >> IM_COL32_A_SHIFT) & 0xFF) == 0) {
        return;
    }
    const ImDrawFlags flags = CornerFlags(tl, tr, bl, br);
    const float radius = ResolveRadius(tl, tr, bl, br, flags);
    dl->AddRectFilled(r.min, r.max, color, radius, flags);
}

void RoundedRectOutline(ImDrawList* dl, const Rect& r, ImU32 color, float thickness, float tl, float tr, float bl,
                        float br) {
    if (dl == nullptr || !r.Valid() || thickness <= 0.0f || ((color >> IM_COL32_A_SHIFT) & 0xFF) == 0) {
        return;
    }
    const ImDrawFlags flags = CornerFlags(tl, tr, bl, br);
    const float radius = ResolveRadius(tl, tr, bl, br, flags);
    dl->AddRect(r.min, r.max, color, radius, flags, thickness);
}

void SoftShadow(ImDrawList* dl, const Rect& r, const ShadowStyle& style, float tl, float tr, float bl, float br) {
    if (dl == nullptr || !r.Valid() || !style.enabled) {
        return;
    }
    const float alpha = static_cast<float>((style.color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
    if (alpha <= 0.002f) {
        return;
    }
    const ImVec2 offset = style.offset;
    const float blur = Maxf(style.blur, 0.0f);
    const float spread = style.spread;

    if (blur < 0.5f) {
        RoundedRectFilled(dl, r.Translate(offset).Expanded(spread), style.color, tl, tr, bl, br);
        return;
    }

    // 层数随模糊半径增长，最多 24 层：越靠外的层 alpha 越低、扩张越大。
    const int steps = static_cast<int>(Clampf(blur * 0.75f, 6.0f, 24.0f));
    for (int i = steps; i >= 1; --i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps); // 1 = 最外层
        const float grow = t * blur * 0.5f + spread;
        const float layer_alpha = alpha * (1.0f - t) * (1.0f - t) * 2.0f / static_cast<float>(steps);
        const Rect layer = r.Translate(offset).Expanded(grow);
        RoundedRectFilled(dl, layer, Theme::Alpha(style.color, layer_alpha), tl + grow, tr + grow, bl + grow,
                          br + grow);
    }
    // 主体：保证阴影在本体正下方仍然是实心的
    RoundedRectFilled(dl, r.Translate(offset).Expanded(spread), Theme::Alpha(style.color, alpha * 0.85f), tl, tr, bl,
                      br);
}

ImFont* CurrentFont() {
    ImFont* font = ImGui::GetFont();
    return font != nullptr ? font : ImGui::GetIO().FontDefault;
}

ImVec2 MeasureText(ImFont* font, float font_size, const char* text, float wrap_width) {
    const float size = font_size > 0.0f ? font_size : ImGui::GetFontSize();
    if (text == nullptr || text[0] == '\0') {
        return ImVec2(0.0f, size);
    }
    ImFont* use = font != nullptr ? font : CurrentFont();
    if (use == nullptr) {
        return ImVec2(0.0f, size);
    }
    return use->CalcTextSizeA(size, FLT_MAX, wrap_width, text);
}

void Text(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text,
          float wrap_width) {
    if (dl == nullptr || text == nullptr || text[0] == '\0') {
        return;
    }
    if (font_size <= 0.0f) {
        return; // 防御：字号为 0 会让 imgui 断言
    }
    ImFont* use = font != nullptr ? font : CurrentFont();
    const float size = font_size > 0.0f ? font_size : ImGui::GetFontSize();
    dl->AddText(use, size, pos, color, text, nullptr, wrap_width);
}

void TextOutlined(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, ImU32 outline_color,
                  const char* text, float wrap_width) {
    if (dl == nullptr || text == nullptr || text[0] == '\0') {
        return;
    }
    if (((outline_color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
        for (int dy = -1; dy <= 1; dy += 2) {
            for (int dx = -1; dx <= 1; dx += 2) {
                Text(dl, font, font_size, ImVec2(pos.x + static_cast<float>(dx), pos.y + static_cast<float>(dy)),
                     outline_color, text, wrap_width);
            }
        }
    }
    Text(dl, font, font_size, pos, color, text, wrap_width);
}

const char* Ellipsize(ImFont* font, float font_size, const char* text, float max_width) {
    static char buffers[4][256];
    static int slot = 0;
    char* out = buffers[slot];
    slot = (slot + 1) % 4;

    if (text == nullptr) {
        out[0] = '\0';
        return out;
    }
    const ImVec2 full = MeasureText(font, font_size, text, 0.0f);
    // 容差 0.5px：避免因为浮点误差把刚好放得下的文本省略掉
    if (max_width <= 0.0f || full.x <= max_width + 0.5f) {
        std::snprintf(out, 256, "%s", text);
        return out;
    }
    const float ellipsis_width = MeasureText(font, font_size, "\xE2\x80\xA6", 0.0f).x;
    const float budget = max_width - ellipsis_width;
    if (budget <= 1.0f) {
        std::snprintf(out, 256, "\xE2\x80\xA6");
        return out;
    }
    // 逐字符累加（UTF-8 连续字节不切断）
    std::size_t cut = 0;
    std::size_t index = 0;
    while (text[index] != '\0') {
        std::size_t next = index + 1;
        while (text[next] != '\0' && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
            ++next;
        }
        const std::size_t length = next - index;
        if (length >= 255) {
            break;
        }
        char candidate[256];
        std::memcpy(candidate, text, next);
        candidate[next] = '\0';
        if (MeasureText(font, font_size, candidate, 0.0f).x > budget) {
            break;
        }
        cut = next;
        index = next;
    }
    const std::size_t copy = cut < 250 ? cut : 250;
    std::memcpy(out, text, copy);
    std::snprintf(out + copy, 256 - copy, "\xE2\x80\xA6");
    return out;
}

void CheckMark(ImDrawList* dl, const Rect& box, ImU32 color, float thickness, float t) {
    if (dl == nullptr || t <= 0.01f) {
        return;
    }
    t = Clampf(t, 0.0f, 1.0f);
    const ImVec2 a(box.min.x + box.Width() * 0.22f, box.min.y + box.Height() * 0.52f);
    const ImVec2 b(box.min.x + box.Width() * 0.42f, box.min.y + box.Height() * 0.72f);
    const ImVec2 c(box.min.x + box.Width() * 0.78f, box.min.y + box.Height() * 0.28f);
    const ImVec2 points[3] = {a, b, c};
    // 分两段画，按 t 推进：0..0.45 画第一笔，0.45..1 画第二笔
    const float first = Clampf(t / 0.45f, 0.0f, 1.0f);
    const float second = Clampf((t - 0.45f) / 0.55f, 0.0f, 1.0f);
    dl->AddLine(a, ImVec2(a.x + (b.x - a.x) * first, a.y + (b.y - a.y) * first), color, thickness);
    if (second > 0.0f) {
        dl->AddLine(b, ImVec2(b.x + (c.x - b.x) * second, b.y + (c.y - b.y) * second), color, thickness);
    }
    (void)points;
}

void TriangleRight(ImDrawList* dl, const ImVec2& center, float size, ImU32 color) {
    if (dl == nullptr) {
        return;
    }
    const float half = size * 0.5f;
    dl->AddTriangleFilled(ImVec2(center.x - half * 0.6f, center.y - half),
                          ImVec2(center.x - half * 0.6f, center.y + half), ImVec2(center.x + half * 0.8f, center.y),
                          color);
}

void RoundedRectVerticalGradient(ImDrawList* dl, const Rect& r, ImU32 top, ImU32 bottom, float rounding) {
    if (dl == nullptr || !r.Valid()) {
        return;
    }
    dl->AddRectFilledMultiColor(r.min, r.max, top, top, bottom, bottom);
    (void)rounding;
}

} // namespace gui_dev::cv::Draw
