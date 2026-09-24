// demo.cpp —— 演示入口（当前从最小状态开始：页面左上角只有一个 128x128 的 Box）。
//
// 结构很简单：
//   1. DemoPage 往页面里放组件（现在只有 Box）
//   2. DemoApp 每帧驱动：Global（输入/画布）→ Page::Update → Page::Render
//   3. 保留三个调试开关：GUI_DEV_WINDOW=WxH、GUI_DEV_NO_VSYNC=1、GUI_DEV_EXIT_AFTER=<帧数>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/components/Box.h"
#include "component_view/pages/Page.h"
#include "core/App.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

using gui_dev::cv::Box;
using gui_dev::cv::Page;

// ---------------------------------------------------------------- 页面 ----

class DemoPage : public Page {
public:
    const char* Title() const override { return "component_view"; }

    void OnBuild() override {
        // 页面左上角放一个 128x128 的盒子。
        // moveTo(0, 0) 就是页面的左上角（根节点没有 padding），想挪位置改这两个数即可。
        box_ = Root().Emplace<Box>("box");
        box_->moveTo(0.0f, 0.0f);
        box_->resize(128.0f, 128.0f);
        box_->fillWith(gui_dev::cv::Theme::kBgWidget);
        box_->roundCorners(gui_dev::cv::Theme::kRadius);
    }

private:
    Box* box_ = nullptr;
};

// ------------------------------------------------------------------ App ----

// 页面是即时模式渲染（App::OnFrame 里直接画），不用 Scene 栈；
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

        page_ = std::make_unique<DemoPage>();
        page_->Bind(ui);

        if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
            exit_after_ = std::atoi(value);
        }
    }

    void OnFrame(gui_dev::UiContext& ui, float dt) override {
        gui_dev::cv::Global::BeginFrame(ui);
        if (page_ != nullptr) {
            page_->Update(dt);
            page_->Render();
        }
        gui_dev::cv::Global::EndFrame();

        if (exit_after_ > 0 && ++frame_ >= exit_after_) {
            ui.GetBackend().RequestQuit();
        }
    }

    void OnShutdown(gui_dev::UiContext& ui) override {
        (void)ui;
        // 页面可能持有纹理等后端资源：必须在后端关闭前释放
        page_.reset();
    }

private:
    std::unique_ptr<DemoPage> page_;
    int frame_ = 0;
    int exit_after_ = 0;
};

} // namespace

int main(int, char**) {
    DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
