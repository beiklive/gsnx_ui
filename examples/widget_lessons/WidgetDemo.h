// 自定义控件教学 Demo：7 个循序渐进的最小例子。
//
// 每个 Lesson 的注释都写了「用了哪些 API / 为什么 / 坑在哪」。
// 建议对照源码从上往下读，同时跑起来点一点看反应。
//
// 核心心智模型（记住这一条就够开始了）：
//   ImGui 是「立即模式 + 布局游标」。它不认识你的控件，所以自定义控件分两步：
//     1) 占位：InvisibleButton / Dummy / ItemAdd —— 交给 ImGui 管布局、命中测试、ID
//     2) 自绘：GetWindowDrawList() 在占位矩形里画什么都可以
//   绘制顺序 = 代码顺序；状态要自己存（成员 / ImGuiStorage），动画吃 io.DeltaTime。
#pragma once

#include "core/App.h"
#include "ui/Scene.h"

namespace gui_dev::demo {

class WidgetDemoScene final : public Scene {
public:
    const char* Name() const override { return "widgets"; }
    void OnRender(UiContext& ui) override;

private:
    // Lesson 3 用的持久化状态（演示「状态存在控件外面」）
    float toggle_state_[3] = {0.0f, 0.0f, 0.0f};
    bool toggle_value_[3] = {false, true, false};

    // Lesson 4/5 的值
    float volume_ = 0.62f;
    float knob_ = 0.35f;

    // Lesson 6 用的每项独立状态
    float pad_level_[4] = {0.2f, 0.5f, 0.8f, 0.35f};

    bool panel_open_ = true;
};

class WidgetDemoApp final : public App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;
    void OnFrame(UiContext& ui, float dt) override;

private:
    std::string version_ = GUI_DEV_VERSION;
    int frame_ = 0;
};

} // namespace gui_dev::demo
