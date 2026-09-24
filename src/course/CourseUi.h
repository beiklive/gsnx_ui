// 课程页面的公共 UI 片段（所有课时共用，避免每课重复一遍样板）。
//
// 注意：这些只是"课程讲义"的排版工具，用的是 ImGui 内建控件 —— 课程里允许，
// 但**正式控件**必须按课时 5 讲的「占位 + 自绘」来做。
#pragma once

#include <utility>
#include <vector>

#include "ui/Scene.h"

namespace gui_dev::course {

// 小节标题（红字 + 分隔线）
void Section(const char* title);
// 灰色正文（printf 风格）
void Note(const char* fmt, ...);
// 代码块：灰底 + 绿字 + 自动行数高度
void Code(const char* lines);
// 项目符号
void Bullet(const char* fmt, ...);
// 键值行（左侧说明，右侧实时值）
void KeyValue(const char* key, const char* fmt, ...);
// 键值行，指定左列宽度
void KeyValueW(float key_width, const char* key, const char* fmt, ...);

// 标准页脚提示（L 上一课 / R 下一课 / B 退出）；每帧重建，指针始终有效
const std::vector<std::pair<const char*, const char*>>& StandardFooter();
// 等价于 Components::EndPanel(ui, StandardFooter())
void EndLesson(UiContext& ui);

} // namespace gui_dev::course
