#include "course/CourseApp.h"

#include <cstdio>
#include <cstdlib>

#include "course/CourseLessons.h"
#include "course/CourseLog.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

void CourseApp::Configure(BackendConfig& cfg, PlatformKind kind) const {
    (void)kind;
    cfg.title = "GBAStation · ImGui 自制控件课程";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.vsync = true;
    cfg.resizable = true;
#if defined(GUI_DEV_PLATFORM_switch)
    cfg.vsync = false; // Switch 由 libnx 垂直同步
#endif
    // 和另外几个 demo 一致的调试开关：GUI_DEV_WINDOW=960x720 / GUI_DEV_NO_VSYNC=1
    if (const char* window = std::getenv("GUI_DEV_WINDOW")) {
        int w = 0;
        int h = 0;
        if (std::sscanf(window, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            cfg.width = w;
            cfg.height = h;
        }
    }
    if (std::getenv("GUI_DEV_NO_VSYNC")) {
        cfg.vsync = false;
    }
}

void CourseApp::SwitchTo(int index) {
    const std::vector<CourseEntry>& entries = CourseEntries();
    const int count = static_cast<int>(entries.size());
    if (count == 0) {
        return;
    }
    index_ = ((index % count) + count) % count; // 环形，允许负数
    // 每课时都是独立的根场景：Reset 会析构旧场景（会触发旧场景的 OnLeave）
    LogEvent("Shell Reset     即将析构旧场景");
    Scenes().Reset(entries[static_cast<std::size_t>(index_)].create());
    LogEvent("Shell SwitchTo  切换到「%s」", entries[static_cast<std::size_t>(index_)].title);
}

void CourseApp::OnStart(UiContext& ui) {
    (void)ui;
    SwitchTo(0);
}

void CourseApp::OnFrame(UiContext& ui, float dt) {
    (void)dt;
    const PadState& pad = ui.Pad();
    if (pad.Pressed(InputAction::PageRight)) {
        SwitchTo(index_ + 1);
    }
    if (pad.Pressed(InputAction::PageLeft)) {
        SwitchTo(index_ - 1);
    }
    if (pad.Pressed(InputAction::Cancel)) {
        ui.GetBackend().RequestQuit();
    }

    // 冒烟测试钩子：GUI_DEV_EXIT_AFTER=<帧数>
    static const int exit_after = [] {
        const char* value = std::getenv("GUI_DEV_EXIT_AFTER");
        return value != nullptr ? std::atoi(value) : 0;
    }();
    if (exit_after > 0 && ++frame_ >= exit_after) {
        ui.GetBackend().RequestQuit();
    }
}

} // namespace gui_dev::course
