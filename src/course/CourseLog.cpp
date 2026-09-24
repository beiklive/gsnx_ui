#include "course/CourseLog.h"

#include <cstdarg>
#include <cstdio>

#include <imgui.h>

namespace gui_dev::course {
namespace {

CourseLogLine g_lines[kCourseLogCapacity];
int g_total = 0; // 累计写入行数（环形，用来算最旧一行的位置）

} // namespace

void LogEvent(const char* fmt, ...) {
    CourseLogLine& line = g_lines[g_total % kCourseLogCapacity];

    va_list args;
    va_start(args, fmt);
    std::vsnprintf(line.text, sizeof(line.text), fmt, args);
    va_end(args);

    line.time = static_cast<float>(ImGui::GetTime());
    line.frame = ImGui::GetFrameCount();
    ++g_total;
}

int LogLineCount() { return g_total < kCourseLogCapacity ? g_total : kCourseLogCapacity; }

const CourseLogLine& LogLineAt(int index) {
    const int oldest = g_total > kCourseLogCapacity ? g_total - kCourseLogCapacity : 0;
    return g_lines[(oldest + index) % kCourseLogCapacity];
}

void ClearLog() { g_total = 0; }

} // namespace gui_dev::course
