// 8 个 Tab 的绘制函数（定义在 TourTabsBasic.cpp / TourTabsAdvanced.cpp）
#pragma once

#include "examples/imgui_tour/ImGuiTourApp.h"

namespace gui_dev::tour {

void DrawOverviewTab(State& s);   // 文本 / 按钮 / 勾选 / 进度 / 提示
void DrawInputsTab(State& s);     // Input / Drag / Slider / Combo / Color
void DrawLayoutTab(State& s);     // Tree / Child / Table / 嵌套 Tab / Splitter
void DrawPopupsTab(State& s);     // Popup / Modal / Menu / 右键菜单 / Tooltip
void DrawAdvancedTab(State& s);   // DragDrop / ListClipper / MultiSelect
void DrawDrawingTab(State& s);    // ImDrawList 全部图元 + 纹理 + 裁剪 + 回调
void DrawFontStyleTab(State& s);  // 字体列表 / 动态字号 / 全局缩放 / 样式编辑
void DrawSystemTab(State& s);     // IO 统计 / 按键状态 / Storage / 日志 / 调试窗口

} // namespace gui_dev::tour
