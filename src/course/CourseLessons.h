// 课程注册表：每增加一课时，只需
//   1) 在本文件声明 CreateLessonNN()
//   2) 在 CourseLessons.cpp 的表里加一行
// 这样课时之间互不影响，改坏一课不影响其他课。
#pragma once

#include <memory>
#include <vector>

#include "ui/Scene.h"

namespace gui_dev::course {

struct CourseEntry {
    const char* title;                              // 屏上显示的课时标题
    std::unique_ptr<Scene> (*create)();             // 该课时的场景工厂
};

const std::vector<CourseEntry>& CourseEntries();

// ---- 各课时工厂 ------------------------------------------------------------
std::unique_ptr<Scene> CreateLesson01Window();
std::unique_ptr<Scene> CreateLesson02Lifecycle();
std::unique_ptr<Scene> CreateLesson03FrameLoop();
std::unique_ptr<Scene> CreateLesson04Hierarchy();
std::unique_ptr<Scene> CreateLesson05Widget();
std::unique_ptr<Scene> CreateLesson06Events();
std::unique_ptr<Scene> CreateLesson07State();
std::unique_ptr<Scene> CreateLesson08Animation();
std::unique_ptr<Scene> CreateLesson09Adaptive();
std::unique_ptr<Scene> CreateLesson10Container();
std::unique_ptr<Scene> CreateLesson11Popup();
std::unique_ptr<Scene> CreateLesson12Perf();
std::unique_ptr<Scene> CreateLesson13Library();

} // namespace gui_dev::course
