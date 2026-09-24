// demo.cpp —— 手柄优先的控件库 Demo 入口。
//
// 只做三件事：
//   1. 把页面挂进 pages_（当前只有一个 ShowcaseShell：左侧 16 个控件 Tab + 右侧展示区）
//   2. 每帧驱动：Global（输入/画布）→ Page::Update → Page::Render
//   3. 处理脚本化退出（GUI_DEV_EXIT_AFTER）与窗口尺寸（GUI_DEV_WINDOW）
//
// 组件与页面都在 component_view/ 下，这里不实现任何控件。
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/pages/ShowcaseShell.h"
#include "core/App.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

// 组件页面是即时模式渲染（App::OnFrame 里直接画），不用 Scene 栈；
// 但主循环把「栈空」当作应用结束，所以压一个空壳场景占住栈顶。
class HostScene : public gui_dev::Scene {
public:
    const char* Name() const override { return "component_view"; }
    void OnRender(gui_dev::UiContext& ui) override { (void)ui; }
};

class DemoApp : public gui_dev::App {
public:
    void Configure(gui_dev::BackendConfig& cfg, gui_dev::PlatformKind kind) const override {
        (void)kind;
        cfg.title = "GUI_DEV · component_view";
        cfg.width = 1280;
        cfg.height = 720;
        cfg.vsync = true;
        cfg.resizable = true;

        if (const char* size = std::getenv("GUI_DEV_WINDOW")) {
            int width = 0;
            int height = 0;
            if (std::sscanf(size, "%dx%d", &width, &height) == 2 && width > 0 && height > 0) {
                cfg.width = width;
                cfg.height = height;
            }
        }
        if (std::getenv("GUI_DEV_NO_VSYNC") != nullptr) {
            cfg.vsync = false;
        }
#if defined(GUI_DEV_PLATFORM_switch)
        cfg.vsync = false; // Switch 由 libnx 垂直同步，SDL 再同步会拖帧
#endif
    }

    void OnStart(gui_dev::UiContext& ui) override {
        gui_dev::cv::Theme::ApplyToImGui();
        Scenes().Reset(std::make_unique<HostScene>());

        // ---- 页面登记：16 个控件页都在 ShowcaseShell 里 --------------------
        auto shell = std::make_unique<gui_dev::cv::ShowcaseShell>();
        shell->Bind(ui);
        pages_.push_back(std::move(shell));

        if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
            exit_after_ = std::atoi(value);
        }
    }

    void OnFrame(gui_dev::UiContext& ui, float dt) override {
        gui_dev::cv::Global::BeginFrame(ui);
        // 所有按键都交给控件自己处理（Focus 是核心状态，页面不做额外分发）。
        if (!pages_.empty()) {
            gui_dev::cv::Page& page = *pages_[static_cast<std::size_t>(page_index_)];
            page.Update(dt);
            page.Render();
        }
        gui_dev::cv::Global::EndFrame();

        if (exit_after_ > 0 && ++frame_ >= exit_after_) {
            ui.GetBackend().RequestQuit();
        }
    }

    void OnShutdown(gui_dev::UiContext& ui) override {
        (void)ui;
        // 页面持有纹理：必须在后端关闭前释放，否则退出时会访问已销毁的渲染器。
        pages_.clear();
    }

private:
    std::vector<std::unique_ptr<gui_dev::cv::Page>> pages_;
    int page_index_ = 0;
    int frame_ = 0;
    int exit_after_ = 0;
};

} // namespace

int main(int, char**) {
    DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
