#include "course/CourseLessons.h"

namespace gui_dev::course {

const std::vector<CourseEntry>& CourseEntries() {
    // 顺序即课时顺序；用 L/R 在课时之间切换。
    static const std::vector<CourseEntry> entries = {
        {"课时 1 · 建立窗口", &CreateLesson01Window},
    };
    return entries;
}

} // namespace gui_dev::course
