// 应用外壳：组装后端 + ImGui + 场景栈，提供统一主循环。
#pragma once

#include <memory>
#include <string>

#include "platform/Backend.h"
#include "ui/Scene.h"

namespace gui_dev {

class UiContext;

// 使用方式（见 src/main.cpp）：
//   class MyApp : public App { ... };
//   int main() { return AppRunner(MyApp{}).Run(); }
class App {
public:
    virtual ~App() = default;

    // 平台相关的结构差异（标题、分辨率、后端参数）集中在这里，
    // 上层业务代码不接触 BackendConfig。
    virtual void Configure(BackendConfig& cfg, PlatformKind kind) const { (void)cfg; (void)kind; }

    // 首帧前调用一次：Push 初始场景、加载资源。
    virtual void OnStart(UiContext& ui) = 0;

    // 每帧逻辑与绘制前的钩子（可选）。
    virtual void OnFrame(UiContext& ui, float dt) { (void)ui; (void)dt; }

    // 退出前释放资源。
    virtual void OnShutdown(UiContext& ui) { (void)ui; }

    SceneStack& Scenes() { return scenes_; }

private:
    SceneStack scenes_;
};

class AppRunner {
public:
    explicit AppRunner(App& app);
    ~AppRunner();

    AppRunner(const AppRunner&) = delete;
    AppRunner& operator=(const AppRunner&) = delete;

    // 返回进程退出码：0 正常，非 0 为后端初始化失败。
    int Run();

private:
    App& app_;
    std::unique_ptr<Backend> backend_;
    std::unique_ptr<UiContext> ui_;
    bool initialized_ = false;
};

} // namespace gui_dev
