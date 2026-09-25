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
#include "component_view/components/Badge.h"
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
using gui_dev::cv::Badge;
using gui_dev::cv::BadgeStyle;
using gui_dev::cv::Box;
using gui_dev::cv::EmuPlatform;
using gui_dev::cv::Button;
using gui_dev::cv::CustomButton;
using gui_dev::cv::IconButton;
using gui_dev::cv::IconButtonShape;
using gui_dev::cv::IconTextButton;
using gui_dev::cv::OptionButton;
using gui_dev::cv::Page;
using gui_dev::cv::TextButton;
using gui_dev::cv::ToastType;
using gui_dev::cv::ToggleButton;
using gui_dev::cv::ValueButton;
namespace Theme = gui_dev::cv::Theme;
namespace Global = gui_dev::cv::Global;

// 启动缩放：1.0 = 不额外缩放（验过的那版行为）。
// 之前默认 1.25 会让逻辑画布从 1280x720 变成 1024x576，进而让后端那段「鼠标坐标修正」开始生效
// （缩放 1.0 时它正好是空操作），怀疑因此把 Switch 上的触摸落点带偏了 —— 先回到 1.0。
// 需要放大时用 GUI_DEV_ZOOM=<倍率> 或直接改这里。
constexpr float kDefaultZoom = 1.0f;

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
        const float height = Theme::kControlHeight; // 统一行高（56）
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

        // 1 普通按钮（弹窗的确认 / 取消这类提示文字）：文字居中，不带说明行
        TextButton* plain = Root().Emplace<TextButton>("普通按钮");
        place(plain, "btn_text");

        // 2 图标 + 文字：图标占左侧正方形格（格内水平+垂直居中），文字紧跟其右
        IconTextButton* icon_text =
            Root().Emplace<IconTextButton>(Icons::Glyph(Icons::Material::Play), "图标 + 文字按钮");
        icon_text->setSubtitle("图标是正方形格，格内居中；图标在左、文字紧跟其右");
        place(icon_text, "btn_icon_text");

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

        // 6 LR 选项选择器：右侧 [L] 固定间隔 [R]，L/R 键切换；选项太长就在间隔里滚动
        OptionButton* option =
            Root().Emplace<OptionButton>(Icons::Glyph(Icons::Material::ImagePlaceholder), "画面缩放");
        option->setOptions({"整数缩放", "线性过滤", "CRT 扫描线（像素风，速度慢）"});
        option->setSubtitle("L / R 键切换选项；选项超长会在间隔里滚动");
        connect(option, &OptionButton::selectionChanged, this, [](int index) {
            if (TraceSignal()) {
                std::printf("[signal] option index = %d\n", index);
                std::fflush(stdout);
            }
        });
        place(option, "btn_option");

        // 7 LR 数值选择器：右侧 [L] 固定间隔 [R]，初始化时给范围/步长/精度
        ValueButton* value = Root().Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "音量");
        value->setup(65.0f, 0.0f, 100.0f, 5.0f, 0);
        value->setSubtitle("L / R 调值（长按加速），松开才发 valueChanged");
        connect(value, &ValueButton::valueChanged, this, [](float current) {
            if (TraceSignal()) {
                std::printf("[signal] value = %.2f\n", static_cast<double>(current));
                std::fflush(stdout);
            }
        });
        place(value, "btn_value");

        // 3 纯图标按钮：只有圆角正方形和圆形两种形态，边长 setSide()；
        // 有说明行时说明行落到图标下方居中（图标格自动给说明行让位）
        IconButton* icon_square = Root().Emplace<IconButton>(Icons::Glyph(Icons::Material::Settings));
        icon_square->setSide(Theme::kControlHeight);
        icon_square->setShape(IconButtonShape::RoundedSquare);
        icon_square->setSubtitle("圆角方形");
        icon_square->moveTo(440.0f, kIconRowY);
        buttons_.push_back(icon_square);

        IconButton* icon_circle = Root().Emplace<IconButton>(Icons::Glyph(Icons::Material::Favorite));
        icon_circle->setSide(Theme::kControlHeight);
        icon_circle->setShape(IconButtonShape::Circle);
        icon_circle->moveTo(440.0f + Theme::kControlHeight + 12.0f, kIconRowY);
        icon_circle->setSubtitle("圆形");
        buttons_.push_back(icon_circle);

        // 顺手保留一个可聚焦 Box（容器/控件两种身份），放在按钮右侧。
        // 不显式 fillWith，底色就来自调色板 → 切主题会自动跟着变。
        box_ = Root().Emplace<Box>("box");
        box_->moveTo(440.0f, 20.0f);
        box_->resize(124.0f, 124.0f);
        box_->roundCorners(Global::component_style.corner_radius);
        box_->makeFocusable();
        connect(box_, &Box::clicked, this, [this] {
            box_->fillWith(box_->hasFocus() ? Theme::kAccent : Theme::kBgWidget); // 显式设色后就不再跟随主题
        });

        // 机种徽标墙：14 个机种 + 一个「其它」兜底，放在网格视图那种容器里
        BuildBadgeWall();

        // 右侧控制列：从上往下排控制按钮（主题 / 放大 / 缩小）
        theme_button_ = AddControl(Icons::Material::DarkMode, "btn_theme", ThemeName());
        connect(theme_button_, &IconButton::clicked, this, [this] { ToggleTheme(); });

        // Toast 触发按钮（业务代码就这一行：Toasts().ShowSuccess(...)）
        toast_ok_ = AddControl(Icons::Material::CheckCircle, "btn_toast_ok", "成功");
        connect(toast_ok_, &IconButton::clicked, this, [this] { Toasts().ShowSuccess("保存状态成功"); });

        toast_error_ = AddControl(Icons::Material::ErrorOutline, "btn_toast_error", "失败");
        connect(toast_error_, &IconButton::clicked, this, [this] { Toasts().ShowError("读取状态失败"); });

        toast_info_ = AddControl(Icons::Material::Info, "btn_toast_info", "信息");
        connect(toast_info_, &IconButton::clicked, this, [this] { Toasts().ShowInfo("正在加载游戏..."); });

        toast_long_ = AddControl(Icons::Material::Description, "btn_toast_long", "长文本");
        connect(toast_long_, &IconButton::clicked, this, [this] {
            // 长文本会自动换行，高度变化会让后面的 Toast 重新排位
            Toasts().ShowInfo("正在加载游戏资源，正在校验存档完整性，请稍候，不要关闭主机电源。");
        });

        theme_button_->setIcon(Icons::Glyph(ThemeIcon()));
        LayoutControls();
    }

    // 右侧控制列贴着画布右边缘、从上往下排（窗口尺寸可变，所以每帧重排一次）
    void OnUpdate(float dt) override {
        (void)dt;
        LayoutControls();
        LayoutBadges(); // 画布尺寸变了（缩放/窗口）徽标跟着重排
    }

    // 一键切浅色 / 深色：换调色板 + 约定样式，再让整棵组件树重新取色
    void ToggleTheme() {
        Theme::ToggleMode();
        Theme::ApplyToImGui();
        RefreshTheme(); // 内部：Global::ApplyTheme() + 根节点装饰复位 + 整棵树重新取色
        theme_button_->setIcon(Icons::Glyph(ThemeIcon()));
        theme_button_->setSubtitle(ThemeName());
    }

    // 徽标墙：两列、每个徽标同样大小、文字一律白色（不套容器 Box，直接画在页面上）
    void BuildBadgeWall() {
        constexpr int kBadgeCount = 15; // 14 个机种 + 其它
        for (int i = 0; i < kBadgeCount; ++i) {
            const EmuPlatform platform = i < 14 ? static_cast<EmuPlatform>(i + 1) : EmuPlatform::Unknown;
            Badge* badge = Root().Emplace<Badge>(platform);
            badge->SetName("badge");
            if (platform == EmuPlatform::Unknown) {
                badge->setText("其它"); // 表里兜底那条文字是空的，这里手动给一个看颜色
            }
            // 统一尺寸 + 白字（Badge 默认文字色跟主题，这里显式固定成白色）
            badge->setHeight(kBadgeHeight);
            badge->setFixedWidth(true);
            badge->setMinWidth(kBadgeWidth);
            badge->setColors(PlatformBadgeInfoOf(platform).background, Theme::kWhite);
            badges_.push_back(badge);
        }
        LayoutBadges();
    }

    // 徽标两列排布：整体贴右侧控制列左边，画布尺寸变了（缩放/窗口）每帧重排
    void LayoutBadges() {
        const float right = Global::canvas_size.x - kControlGap - kControlSize - 20.0f;
        const float left = right - (kBadgeWidth * 2.0f + kBadgeGapX);
        for (std::size_t i = 0; i < badges_.size(); ++i) {
            const float x = left + static_cast<float>(i % 2) * (kBadgeWidth + kBadgeGapX);
            const float y = kBadgeTop + static_cast<float>(i / 2) * (kBadgeHeight + kBadgeGapY);
            badges_[i]->moveTo(x, y);
        }
    }

    // 控制列里的按钮：图标 + 按钮外侧说明行，边长统一
    IconButton* AddControl(gui_dev::Icons::Material icon, const char* name, const char* caption) {
        IconButton* button = Root().Emplace<IconButton>(Icons::Glyph(icon));
        button->SetName(name);
        button->setSide(kControlSize);
        button->setShape(IconButtonShape::RoundedSquare);
        button->setSubtitle(caption);
        controls_[control_count_++] = button;
        buttons_.push_back(button); // 蹭一下「+ 键开关说明行」的逻辑
        return button;
    }

    void LayoutControls() {
        float y = kControlTop;
        for (int i = 0; i < control_count_; ++i) {
            controls_[i]->moveTo(ControlX(), y);
            y += kControlSize + kControlGap;
        }
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
    // 控制列参数：贴右边缘 20px，按钮边长 56px
    static constexpr float kControlSize = Theme::kControlHeight; // 统一控件尺寸（56）
    static constexpr float kIconRowY = 164.0f; // 两个图标按钮的 y（在 Box 下方）
    static constexpr float kControlRight = 20.0f;
    static constexpr float kControlTop = 20.0f;

    static float ControlX() { return Global::canvas_size.x - kControlRight - kControlSize; }
    static gui_dev::Icons::Material ThemeIcon() {
        // 图标显示「当前主题」：浅色 = 太阳，深色 = 月亮
        return Theme::IsLight() ? Icons::Material::LightMode : Icons::Material::DarkMode;
    }
    static const char* ThemeName() { return Theme::IsLight() ? "浅色" : "深色"; }

    // 徽标墙几何
    // 徽标墙：两列，统一尺寸
    static constexpr float kBadgeWidth = 92.0f;
    static constexpr float kBadgeHeight = Theme::kBadgeHeight; // 统一标签高（26）
    static constexpr float kBadgeGapX = 14.0f;
    static constexpr float kBadgeGapY = 8.0f;
    static constexpr float kBadgeTop = 300.0f;

    static constexpr int kControlMax = 8;
    static constexpr float kControlGap = 12.0f;

    std::vector<Button*> buttons_;
    std::vector<Badge*> badges_;
    Box* box_ = nullptr;
    IconButton* theme_button_ = nullptr;
    IconButton* toast_ok_ = nullptr;
    IconButton* toast_error_ = nullptr;
    IconButton* toast_info_ = nullptr;
    IconButton* toast_long_ = nullptr;
    IconButton* controls_[kControlMax] = {};
    int control_count_ = 0;
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
        // 720p 手持基准下整体放大 1.25 倍（右侧控制列的放大/缩小还能再调）
        ui.SetUiZoom(kDefaultZoom);
        // 默认浅色主题（桌面端白底看着舒服），右侧控制列的按钮还能一键切成深色
        gui_dev::cv::Theme::SetMode(gui_dev::cv::Theme::ThemeMode::Light);
        gui_dev::cv::Theme::ApplyToImGui();
        gui_dev::cv::Global::ApplyTheme();
        Scenes().Reset(std::make_unique<HostScene>());

        page_ = std::make_unique<DemoPage>();
        page_->Bind(ui);

        if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
            exit_after_ = std::atoi(value);
        }
        // 调试：GUI_DEV_ZOOM=1.25 直接以指定缩放启动（右侧控制列的放大/缩小按钮也能改）
        if (const char* zoom = std::getenv("GUI_DEV_ZOOM")) {
            const float value = static_cast<float>(std::atof(zoom));
            if (value > 0.0f) {
                ui.SetUiZoom(value);
            }
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
