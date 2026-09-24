// 课程外壳：负责「课时切换」和「退出」，各课时只管画自己的内容。
//
// 为什么单独做一个 App：课程需要跨课时的导航（L/R 换课），
// 而每课时都是独立的 Scene —— 导航逻辑放这里，课时里不重复写。
#pragma once

#include <string>

#include "core/App.h"

namespace gui_dev::course {

class CourseApp final : public App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;
    void OnFrame(UiContext& ui, float dt) override;

private:
    void SwitchTo(int index);

    int index_ = 0;
    int frame_ = 0;
};

} // namespace gui_dev::course
