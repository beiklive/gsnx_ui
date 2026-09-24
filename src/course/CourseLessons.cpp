#include "course/CourseLessons.h"

namespace gui_dev::course {

const std::vector<CourseEntry>& CourseEntries() {
    // 顺序即课时顺序；用 L/R 在课时之间切换。
    static const std::vector<CourseEntry> entries = {
        {"课时 1 · 建立窗口", &CreateLesson01Window},
        {"课时 2 · 生命周期", &CreateLesson02Lifecycle},
        {"课时 3 · 帧循环解剖", &CreateLesson03FrameLoop},
        {"课时 4 · UI 层级与布局", &CreateLesson04Hierarchy},
        {"课时 5 · 控件制作", &CreateLesson05Widget},
        {"课时 6 · 事件响应", &CreateLesson06Events},
        {"课时 7 · 状态管理", &CreateLesson07State},
        {"课时 8 · 动画系统", &CreateLesson08Animation},
        {"课时 9 · 布局自适应", &CreateLesson09Adaptive},
        {"课时 10 · 容器与复合控件", &CreateLesson10Container},
        {"课时 11 · 弹层与拖放", &CreateLesson11Popup},
        {"课时 12 · 性能与质量", &CreateLesson12Perf},
        {"课时 13 · 综合实战：游戏库列表", &CreateLesson13Library},
    };
    return entries;
}

} // namespace gui_dev::course
