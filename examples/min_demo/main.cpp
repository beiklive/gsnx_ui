// 最小接入示例 —— 证明 component_view 可以整体搬进别的项目，不用重做。
//
// 这份文件就是「另一个项目要写的东西」的全部：一个页面 + 几个组件 + 主题/缩放初始化 + 主循环。
// 库本身不依赖 demo.cpp，也不依赖任何业务代码；需要一起带上的只有：
//   component_view/  +  framework/（输入抽象 platform/Input.h、图标表 ui/Icons.*、
//   宿主桥接 ui/UiContext.*、主循环 core/App.*）+ 字体资源（MaterialIcons-Regular.ttf 等）。
#include <memory>

#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/CapsuleTabs.h"
#include "component_view/components/Header.h"
#include "component_view/components/TabColumn.h"
#include "component_view/pages/Page.h"
#include "core/App.h"
#include "ui/Icons.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

using gui_dev::InputAction;
namespace Icons = gui_dev::Icons;
namespace Global = gui_dev::cv::Global;
namespace Theme = gui_dev::cv::Theme;
using gui_dev::cv::Box;
using gui_dev::cv::CapsuleTabs;
using gui_dev::cv::Header;
using gui_dev::cv::IconButton;
using gui_dev::cv::IconButtonShape;
using gui_dev::cv::Page;
using gui_dev::cv::TabColumn;
using gui_dev::cv::TextButton;
using gui_dev::cv::ToggleButton;

// 一个页面 = 左边 tab 列 + 右边内容区；内容用 Header 分段，切换只切 visible。
class SettingsPage : public Page {
public:
    const char* Title() const override { return "min_demo"; }

    void OnBuild() override {
        tab_ = Root().Emplace<TabColumn>();
        tab_->setItems({{Icons::Glyph(Icons::Material::Settings), "常规"},
                        {Icons::Glyph(Icons::Material::Info), "关于"}});

        header_a_ = Root().Emplace<Header>("常规设置");
        header_a_->setInfo("Header = 竖条 + 标题 + 分隔线");

        toggle_ = Root().Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络");
        toggle_->setSubtitle("A / 点击切换");
        toasts_ = Root().Emplace<TextButton>("弹一个 Toast");
        toasts_->setFontSize(Theme::kFontHeader);

        header_b_ = Root().Emplace<Header>("关于");
        capsule_ = Root().Emplace<CapsuleTabs>();
        capsule_->setLabels({"帧率", "分辨率", "缩放"}, 0);

        // 焦点变化即切页（TabColumn 的默认行为），这里只切可见性
        connect(tab_, &TabColumn::selectionChanged, this, [this](int index) {
            const bool first = index == 0;
            header_a_->visible = toggle_->visible = toasts_->visible = first;
            header_b_->visible = capsule_->visible = !first;
        });
        connect(toasts_, &gui_dev::cv::Widget::clicked, this, [this] { Toasts().ShowSuccess("按钮被按下"); });
        tab_->setFocusTarget(toggle_);

        // 初始只显示第一页（selectionChanged 只在「变化」时发，初始状态要自己设）
        header_b_->visible = capsule_->visible = false;
    }

    void OnUpdate(float dt) override {
        (void)dt;
        // 布局：左列 tab（贴左边），右边内容区从 250 起
        const float height = Global::canvas_size.y - 40.0f;
        tab_->position = ImVec2(20.0f, 20.0f);
        tab_->size = ImVec2(210.0f, height);
        const float cx = 250.0f;
        const float cw = Global::canvas_size.x - 270.0f;

        header_a_->position = ImVec2(cx, 20.0f);
        header_a_->size.x = cw;
        toggle_->moveTo(cx, 90.0f);
        toggle_->resize(cw, Theme::kControlHeight);
        toasts_->moveTo(cx, 160.0f);
        toasts_->resize(cw, Theme::kControlHeight);

        header_b_->position = ImVec2(cx, 20.0f);
        header_b_->size.x = cw;
        capsule_->position = ImVec2(cx, 120.0f);
        capsule_->size.x = cw;
    }

private:
    TabColumn* tab_ = nullptr;
    Header* header_a_ = nullptr;
    Header* header_b_ = nullptr;
    ToggleButton* toggle_ = nullptr;
    TextButton* toasts_ = nullptr;
    CapsuleTabs* capsule_ = nullptr;
};

class MinScene : public gui_dev::Scene {
public:
    const char* Name() const override { return "min_demo"; }
    void OnRender(gui_dev::UiContext& ui) override { (void)ui; }
};

class MinApp : public gui_dev::App {
public:
    void Configure(gui_dev::BackendConfig& cfg, gui_dev::PlatformKind kind) const override {
        (void)kind;
        cfg.title = "component_view · min demo";
        cfg.width = 1280;
        cfg.height = 720;
        cfg.vsync = true;
    }

    void OnStart(gui_dev::UiContext& ui) override {
        ui.SetUiZoom(1.2f);            // UI 缩放（整套组件一起放大）
        Theme::SetMode(Theme::ThemeMode::Dark);
        Theme::ApplyToImGui();
        Global::ApplyTheme();
        Scenes().Reset(std::make_unique<MinScene>());
        page_ = std::make_unique<SettingsPage>();
        page_->Bind(ui);
    }

    void OnFrame(gui_dev::UiContext& ui, float dt) override {
        Global::BeginFrame(ui);
        page_->Update(dt);
        page_->Render();
        Global::EndFrame();
    }

    void OnShutdown(gui_dev::UiContext& ui) override {
        (void)ui;
        page_.reset(); // 页面可能持有后端资源，必须在后端关闭前释放
    }

private:
    std::unique_ptr<SettingsPage> page_;
};

} // namespace

#if defined(GUI_DEV_PLATFORM_android) || defined(GUI_DEV_PLATFORM_ios)
// 移动端 SDL 用 SDL_main 当入口：Android 是 SDL_android_main.c（JNI），
// iOS 是 SDL_uikitappdelegate（UIApplicationMain）。这个头必须出现在 main 定义之前。
#include <SDL_main.h>
#endif

int main(int, char**) {
    MinApp app;
    return gui_dev::AppRunner(app).Run();
}
