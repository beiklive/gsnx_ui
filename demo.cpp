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
#include "component_view/components/Button.h"
#include "component_view/pages/Page.h"
#include "ui/Icons.h"
#include "core/App.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

using gui_dev::InputAction;
namespace Icons = gui_dev::Icons;
using gui_dev::cv::Box;
using gui_dev::cv::Button;
using gui_dev::cv::CustomButton;
using gui_dev::cv::IconButton;
using gui_dev::cv::IconTextButton;
using gui_dev::cv::OptionButton;
using gui_dev::cv::Page;
using gui_dev::cv::TextButton;
using gui_dev::cv::ToggleButton;
using gui_dev::cv::ValueButton;
namespace Theme = gui_dev::cv::Theme;
namespace Global = gui_dev::cv::Global;

// 验收用开关：GUI_DEV_TRACE_SIGNAL=1 时把按钮状态变化打到终端，方便脚本化测试
bool TraceSignal() {
    const char* value = std::getenv("GUI_DEV_TRACE_SIGNAL");
    return value != nullptr && value[0] != '0';
}

// ---------------------------------------------------------------- 页面 ----

class DemoPage : public Page {
public:
    const char* Title() const override { return "component_view"; }

    void OnBuild() override {
        // 所有按钮都用全局约定样式（Global::component_style）：1px 灰白边框 / 5px 圆角 /
        // 右下角阴影 / 流光聚焦框（与按钮留 2px 边距）。单个按钮可以用 setBorder() 等覆盖。
        const float x = 20.0f;
        const float width = 396.0f;
        const float height = 52.0f;
        const float gap = 10.0f;
        float cursor = 20.0f;
        const auto place = [&](Button* button, const char* name) {
            button->SetName(name);
            button->moveTo(x, cursor);
            button->resize(width, height);
            button->setSubtitle(button->subtitle, true); // 说明行默认打开（+ 键可以整体关掉）
            cursor += height + gap;
            buttons_.push_back(button);
            return button;
        };

        // 1 普通按钮：文字居中
        TextButton* plain = Root().Emplace<TextButton>("普通按钮");
        plain->setSubtitle("文字居中显示；这行小字可以用 showSubtitle(false) 关掉");
        place(plain, "btn_text");

        // 2 图标 + 文字：图标到边框上下左右距离相同，图标和文字都靠左
        IconTextButton* icon_text =
            Root().Emplace<IconTextButton>(Icons::Glyph(Icons::Material::Play), "图标 + 文字按钮");
        icon_text->setSubtitle("图标是正方形，四周留白相同；图标在左、文字紧随其后");
        place(icon_text, "btn_icon_text");

        // 3 纯图标按钮
        IconButton* icon_only = Root().Emplace<IconButton>(Icons::Glyph(Icons::Material::Settings));
        icon_only->setSubtitle("只显示图标（图标在框内居中）");
        place(icon_only, "btn_icon");

        // 4 开关按钮：右侧显示 开/关（开=蓝、关=灰），A/点击切换
        ToggleButton* toggle = Root().Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络");
        toggle->setSubtitle("点击 / A 切换开关状态");
        place(toggle, "btn_toggle");
        connect(toggle, &ToggleButton::toggled, this, [](bool on) {
            // 需要的话在这里接收：on = true 表示打开
            if (TraceSignal()) {
                std::printf("[signal] toggle = %s\n", on ? "开" : "关");
                std::fflush(stdout);
            }
        });

        // 5 自定义右侧文字按钮
        CustomButton* custom = Root().Emplace<CustomButton>(Icons::Glyph(Icons::Material::Storage), "存储路径");
        custom->setRightText("sdmc:/switch/", Theme::kTeal);
        custom->setSubtitle("右侧文字的内容与颜色都可以改（setRightText）");
        place(custom, "btn_custom");

        // 6 LR 选项选择器：右侧 [L] 选项 [R]，L/R 键切换
        OptionButton* option =
            Root().Emplace<OptionButton>(Icons::Glyph(Icons::Material::ImagePlaceholder), "画面缩放");
        option->setOptions({"整数缩放", "线性过滤", "CRT 扫描线"});
        option->setSubtitle("L / R 键切换选项");
        connect(option, &OptionButton::selectionChanged, this, [](int index) {
            if (TraceSignal()) {
                std::printf("[signal] option index = %d\n", index);
                std::fflush(stdout);
            }
        });
        place(option, "btn_option");

        // 7 LR 数值选择器：右侧 [L] 数值 [R]，初始化时给范围/步长/精度
        ValueButton* value = Root().Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "音量");
        value->setup(65.0f, 0.0f, 100.0f, 5.0f, 0);
        value->setSubtitle("L / R 调值，步长 5、范围 0~100");
        connect(value, &ValueButton::valueChanged, this, [](float current) {
            if (TraceSignal()) {
                std::printf("[signal] value = %.2f\n", static_cast<double>(current));
                std::fflush(stdout);
            }
        });
        place(value, "btn_value");

        // 顺手保留一个可聚焦 Box（容器/控件两种身份），放在按钮右侧
        box_ = Root().Emplace<Box>("box");
        box_->moveTo(440.0f, 20.0f);
        box_->resize(128.0f, 128.0f);
        box_->fillWith(Theme::kBgWidget);
        box_->roundCorners(Global::component_style.corner_radius);
        box_->makeFocusable();
        connect(box_, &Box::clicked, this, [this] {
            box_->fillWith(box_->hasFocus() ? Theme::kAccent : Theme::kBgWidget);
        });
    }

    // + 键：一键开关所有按钮的说明行（验证「是否显示说明」接口，开关都保持文字块垂直居中）
    void OnInput() override {
        if (Global::pad.Pressed(InputAction::Menu) && Global::Available(InputAction::Menu)) {
            subtitle_on_ = !subtitle_on_;
            for (Button* button : buttons_) {
                button->showSubtitle(subtitle_on_);
            }
            Global::MarkConsumed(InputAction::Menu);
        }
    }

private:
    std::vector<Button*> buttons_;
    Box* box_ = nullptr;
    bool subtitle_on_ = true;
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
