// demo.cpp —— 组件库演示入口。
//
// 这里只做三件事：
//   1. 把 component_view 里的页面挂进 pages_
//   2. 每帧驱动 Global（输入/画布）→ Page::Update → Page::Render
//   3. 处理页面切换（L / R）与脚本化退出（GUI_DEV_EXIT_AFTER）
//
// 组件与页面本身都在 component_view/ 下，不在这里实现。
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/pages/ComponentGalleryPage.h"
#include "component_view/pages/PropertyPage.h"
#include "core/App.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

// component_view 的页面是即时模式渲染（App::OnFrame 里直接画），不用 Scene 栈；
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

        // ---- 页面登记：加自己的页面就 push 到这里 --------------------------
        pages_.push_back(std::make_unique<gui_dev::cv::ComponentGalleryPage>());
        pages_.push_back(std::make_unique<gui_dev::cv::PropertyPage>());

        for (std::size_t index = 0; index < pages_.size(); ++index) {
            pages_[index]->Bind(ui);
            pages_[index]->SetPageInfo(static_cast<int>(index), static_cast<int>(pages_.size()));
        }

        if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
            exit_after_ = std::atoi(value);
        }
    }

    void OnFrame(gui_dev::UiContext& ui, float dt) override {
        gui_dev::cv::Global::BeginFrame(ui);

        const gui_dev::PadState& pad = ui.Pad();
        if (pad.Pressed(gui_dev::InputAction::PageLeft) && page_index_ > 0) {
            --page_index_;
        }
        if (pad.Pressed(gui_dev::InputAction::PageRight) &&
            page_index_ + 1 < static_cast<int>(pages_.size())) {
            ++page_index_;
        }

        if (pages_.empty()) {
            gui_dev::cv::Global::EndFrame();
            return;
        }
        gui_dev::cv::Page& page = *pages_[static_cast<std::size_t>(page_index_)];
        page.Update(dt);
        page.Render();

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
