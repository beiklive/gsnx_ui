// 绘制工具：圆角矩形、软阴影、文本。组件只通过这里落笔，方便统一改视觉。
#pragma once

#include "component_view/Types.h"

namespace gui_dev::cv::Draw {

// 四角圆角 -> ImDrawFlags（四角相同时返回 0，imgui 约定 0 = 四角全圆）
ImDrawFlags CornerFlags(float tl, float tr, float bl, float br);
float MaxCorner(float tl, float tr, float bl, float br);

// 填充圆角矩形
void RoundedRectFilled(ImDrawList* dl, const Rect& r, ImU32 color, float tl, float tr, float bl, float br);
// 描边圆角矩形（厚度向内外各半）
void RoundedRectOutline(ImDrawList* dl, const Rect& r, ImU32 color, float thickness, float tl, float tr, float bl,
                        float br);
// 软阴影：用多层递减 alpha 的圆角矩形逼近高斯模糊（不依赖后处理）
void SoftShadow(ImDrawList* dl, const Rect& r, const ShadowStyle& style, float tl, float tr, float bl, float br);

// 文本尺寸测量（font 为 nullptr 时用当前字体）
ImVec2 MeasureText(ImFont* font, float font_size, const char* text, float wrap_width = 0.0f);
// 文本绘制（返回绘制后的右下角）
void Text(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text,
          float wrap_width = 0.0f);
// 取当前 ImGui 上下文的主字体
ImFont* CurrentFont();

} // namespace gui_dev::cv::Draw
