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
// 单个字形（图标通常就是一个码位）的「墨迹」上下边界，相对行盒顶的偏移（已是 font_size 尺度）。
// 用途：图标要按看得见的形状居中 —— 行盒下方带 descender 空白，按行盒居中会显得偏上。
// 返回 false 表示取不到字形（回退到按行盒居中）。
bool GlyphInkExtent(ImFont* font, float font_size, const char* utf8_glyph, float& ink_top, float& ink_bottom);
// 文本绘制（返回绘制后的右下角）
void Text(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text,
          float wrap_width = 0.0f);
// 取当前 ImGui 上下文的主字体
ImFont* CurrentFont();

// ---- 流光聚焦框 -----------------------------------------------------------
// 沿圆角矩形边界画一圈「颜色在流动」的闭合边框：
//   rect       要框住的矩形（调用方自己按 margin 外扩）
//   thickness  边框粗细
//   phase      相位（0..1 循环，用 Global::time * speed 喂进来）
//   saturation / brightness  HSV 的 S/V（饱和度 0 = 白灰流光）
//   radius     圆角半径（默认 <0 时按 thickness*2 估；正常应传控件圆角 + 外扩量，才会贴着按钮的圆角走）
// 实现：把圆角矩形按弧长采样成点列，逐段画填充四边形，每段的色相 = 弧长比例 + phase
void FlowingRing(ImDrawList* dl, const Rect& rect, float thickness, float phase, float saturation = 0.75f,
                 float brightness = 1.0f, float alpha = 1.0f, float step = 3.0f, float radius = -1.0f);

// HSV -> ImU32（h/s/v 都是 0..1），流光用
ImU32 Hsv(float h, float s, float v, float alpha = 1.0f);

// ---- 文本增强 -------------------------------------------------------------
// 带 1px 描边（四向偏移）的文本：深色背景上更容易读。
void TextOutlined(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, ImU32 outline_color,
                  const char* text, float wrap_width = 0.0f);
// 按可用宽度裁切，放不下时用 "…" 结尾（返回的是内部轮转缓冲，别跨帧保存）。
const char* Ellipsize(ImFont* font, float font_size, const char* text, float max_width);
// 在 slot 里居中显示一行文字；放不下就横向循环滚动（跑马灯），超出部分按 slot 裁掉。
// phase 传 Global::time（秒），speed 是滚动速度 px/s。
void MarqueeText(ImDrawList* dl, ImFont* font, float font_size, const Rect& slot, ImU32 color, const char* text,
                 float phase, float speed = 26.0f);

// ---- 形状增强 -------------------------------------------------------------
// 对勾（复选框勾选动画用，t = 0..1 控制绘制进度）
void CheckMark(ImDrawList* dl, const Rect& box, ImU32 color, float thickness, float t);
// 右向三角（列表/菜单展开指示）
void TriangleRight(ImDrawList* dl, const ImVec2& center, float size, ImU32 color);
// 竖直/水平渐变填充（进度、滑条高亮用）
void RoundedRectVerticalGradient(ImDrawList* dl, const Rect& r, ImU32 top, ImU32 bottom, float rounding);

} // namespace gui_dev::cv::Draw
