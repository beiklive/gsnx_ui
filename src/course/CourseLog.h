// 跨场景共享的生命周期日志（课时 2 用）。
//
// 为什么需要它：`OnLeave` 发生在场景**析构时**，场景自己的成员日志会跟着一起消失，
// 屏上根本看不到。把它放到场景之外，才能看到真实顺序：
//     课时 1 的 OnLeave  ->  课时 2 的 OnEnter
//
// 实现要点（也顺便示范本项目的性能约束）：
//   · 固定容量环形缓冲，无任何堆分配；
//   · 行文本用固定 char[]，时间戳来自 ImGui::GetTime()，帧号来自 GetFrameCount()。
#pragma once

#include <cstddef>

namespace gui_dev::course {

struct CourseLogLine {
    char text[80] = {};
    float time = 0.0f;  // 记录时刻（ImGui 时间，秒）
    int frame = 0;      // 记录时的帧号
};

inline constexpr int kCourseLogCapacity = 16;

// 追加一行（printf 风格）。容量满时覆盖最旧的一行。
void LogEvent(const char* fmt, ...);

// index 0 = 最旧的一行
int LogLineCount();
const CourseLogLine& LogLineAt(int index);
void ClearLog();

} // namespace gui_dev::course
