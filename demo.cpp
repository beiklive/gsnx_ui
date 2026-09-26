// demo.cpp —— 演示入口（当前从最小状态开始：页面左上角只有一个 128x128 的 Box）。
//
// 结构很简单：
//   1. DemoPage 往页面里放组件（现在只有 Box）
//   2. DemoApp 每帧驱动：Global（输入/画布）→ Page::Update → Page::Render
//   3. 保留三个调试开关：GUI_DEV_WINDOW=WxH、GUI_DEV_NO_VSYNC=1、GUI_DEV_EXIT_AFTER=<帧数>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <map>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/components/Badge.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/Anim.h"
#include "component_view/components/CapsuleTabs.h"
#include "component_view/components/Content.h"
#include "component_view/components/Header.h"
#include "component_view/components/MarkdownView.h"
#include "component_view/components/TabColumn.h"
#include "component_view/pages/Page.h"
#include "component_view/popup/Popup.h"
#include "ui/Icons.h"
#include "ui/Texture.h"
#include "core/App.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace {

using gui_dev::InputAction;
namespace Icons = gui_dev::Icons;
using gui_dev::cv::Badge;
using gui_dev::cv::BadgeStyle;
using gui_dev::cv::Box;
using gui_dev::cv::Widget;
using gui_dev::cv::EmuPlatform;
using gui_dev::cv::Button;
using gui_dev::cv::CapsuleTabs;
using gui_dev::cv::EdgeInsets;
using gui_dev::cv::CustomButton;
using gui_dev::cv::Image;
using gui_dev::cv::Label;
using gui_dev::cv::MarkdownView;
using gui_dev::cv::Popup;
using gui_dev::cv::PopupButtonLayout;
using gui_dev::cv::PopupKind;
using gui_dev::cv::ProgressBar;
using gui_dev::cv::Separator;
using gui_dev::cv::Align;
using gui_dev::cv::LayoutMode;
namespace cv = gui_dev::cv;
using gui_dev::cv::IconButton;
using gui_dev::cv::Header;
using gui_dev::cv::Overflow;
using gui_dev::cv::IconButtonShape;
using gui_dev::cv::IconTextButton;
using gui_dev::cv::OptionButton;
using gui_dev::cv::Page;
using gui_dev::cv::TabColumn;
using gui_dev::cv::TextButton;
using gui_dev::cv::ToastType;
using gui_dev::cv::ToggleButton;
using gui_dev::cv::ValueButton;
namespace Theme = gui_dev::cv::Theme;
namespace Global = gui_dev::cv::Global;
namespace Anim = gui_dev::cv::Anim;

// 启动缩放：1.2 = 整套 UI（含字体与控件几何）放大 1.2 倍。
// 后端把 drawable 除以 (auto_scale * zoom) 得到逻辑画布，所以放大后逻辑画布变成 1067x600，
// 布局全部按画布尺寸自适应（面板/内容区尺寸都从 Global::canvas_size 推）。
// 想回到 1:1 用 GUI_DEV_ZOOM=1，或直接改这里。
constexpr float kDefaultZoom = 1.2f;

// 验收用开关：GUI_DEV_TRACE_SIGNAL=1 时把按钮状态变化打到终端，方便脚本化测试
bool TraceSignal() {
    const char* value = std::getenv("GUI_DEV_TRACE_SIGNAL");
    return value != nullptr && value[0] != '0';
}

// ---------------------------------------------------------------- 页面 ----

// 页面 = 左侧 TabColumn（纵向标签列，包在一个 Box 面板里）
//      + 右侧内容区（各 tab 的子页面，包在另一个 Box 面板里），
// 内容区里用 Header 把控件按区块水平分隔开。
//
// 所有控件在 OnBuild 时一次性建好，按归属的 tab 记进 pages_；切 tab（焦点落上去就切）时：
//   旧页 0.12s 左移淡出 → 播完才隐藏；新页 0.22s 逐项错开、从右边快速弹进来（右滑 + 轻微过冲 + 淡入），
//   两页同时进行，所以不会出现「瞬间刷新」。动画语义对齐 examples/pause_menu。
class DemoPage : public Page {
public:
    ~DemoPage() override {
        // 后台线程必须在页面销毁前收掉（UI 单线程，不允许线程去碰 UI 对象）
        if (scan_thread_.joinable()) {
            scan_thread_.join();
        }
    }

    const char* Title() const override { return "component_view"; }

    void OnBuild() override {
        BuildPanels();
        BuildTabColumn();

        BuildBasicsPage(); // tab 0 基础控件（Label/Separator/ProgressBar/Image/Checkbox/RadioGroup/开关/选择器/标签/滚动列表）
        BuildPopupPage();  // tab 1 按钮变体 + 7 个弹窗示例
        BuildBadgePage();  // tab 2 徽标
        BuildToastPage();  // tab 3 提示
        BuildNavPage();    // tab 4 导航

        // 主题开关是「应用级」的：钉在内容面板右下角，任何 tab 下都能切，不放进 tab 页面
        theme_button_ = tab_panel_->Emplace<IconButton>(Icons::Glyph(ThemeIcon()));
        theme_button_->SetName("btn_theme");
        theme_button_->setSide(kControlSize);
        theme_button_->setShape(IconButtonShape::RoundedSquare);
        theme_button_->setSubtitle(ThemeName());
        connect(theme_button_, &IconButton::clicked, this, [this] { ToggleTheme(); });
        buttons_.push_back(theme_button_);

        // 主题旁边的焦点框样式开关：流光框 / pause_menu 角标框。
        focus_style_button_ = tab_panel_->Emplace<IconButton>(Icons::Glyph(FocusStyleIcon()));
        focus_style_button_->SetName("btn_focus_style");
        focus_style_button_->setSide(kControlSize);
        focus_style_button_->setShape(IconButtonShape::RoundedSquare);
        focus_style_button_->setSubtitle(FocusStyleName());
        connect(focus_style_button_, &IconButton::clicked, this, [this] { ToggleFocusStyle(); });
        buttons_.push_back(focus_style_button_);

        ShowTab(0);
    }

    void OnUpdate(float dt) override {
        UpdatePageAnim(dt);
        LayoutPanels();  // 两个面板的位置/尺寸（画布尺寸可变）
        LayoutContent(); // 面板内的控件重排

        // 基础控件页：进度条一直在走（确定 + 不确定两种形态）
        basics_progress_ += dt * 0.22f;
        if (basics_progress_ > 1.15f) {
            basics_progress_ -= 1.15f;
        }
        progress_->setValue(cv::Clampf(basics_progress_ / 1.0f, 0.0f, 1.0f));

        // 弹窗页：后台扫描任务的进度回传（UI 线程只做读取 + 刷 UI）
        UpdateProgressDemo();

        
        
    }

    // ---------------------------------------------------- 两个 Box 面板 ----
    // tab 栏和子页面是两个独立部分，各自用一个 Box 包住（底色用 kBgPanel，
    // 和按钮的 kBgWidget 区分开，边框/圆角/阴影沿用 Global::component_style）
    void BuildPanels() {
        tab_panel_ = Root().Emplace<Box>("tab_panel");
        tab_panel_->roundCorners(Global::component_style.corner_radius);
        tab_panel_->padding = EdgeInsets::All(kTabPanelPadding);

        content_panel_ = Root().Emplace<Box>("content_panel");
        content_panel_->roundCorners(Global::component_style.corner_radius);
        content_panel_->padding = EdgeInsets::All(kContentPanelPadding);
        // 内容装不下时可以滚（UI 放大到 1.5 倍后逻辑画布只有 853x480，按钮页会超出来）；
        // 装得下时滚动条自动隐藏，所以 1.0 倍下看不出区别。
        content_panel_->overflow = Overflow::Scroll;
        content_panel_->scroll_bar_auto_hide = true;
        content_panel_->scroll_overscroll = true;

        ApplyPanelColors();
    }

    void ApplyPanelColors() {
        // 显式设色不跟随主题，所以切主题时由 ToggleTheme() 再调一次
        tab_panel_->fillWith(Theme::kBgPanel);
        content_panel_->fillWith(Theme::kBgPanel);
    }

    // ------------------------------------------------------- tab 列 ----
    void BuildTabColumn() {
        tab_column_ = tab_panel_->Emplace<TabColumn>();
        tab_column_->SetName("tab_column");
        tab_column_->setItems({
            {Icons::Glyph(Icons::Material::Settings), "基础控件"},
            {Icons::Glyph(Icons::Material::Description), "按钮 / 弹窗"},
            {Icons::Glyph(Icons::Material::ImagePlaceholder), "徽标"},
            {Icons::Glyph(Icons::Material::Info), "提示"},
            {Icons::Glyph(Icons::Material::Games), "导航"},
        });
        // 选中项变了（焦点落上去就变）：切页面
        connect(tab_column_, &TabColumn::selectionChanged, this, [this](int index) {
            ShowTab(index);
            if (TraceSignal()) {
                std::printf("[signal] tab = %d (%s)\n", index,
                            tab_column_->items()[static_cast<std::size_t>(index)].text.c_str());
                std::fflush(stdout);
            }
        });
        // 对已选中项再按 A / 再点一次：重播一次入场
        connect(tab_column_, &TabColumn::activated, this, [this](int index) {
            page_anim_[index].enter_time = 0.0f;
            Toasts().ShowInfo("已在 " + tab_column_->items()[static_cast<std::size_t>(index)].text + " 页");
        });
    }

    // ------------------------------------------------- 子页面切换动画 ----
    // 归到某个 tab：设内容区焦点分区（TabColumn 是分区 1）、登记进 pages_（先全部隐藏，由 ShowTab 打开）
    template <typename T>
    T* AddTo(int tab, T* widget) {
        widget->SetFocusZone(kContentZone);
        widget->visible = false;
        pages_[tab].push_back(widget);
        return widget;
    }

    void ShowTab(int index) {
        if (index < 0 || index >= kTabCount) {
            return;
        }
        // 上一次退场还没播完就直接结束它，避免两页同时在退
        if (exiting_tab_ >= 0) {
            page_anim_[exiting_tab_].exit = 1.0f;
            exiting_tab_ = -1;
        }
        if (index != active_tab_ && !pages_[active_tab_].empty()) {
            page_anim_[active_tab_].exit = 0.0f;
            exiting_tab_ = active_tab_;
        }
        active_tab_ = index;
        // 切页语义：Page 作用域的弹窗跟随页面关闭，Global 作用域的保留（需求 §32）
        Popups().ClosePageScoped();
        page_anim_[index].enter_time = 0.0f; // 新页从 0 开始入场
        // 滚动状态跟着页面走：切走就当重置，否则新页面会停在上一页滚到的位置
        content_panel_->scroll = ImVec2(0.0f, 0.0f);
        content_panel_->scroll_target = ImVec2(0.0f, 0.0f);
        ApplyTabVisibility();

        // → / R 进内容区时落在当前页第一个可聚焦控件上
        Widget* first = nullptr;
        for (Widget* widget : pages_[index]) {
            if (widget->focusable) {
                first = widget;
                break;
            }
        }
        tab_column_->setFocusTarget(first);
    }

    // 当前页 + 正在退场的那一页可见，其余隐藏
    void ApplyTabVisibility() {
        for (int tab = 0; tab < kTabCount; ++tab) {
            const bool shown = (tab == active_tab_) || (tab == exiting_tab_);
            // 退场中的旧页仍然可见（在播动画），但不参与焦点导航：否则方向键会跑到旧页上
            const bool interactive = (tab == active_tab_);
            for (Widget* widget : pages_[tab]) {
                widget->visible = shown;
                widget->focus_inert = !interactive;
            }
        }
    }

    void UpdatePageAnim(float dt) {
        // 整页一个进度：子页面整体进场，内部元素不再各自错开
        page_anim_[active_tab_].enter_time = gui_dev::cv::Minf(
            page_anim_[active_tab_].enter_time + gui_dev::cv::Maxf(dt, 0.0f), kPageEnterDuration);
        if (exiting_tab_ >= 0) {
            page_anim_[exiting_tab_].exit =
                Anim::AdvanceOnce(page_anim_[exiting_tab_].exit, kPageExitDuration, dt);
            if (page_anim_[exiting_tab_].exit >= 1.0f) {
                exiting_tab_ = -1;
                ApplyTabVisibility();
            }
        }
        for (int tab = 0; tab < kTabCount; ++tab) {
            if (tab == active_tab_) {
                // 入场：整页一起从右边快速弹进来（位移套 EaseOutBack → 略微过冲再落位，透明度 EaseOutCubic）
                const float raw = gui_dev::cv::Clampf(page_anim_[tab].enter_time / kPageEnterDuration, 0.0f, 1.0f);
                const ImVec2 offset(kPageEnterOffset * (1.0f - Anim::EaseOutBack(raw)), 0.0f);
                const float alpha = Anim::EaseOutCubic(raw);
                for (Widget* widget : pages_[tab]) {
                    widget->visual_translate = offset;
                    widget->opacity = alpha;
                }
            } else if (tab == exiting_tab_) {
                const float eased = Anim::EaseOutCubic(page_anim_[tab].exit);
                for (Widget* widget : pages_[tab]) {
                    widget->visual_translate = ImVec2(-kPageExitOffset * eased, 0.0f);
                    widget->opacity = 1.0f - eased;
                }
            }
        }
    }

    // ---------------------------------------------------------- 区块 ----
    // 区块标题（竖条 + 标题 + 右对齐补充文字 + 底部分隔线），宽度撑满内容区
    Header* AddHeader(int tab, const char* title, const char* info = nullptr) {
        Header* header = AddTo(tab, content_panel_->Emplace<Header>(title));
        header->SetName("section_header");
        if (info != nullptr) {
            header->setInfo(info);
        }
        return header;
    }

    // Section 的说明行：单行小字浅色（规范：Section Title → 说明 → 控件预览）
    Label* AddDescription(int tab, const char* text) {
        Label* label = AddTo(tab, content_panel_->Emplace<Label>(text));
        label->SetName("section_desc");
        label->setFontSize(Theme::kFontSmall);
        label->setColor(Theme::kTextMuted);
        label->ellipsize = true;
        return label;
    }

    // ------------------------------------------------------- tab 0 按钮 ----
    void BuildPopupPage() {
        page_title_popups_ = AddTo(kTabPopups, content_panel_->Emplace<Label>("按钮 / 弹窗"));
        page_title_popups_->SetName("page_title");
        page_title_popups_->setFontSize(Theme::kFontTitle + 4.0f);

        header_buttons_ = AddHeader(kTabPopups, "按钮变体", "点击 / A 触发，+ 键切说明行");

        // 1 普通按钮（弹窗的确认 / 取消这类提示文字）：文字居中，字号比其它按钮大一档
        TextButton* plain = AddTo(kTabPopups, content_panel_->Emplace<TextButton>("普通按钮"));
        plain->setFontSize(Theme::kFontHeader); // 20（其它按钮正文 16）
        AddStackButton(plain, "btn_text");

        // 2 图标 + 文字：图标占左侧正方形格（格内水平+垂直居中），文字紧跟其右
        IconTextButton* icon_text = AddTo(
            kTabPopups,
            content_panel_->Emplace<IconTextButton>(Icons::Glyph(Icons::Material::Play), "图标 + 文字按钮"));
        icon_text->setSubtitle("图标是正方形格，格内居中；图标在左、文字紧跟其右");
        AddStackButton(icon_text, "btn_icon_text");

        // 3 开关按钮：右侧显示 开/关（开=蓝、关=灰），A/点击切换
        ToggleButton* toggle =
            AddTo(kTabPopups, content_panel_->Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络"));
        toggle->setSubtitle("点击 / A 切换开关状态");
        connect(toggle, &ToggleButton::toggled, this, [](bool on) {
            if (TraceSignal()) {
                std::printf("[signal] toggle = %s\n", on ? "开" : "关");
                std::fflush(stdout);
            }
        });
        AddStackButton(toggle, "btn_toggle");

        // 4 自定义右侧文字按钮
        CustomButton* custom = AddTo(
            kTabPopups, content_panel_->Emplace<CustomButton>(Icons::Glyph(Icons::Material::Storage), "存储路径"));
        custom->setRightText("sdmc:/switch/", Theme::kTeal);
        custom->setSubtitle("右侧文字的内容与颜色都可以改（setRightText）");
        AddStackButton(custom, "btn_custom");

        // 5 LR 选项选择器：右侧 [L] 固定间隔 [R]，L/R 键切换；选项太长就在间隔里滚动
        OptionButton* option = AddTo(
            kTabPopups,
            content_panel_->Emplace<OptionButton>(Icons::Glyph(Icons::Material::ImagePlaceholder), "画面缩放"));
        option->setOptions({"整数缩放", "线性过滤", "CRT 扫描线（像素风，速度慢）"});
        option->setSubtitle("L / R 键切换选项；选项超长会在间隔里滚动");
        connect(option, &OptionButton::selectionChanged, this, [](int index) {
            if (TraceSignal()) {
                std::printf("[signal] option index = %d\n", index);
                std::fflush(stdout);
            }
        });
        AddStackButton(option, "btn_option");

        // 6 LR 数值选择器：右侧 [L] 固定间隔 [R]，初始化时给范围/步长/精度
        ValueButton* value =
            AddTo(kTabPopups, content_panel_->Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "音量"));
        value->setup(65.0f, 0.0f, 100.0f, 5.0f, 0);
        value->setSubtitle("L / R 调值（长按加速），松开才发 valueChanged");
        connect(value, &ValueButton::valueChanged, this, [](float current) {
            if (TraceSignal()) {
                std::printf("[signal] value = %.2f\n", static_cast<double>(current));
                std::fflush(stdout);
            }
        });
        AddStackButton(value, "btn_value");

        // 7 全局开关：一键禁用上面这些按钮（禁用 = 不可聚焦 + 整体置灰）
        disable_toggle_ = AddTo(
            kTabPopups, content_panel_->Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Close), "禁用全部按钮"));
        disable_toggle_->resize(kToggleWidth, Theme::kControlHeight);
        disable_toggle_->setSubtitle("打开 = 全部按钮不可聚焦并置灰");
        connect(disable_toggle_, &ToggleButton::toggled, this, [this](bool off) {
            const bool enabled = !off;
            for (Button* button : buttons_) {
                if (button == disable_toggle_) {
                    continue; // 自己得留着，不然关不回来
                }
                button->enabled = enabled;
            }
        });

        // 图标按钮（Box 预览挪到「基础控件」页了）
        header_icons_ = AddHeader(kTabPopups, "图标按钮", "方形 / 圆形");

        icon_square_ = AddTo(kTabPopups, content_panel_->Emplace<IconButton>(Icons::Glyph(Icons::Material::Settings)));
        icon_square_->setSide(kControlSize);
        icon_square_->setShape(IconButtonShape::RoundedSquare);
        icon_square_->setSubtitle("圆角方形");
        buttons_.push_back(icon_square_);

        icon_circle_ = AddTo(kTabPopups, content_panel_->Emplace<IconButton>(Icons::Glyph(Icons::Material::Favorite)));
        icon_circle_->setSide(kControlSize);
        icon_circle_->setShape(IconButtonShape::Circle);
        icon_circle_->setSubtitle("圆形");
        buttons_.push_back(icon_circle_);

        // ---------------- 弹窗 / 模态框示例（7 个，覆盖需求里的全部形态） ----------------
        header_popups_ = AddHeader(kTabPopups, "弹窗 / 模态框", "A / 点击打开，B 关闭");

        int demo_index = 0;
        AddPopupButton(demo_index++, Icons::Material::Update, "进度对话框", "后台线程跑任务，实时回传进度，跑完自动关闭", [this] {
            ShowProgressDemo();
        });
        AddPopupButton(demo_index++, Icons::Material::Info, "信息对话框", "单按钮：内容 + [确定]", [this] {
            Popups().ShowInfo("操作完成", "存档已写入 sdmc:/switch/GUI_DEV/saves/，可以继续游戏。");
        });
        AddPopupButton(demo_index++, Icons::Material::HelpOutline, "确认对话框", "双按钮：默认焦点在取消，B = 取消", [this] {
            Popups().ShowConfirm("删除存档", "删除后无法恢复，确定继续吗？", [this] { Toasts().ShowSuccess("存档已删除"); });
        });
        AddPopupButton(demo_index++, Icons::Material::SelectAll, "选择对话框", "多按钮竖排，A 确认后执行回调", [this] {
            ShowSelectionDemo();
        });
        AddPopupButton(demo_index++, Icons::Material::Description, "Markdown 弹窗", "标题 / 粗体 / 列表 / 链接 / 代码块 / 图片", [this] {
            ShowMarkdownDemo();
        });
        AddPopupButton(demo_index++, Icons::Material::PhotoLibrary, "图片对话框", "等比缩放 / 居中 / 限高", [this] {
            ShowImageDemo();
        });
        AddPopupButton(demo_index++, Icons::Material::Edit, "自定义页面", "弹窗里放 Tab/选择器/开关/滚动列表", [this] {
            ShowCustomDemo();
        });
    }

    // 弹窗示例按钮：统一大小 + 记录顺序（布局时排两列）
    void AddPopupButton(int demo_index, gui_dev::Icons::Material icon, const char* title, const char* subtitle,
                        std::function<void()> on_click) {
        IconTextButton* button =
            AddTo(kTabPopups, content_panel_->Emplace<IconTextButton>(Icons::Glyph(icon), title));
        button->SetName(std::string("popup_demo_") + std::to_string(demo_index));
        button->setSubtitle(subtitle);
        button->showSubtitle(true);
        button->resize(kStackWidth, Theme::kControlHeight);
        connect(button, &Widget::clicked, this, [handler = std::move(on_click)] { handler(); });
        popup_buttons_.push_back(button);
        buttons_.push_back(button);
    }

    void AddStackButton(Button* button, const char* name) {
        button->SetName(name);
        button->resize(kStackWidth, Theme::kControlHeight);
        button->showSubtitle(true);
        buttons_.push_back(button);
        stack_buttons_.push_back(button);
    }

    // ------------------------------------------------- tab 0 基础控件 ----
    // 一页把「最基础的那批控件」摆全：文本 / 分隔 / 进度 / 图片 / 开关 / 选择 / 标签 / 滚动列表。
    // 走查顺序固定：Label → Separator → ProgressBar → Image → Checkbox → Switch → Radio →
    // Selector → Slider → Tab → ScrollView（焦点上下移动即可逐个验证手柄与触摸）。
    void BuildBasicsPage() {
        // 页首 Page Title（规范 §6）→ 下面每个区块是「Section Title → 说明 → 控件预览」。
        // 不实现 Checkbox / Radio：需要状态选择时用 Switch / Selector / Tab（规范 §9）。
        page_title_basics_ = AddTo(kTabBasics, content_panel_->Emplace<Label>("基础控件"));
        page_title_basics_->SetName("page_title");
        page_title_basics_->setFontSize(Theme::kFontTitle + 4.0f);

        // ---- Box ----
        header_box_ = AddHeader(kTabBasics, "Box", "容器 / 可聚焦控件");
        desc_box_ = AddDescription(kTabBasics, "最基础的矩形：既能当纯容器，也能自己拿焦点。");
        box_ = AddTo(kTabBasics, content_panel_->Emplace<Box>("box"));
        box_->roundCorners(Global::component_style.corner_radius);
        box_->makeFocusable();
        connect(box_, &Box::clicked, this, [this] {
            box_->fillWith(box_->hasFocus() ? Theme::kAccent : Theme::kBgWidget);
        });

        // ---- Label ----
        header_label_ = AddHeader(kTabBasics, "Label", "单行 / 换行 / 多行 / 图标");
        desc_label_ = AddDescription(kTabBasics, "文本内容：可以自动换行，也可以按 \\n 显式断行。");
        label_title_ = AddTo(kTabBasics, content_panel_->Emplace<Label>("基础控件预览"));
        label_title_->setFontSize(Theme::kFontTitle);
        label_body_ = AddTo(kTabBasics, content_panel_->Emplace<Label>(
            "这是一段会自动换行的正文：把宽度交给布局后，Label 会按可用宽度断行，中文与 english mixed 都不会溢出。"));
        label_body_->setWrap(true, 6.0f);
        label_body_->font_size = Theme::kFontSmall;
        label_body_->setColor(Theme::kTextMuted);
        label_multi_ = AddTo(kTabBasics, content_panel_->Emplace<Label>(
            "多行文本用 \\n 显式断行：\n第一行 · 第二行 · 第三行"));
        label_multi_->setFontSize(Theme::kFontSmall);
        label_icon_ = AddTo(kTabBasics, content_panel_->Emplace<Label>("带图标的标签（图标是 Material 字形）"));
        label_icon_->setIcon(Icons::Glyph(Icons::Material::FavoriteBorder));
        label_icon_->setFontSize(Theme::kFontSmall);

        // ---- Button ----
        header_button_ = AddHeader(kTabBasics, "Button", "执行操作的基础控件");
        desc_button_ = AddDescription(kTabBasics, "Focus 后按 A，或直接点击 / 触摸，都会触发 clicked。");
        preview_button_ = AddTo(kTabBasics, content_panel_->Emplace<TextButton>("普通按钮"));
        preview_button_->SetName("btn_preview");
        preview_button_->setFontSize(Theme::kFontHeader);
        preview_button_->showSubtitle(false);
        connect(preview_button_, &Widget::clicked, this, [this] { Toasts().ShowSuccess("按钮被触发"); });

        // ---- Image ----
        header_image_ = AddHeader(kTabBasics, "Image", "等比缩放 / 居中 / 限高 / 裁剪");
        desc_image_ = AddDescription(kTabBasics, "资源来自 assets/img/test.png（876×876）。");
        test_tex_ = gui_dev::TextureRef(ui().GetBackend(), "img/test.png");
        const float source_w = static_cast<float>(test_tex_.Width());
        const float source_h = static_cast<float>(test_tex_.Height());
        image_cap_contain_ = AddDescription(kTabBasics, "Contain：等比缩放，完整装进内容区并居中");
        image_contain_ = AddTo(kTabBasics, content_panel_->Emplace<Image>());
        image_contain_->setTexture(test_tex_.ImGuiRef(), source_w, source_h);
        image_contain_->setFit(Image::Fit::Contain);
        image_cap_cover_ = AddDescription(kTabBasics, "Cover：等比放大填满内容区，超出部分裁掉");
        image_cover_ = AddTo(kTabBasics, content_panel_->Emplace<Image>());
        image_cover_->setTexture(test_tex_.ImGuiRef(), source_w, source_h);
        image_cover_->setFit(Image::Fit::Cover);
        image_cap_native_ = AddDescription(kTabBasics, "None：按原始像素显示，超出的部分裁掉");
        image_native_ = AddTo(kTabBasics, content_panel_->Emplace<Image>());
        image_native_->setTexture(test_tex_.ImGuiRef(), source_w, source_h);
        image_native_->setFit(Image::Fit::None);
        image_cap_limit_ = AddDescription(kTabBasics, "限高（max_size）+ 居中：高度被限制住，宽度等比跟随");
        image_limited_ = AddTo(kTabBasics, content_panel_->Emplace<Image>());
        image_limited_->setTexture(test_tex_.ImGuiRef(), source_w, source_h);
        image_limited_->setFit(Image::Fit::Contain);
        image_limited_->max_size = ImVec2(0.0f, 190.0f);

        // ---- ProgressBar ----
        header_progress_ = AddHeader(kTabBasics, "ProgressBar", "确定进度 / 不确定进度");
        desc_progress_ = AddDescription(kTabBasics, "确定进度会平滑跟随，不确定进度是跑动条。");
        progress_ = AddTo(kTabBasics, content_panel_->Emplace<ProgressBar>());
        progress_->setLabel("正在加载核心");
        progress_->setBarHeight(12.0f);
        progress_wait_ = AddTo(kTabBasics, content_panel_->Emplace<ProgressBar>());
        progress_wait_->setIndeterminate(true);
        progress_wait_->setBarHeight(12.0f);
        progress_wait_->setLabel("正在查找更新…");

        // ---- Selector ----
        header_selector_ = AddHeader(kTabBasics, "Selector", "在多个选项之间选择");
        desc_selector_ = AddDescription(kTabBasics, "左右键（L / R 或 ← →）切换选项，触摸可直接点。");
        selector_ = AddTo(kTabBasics, content_panel_->Emplace<OptionButton>(
                                          Icons::Glyph(Icons::Material::ImagePlaceholder), "画面缩放"));
        selector_->setOptions({"整数缩放", "线性过滤", "CRT 扫描线"});
        connect(selector_, &OptionButton::selectionChanged, this, [](int index) {
            if (TraceSignal()) {
                std::printf("[signal] selector = %d\n", index);
                std::fflush(stdout);
            }
        });

        // ---- Switch ----
        header_switch_ = AddHeader(kTabBasics, "Switch", "开 / 关状态");
        desc_switch_ = AddDescription(kTabBasics, "按 A 或点击切换；状态变化发 toggled 信号。");
        switch_ = AddTo(kTabBasics,
                        content_panel_->Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络"));
        switch_->setChecked(true, false);
        connect(switch_, &ToggleButton::toggled, this, [](bool on) {
            if (TraceSignal()) {
                std::printf("[signal] switch = %s\n", on ? "开" : "关");
                std::fflush(stdout);
            }
        });

        // ---- Slider ----
        header_slider_ = AddHeader(kTabBasics, "Slider", "在范围内连续调值");
        desc_slider_ = AddDescription(kTabBasics, "长按左右键连续调值（带加速），松开才发 valueChanged。");
        slider_ = AddTo(kTabBasics,
                        content_panel_->Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "音量"));
        slider_->setup(65.0f, 0.0f, 100.0f, 5.0f, 0);
        connect(slider_, &ValueButton::valueChanged, this, [](float value) {
            if (TraceSignal()) {
                std::printf("[signal] slider = %.0f\n", static_cast<double>(value));
                std::fflush(stdout);
            }
        });

        // ---- Tab ----
        header_tabs_ = AddHeader(kTabBasics, "Tab", "在一组标签之间切换");
        desc_tabs_ = AddDescription(kTabBasics, "L / R 切换标签，选中项停在中间（胶囊高亮）。");
        tabs_demo_ = AddTo(kTabBasics, content_panel_->Emplace<CapsuleTabs>());
        tabs_demo_->setLabels({"所有", "GBA", "GBC", "FC", "NDS", "3DS"}, 0);
        connect(tabs_demo_, &CapsuleTabs::selectionChanged, this, [this](int index) {
            (void)index;
            if (TraceSignal()) {
                std::printf("[signal] basics_tab = %d\n", index);
                std::fflush(stdout);
            }
        });

        // ---- Separator ----
        header_separator_ = AddHeader(kTabBasics, "Separator", "分隔线");
        desc_separator_ = AddDescription(kTabBasics, "水平分隔线，颜色与缩进都跟主题走。");
        separator_ = AddTo(kTabBasics, content_panel_->Emplace<Separator>());
    }

    // ------------------------------------------------------- tab 1 徽标 ----
    // 徽标墙：两列、每个徽标同样大小、文字一律白色（不套容器 Box，直接画在面板上）
    void BuildBadgePage() {
        constexpr int kBadgeCount = 15; // 14 个机种 + 其它
        header_badges_ = AddHeader(kTabBadges, "机种徽标", "15 个机种");
        for (int i = 0; i < kBadgeCount; ++i) {
            const EmuPlatform platform = i < 14 ? static_cast<EmuPlatform>(i + 1) : EmuPlatform::Unknown;
            Badge* badge = AddTo(kTabBadges, content_panel_->Emplace<Badge>(platform));
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
    }

    // ------------------------------------------------------- tab 2 提示 ----
    void BuildToastPage() {
        header_toasts_ = AddHeader(kTabToasts, "状态提示", "一个按钮一行业务代码");

        IconButton* ok = AddToastButton(Icons::Material::CheckCircle, "btn_toast_ok", "成功");
        connect(ok, &IconButton::clicked, this, [this] { Toasts().ShowSuccess("保存状态成功"); });

        IconButton* error = AddToastButton(Icons::Material::ErrorOutline, "btn_toast_error", "失败");
        connect(error, &IconButton::clicked, this, [this] { Toasts().ShowError("读取状态失败"); });

        IconButton* info = AddToastButton(Icons::Material::Info, "btn_toast_info", "信息");
        connect(info, &IconButton::clicked, this, [this] { Toasts().ShowInfo("正在加载游戏..."); });

        IconButton* long_text = AddToastButton(Icons::Material::Description, "btn_toast_long", "长文本");
        connect(long_text, &IconButton::clicked, this, [this] {
            // 长文本会自动换行，高度变化会让后面的 Toast 重新排位
            Toasts().ShowInfo("正在加载游戏资源，正在校验存档完整性，请稍候，不要关闭主机电源。");
        });
    }

    IconButton* AddToastButton(gui_dev::Icons::Material icon, const char* name, const char* caption) {
        IconButton* button = AddTo(kTabToasts, content_panel_->Emplace<IconButton>(Icons::Glyph(icon)));
        button->SetName(name);
        button->setSide(kControlSize);
        button->setShape(IconButtonShape::RoundedSquare);
        button->setSubtitle(caption);
        toast_buttons_.push_back(button);
        buttons_.push_back(button); // 蹭一下「+ 键开关说明行」的逻辑
        return button;
    }

    // ------------------------------------------------------- tab 3 导航 ----
    // 胶囊标签条（学 GBAStation 游戏库顶部的机种轮播）：
    // 选中项停在条带中心，L/R 或点标签切换；胶囊是 104x42 圆角 21 的半透明高亮，
    // 字号 17→22、透明度 0.42→1.0 按离中心的距离衰减。
    void BuildNavPage() {
        header_nav_ = AddHeader(kTabNav, "胶囊标签条", "L / R 或点标签切换");

        capsule_ = AddTo(kTabNav, content_panel_->Emplace<CapsuleTabs>());
        capsule_->SetName("capsule_tabs");
        capsule_->setLabels({"所有", "GBA", "GBC", "FC", "SFC", "NDS", "3DS", "MD"}, 1);
        capsule_->size.x = kCapsuleWidth; // 高度按内容自适应（胶囊 42 + 阴影余量）
        connect(capsule_, &CapsuleTabs::selectionChanged, this, [this](int index) {
            if (TraceSignal()) {
                std::printf("[signal] capsule index = %d (%s)\n", index, capsule_->currentLabel());
                std::fflush(stdout);
            }
        });
        connect(capsule_, &CapsuleTabs::activated, this, [this](int index) {
            // A 键 / 再点一次已选中项
            Toasts().ShowInfo(std::string("进入 ") + capsule_->labels[static_cast<std::size_t>(index)]);
        });
    }

    // ------------------------------------------------- 弹窗示例实现 ----
    // 这些方法就是业务侧的真实调用方式：Show* 拿回 Popup*，之后按需 setXxx / Close。
    void ShowSelectionDemo() {
        const char* names[] = {"mGBA", "VBA-M", "NanoBoyAdvance"};
        const char* notes[] = {"兼容性最好", "老牌核心", "精度最高"};
        std::vector<Popup::ButtonSpec> options;
        for (int i = 0; i < 3; ++i) {
            Popup::ButtonSpec spec;
            spec.text = std::string(names[i]) + "    " + notes[i];
            spec.icon = Icons::Glyph(Icons::Material::Play);
            spec.primary = (i == 0);
            spec.close_on_click = false; // 选择弹窗保持开着，确认弹窗叠在它上面（弹窗栈）
            // 选择之后在它上面再叠一个确认弹窗：验证弹窗栈（关闭顶部后焦点回到下面这层）
            spec.on_click = [this, name = std::string(names[i])] {
                Popups().ShowConfirm("使用该核心启动？", "已选择 " + name + "，是否立即用它加载当前 ROM？",
                                     [this, name] { Toasts().ShowSuccess("正在用 " + name + " 启动…"); });
            };
            options.push_back(std::move(spec));
        }
        Popup::ButtonSpec cancel;
        cancel.text = "取消";
        cancel.on_click = [this] { Toasts().ShowInfo("已取消选择"); };
        options.push_back(std::move(cancel));
        Popups().ShowSelection("选择模拟核心", "这个 ROM 有多个可用核心，选一个：", std::move(options),
                               PopupButtonLayout::Vertical);
    }

    // Markdown 弹窗：标题 / 粗体 / 斜体 / 列表 / 链接 / 代码块 / 图片，全部走真实 Markdown 解析
    // 规范：Markdown 富文本只需要「文字 + 图片」，不做图表 / Mermaid / WebView。
    static const char* MarkdownDemoText() {
        return
            "# 游戏说明\n"
            "\n"
            "**GUI_DEV** 是一个跨平台 UI 组件库，支持 *Switch / Windows / macOS*，\n"
            "同一份组件代码在三个平台上保持一致。\n"
            "\n"
            "## 操作方式\n"
            "\n"
            "- A：确认\n"
            "- B：返回\n"
            "- L / R：翻页\n"
            "\n"
            "## 界面预览\n"
            "\n"
            "![界面预览](img/test.png)\n"
            "\n"
            "## 目录结构\n"
            "\n"
            "```\n"
            "component_view/\n"
            "  components/   基础控件\n"
            "  popup/        弹窗\n"
            "  UILayer.h     层级\n"
            "```\n"
            "\n"
            "详细说明见 [组件文档](docs/component-view.md)。\n";
    }

    // Markdown 图片：![alt](path) 走这里拿纹理（懒加载 + 缓存；控件层不碰平台接口）
    MarkdownView::ImageResolver MakeImageResolver() {
        return [this](const std::string& path, ImTextureRef& texture, float& width, float& height) {
            auto found = markdown_textures_.find(path);
            if (found == markdown_textures_.end()) {
                found = markdown_textures_.emplace(path, gui_dev::TextureRef(ui().GetBackend(), path.c_str())).first;
            }
            if (!found->second.Valid()) {
                return false; // 没有这张图：imgui_markdown 会退化成链接文本
            }
            texture = found->second.ImGuiRef();
            width = static_cast<float>(found->second.Width());
            height = static_cast<float>(found->second.Height());
            return true;
        };
    }

    void ShowMarkdownDemo() {
        // 真实调用 PopupManager + imgui_markdown：标题/粗体/斜体/列表/链接/代码块/图片
        Popups().ShowMarkdown("游戏说明", MarkdownDemoText(), MakeImageResolver(), 300.0f, PopupKind::Info);
    }

    void ShowImageDemo() {
        Popups().ShowImage("界面预览", test_tex_.ImGuiRef(), static_cast<float>(test_tex_.Width()),
                           static_cast<float>(test_tex_.Height()));
    }

    // 自定义页面弹窗：内容也是单栏纵向，用的都是现有控件（Tab / Selector / Switch / Slider / Button）
    void ShowCustomDemo() {
        Popups().ShowCustom("游戏设置", [this](Widget& content) {
            CapsuleTabs* tabs = content.Emplace<CapsuleTabs>();
            tabs->setLabels({"通用", "画面", "音频"}, 0);
            tabs->size = ImVec2(0.0f, 58.0f);
            connect(tabs, &CapsuleTabs::selectionChanged, this, [this](int index) {
                Toasts().ShowInfo("切到第 " + std::to_string(index + 1) + " 组设置");
            });

            Separator* line = content.Emplace<Separator>();
            line->setThickness(1.0f);

            OptionButton* scaler = content.Emplace<OptionButton>(Icons::Glyph(Icons::Material::ImagePlaceholder),
                                                               "画面缩放");
            scaler->setOptions({"整数缩放", "线性过滤", "CRT 扫描线"});
            scaler->resize(0.0f, Theme::kControlHeight);

            ToggleButton* fast = content.Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Memory), "多线程渲染");
            fast->setChecked(true, false);
            fast->resize(0.0f, Theme::kControlHeight);

            ValueButton* volume = content.Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "主音量");
            volume->setup(70.0f, 0.0f, 100.0f, 5.0f, 0);
            volume->resize(0.0f, Theme::kControlHeight);

            // 内容里的按钮不关弹窗（close_on_click = false），只做业务动作
            TextButton* apply = content.Emplace<TextButton>("应用");
            apply->resize(0.0f, Theme::kControlHeight);
            connect(apply, &Widget::clicked, this, [this] { Toasts().ShowSuccess("设置已应用（弹窗不关闭）"); });
        });
    }

    // ---- 进度弹窗 + 真实后台线程（UI 线程只读进度并刷 UI，不做阻塞操作）----
    void ShowProgressDemo() {
        Popups().ShowProgress("正在扫描 ROM", "准备中…");
        scan_active_ = true;
        scan_step_.store(0);
        scan_finished_.store(false);
        if (scan_thread_.joinable()) {
            scan_thread_.join(); // 上一次任务已经结束，回收线程
        }
        scan_thread_ = std::thread([](std::atomic<int>* step, std::atomic<bool>* finished) {
            constexpr int kTotal = 48;
            for (int i = 1; i <= kTotal; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(55));
                step->store(i);
            }
            finished->store(true);
        }, &scan_step_, &scan_finished_);
    }

    void UpdateProgressDemo() {
        if (!scan_active_) {
            return;
        }
        Popup* popup = Popups().Find("progress");
        if (popup == nullptr) {
            scan_active_ = false; // 已被关闭/回收：不持有裸指针，避免悬垂
            return;
        }
        constexpr int kTotal = 48;
        const int step = scan_step_.load();
        const float value = cv::Clampf(static_cast<float>(step) / static_cast<float>(kTotal), 0.0f, 1.0f);
        popup->setProgress(value);
        popup->setMessage("已扫描 " + std::to_string(step * 31) + " 个文件");
        if (scan_finished_.load()) {
            popup->setKind(PopupKind::Success);
            popup->setMessage("扫描完成：共 " + std::to_string(kTotal * 31) + " 个文件");
            popup->Close(); // 任务完成 → 自动关闭 → 焦点恢复到打开前的控件
            scan_active_ = false;
            Toasts().ShowSuccess("ROM 扫描完成");
        }
    }

    // ---------------------------------------------------------- 布局 ----
    void LayoutPanels() {
        const float content_w = Global::canvas_size.x - kMargin * 2.0f;
        const float content_h = Global::canvas_size.y - kMargin * 2.0f;
        const float tab_w = Theme::kTabColumnWidth + kTabPanelPadding * 2.0f;
        const float page_w = gui_dev::cv::Maxf(content_w - tab_w - kPanelGap, 0.0f);

        tab_panel_->moveTo(kMargin, kMargin);
        tab_panel_->resize(tab_w, content_h);
        content_panel_->moveTo(kMargin + tab_w + kPanelGap, kMargin);
        content_panel_->resize(page_w, content_h);

        // TabColumn 撑满面板内容区，底部留一行给主题/焦点框开关
        tab_column_->position = ImVec2(0.0f, 0.0f);
        tab_column_->size = ImVec2(Theme::kTabColumnWidth, TabInnerHeight() - kControlSize - kRowGap);
        const float controls_y = TabInnerHeight() - kControlSize;
        focus_style_button_->moveTo(0.0f, controls_y);
        theme_button_->moveTo(kControlSize + kRowGap, controls_y);
    }

    void LayoutContent() {
        const float left = 0.0f; // 位置都相对内容面板的内容区
        const float top = 0.0f;

        // 规范：子页面一律「单栏 + 纵向排列」。
        // 每个区块 = Section Title → 说明 → 控件预览，间距统一，超出屏幕由整页滚动。
        {
            const float w = ContentWidth();
            float y = top;
            // Page Title
            page_title_basics_->SetPosition(left, y);
            page_title_basics_->size.x = w;
            y += 44.0f;
            auto placeLabel = [&](Label* label, float height) {
                label->SetPosition(left, y);
                label->size.x = w;
                label->size.y = height;
                y += height;
            };
            auto section = [&](Header* header, Label* desc) {
                header->SetPosition(left, y);
                header->size.x = w;
                y += kHeaderHeight + 2.0f;
                if (desc != nullptr) {
                    placeLabel(desc, kDescHeight);
                }
                y += kPreviewGap;
            };
            auto widget = [&](Widget* object, float height, float gap_after = kSectionGap) {
                object->SetPosition(left, y);
                object->size.x = w;
                object->size.y = height;
                y += height + gap_after;
            };

            // ---- Box ----
            section(header_box_, desc_box_);
            box_->SetPosition(left + (w - 124.0f) * 0.5f, y);
            box_->size = ImVec2(124.0f, 124.0f);
            y += 124.0f + kSectionGap;

            // ---- Label ----
            section(header_label_, desc_label_);
            placeLabel(label_title_, 30.0f);
            placeLabel(label_body_, 44.0f);
            placeLabel(label_multi_, 46.0f);
            placeLabel(label_icon_, 26.0f);
            y += kSectionGap;

            // ---- Button ----
            section(header_button_, desc_button_);
            preview_button_->resize(w, Theme::kControlHeight);
            preview_button_->moveTo(left, y);
            y += Theme::kControlHeight + kSectionGap;

            // ---- Image（四种能力，纵向依次预览）----
            section(header_image_, desc_image_);
            auto imagePreview = [&](Label* caption, Image* image, float height) {
                placeLabel(caption, kCaptionHeight);
                widget(image, height, kImageGap);
            };
            imagePreview(image_cap_contain_, image_contain_, 150.0f);
            imagePreview(image_cap_cover_, image_cover_, 150.0f);
            imagePreview(image_cap_native_, image_native_, 170.0f);
            imagePreview(image_cap_limit_, image_limited_, 190.0f);
            y += kSectionGap - kImageGap;

            // ---- ProgressBar ----
            section(header_progress_, desc_progress_);
            progress_->SetPosition(left, y);
            progress_->size = ImVec2(w, 0.0f);
            y += 40.0f;
            progress_wait_->SetPosition(left, y);
            progress_wait_->size = ImVec2(w, 0.0f);
            y += 40.0f + kSectionGap;

            // ---- Selector（L / R 选项）----
            section(header_selector_, desc_selector_);
            selector_->resize(w, Theme::kControlHeight);
            selector_->moveTo(left, y);
            y += Theme::kControlHeight + kSectionGap;

            // ---- Switch ----
            section(header_switch_, desc_switch_);
            switch_->resize(w, Theme::kControlHeight);
            switch_->moveTo(left, y);
            y += Theme::kControlHeight + kSectionGap;

            // ---- Slider（L / R 数值）----
            section(header_slider_, desc_slider_);
            slider_->resize(w, Theme::kControlHeight);
            slider_->moveTo(left, y);
            y += Theme::kControlHeight + kSectionGap;

            // ---- Tab（胶囊标签条）----
            section(header_tabs_, desc_tabs_);
            widget(tabs_demo_, 58.0f);

            // ---- Separator ----
            section(header_separator_, desc_separator_);
            separator_->SetPosition(left, y);
            separator_->size = ImVec2(w, 9.0f);
        }

        // ---- 按钮 + 弹窗页（tab 1）：同样单栏纵向 ----
        {
            const float w = ContentWidth();
            float y = top;
            const float gap = kSectionGap;

            page_title_popups_->SetPosition(left, y);
            page_title_popups_->size.x = w;
            y += 44.0f;

            header_buttons_->SetPosition(left, y);
            header_buttons_->size.x = w;
            y += kHeaderHeight + kPreviewGap;
            for (Button* button : stack_buttons_) {
                button->resize(w, Theme::kControlHeight);
                button->moveTo(left, y);
                y += Theme::kControlHeight + kRowGap;
            }
            y += gap;

            header_icons_->SetPosition(left, y);
            header_icons_->size.x = w;
            y += kHeaderHeight + kPreviewGap;
            // 窄控件居中：不然空间导航会"跳过"它们（宽行永远更近）
            icon_square_->moveTo(left + (w - kControlSize) * 0.5f, y);
            y += kControlSize + kRowGap;
            icon_circle_->moveTo(left + (w - kControlSize) * 0.5f, y);
            y += kControlSize + gap;

            header_popups_->SetPosition(left, y);
            header_popups_->size.x = w;
            y += kHeaderHeight + kPreviewGap;
            for (IconTextButton* button : popup_buttons_) {
                button->resize(w, Theme::kControlHeight);
                button->moveTo(left, y);
                y += Theme::kControlHeight + kRowGap;
            }
            y += gap;

            disable_toggle_->resize(w, Theme::kControlHeight);
            disable_toggle_->moveTo(left, y);
        }

        // ---- 徽标页：标题下面、内容区里居中 ----
        const float header_w = ContentWidth();
        header_badges_->position = ImVec2(left, top);
        header_badges_->size.x = header_w;
        const int rows = static_cast<int>((badges_.size() + 1) / 2);
        const float wall_w = kBadgeWidth * 2.0f + kBadgeGapX;
        const float wall_h = static_cast<float>(rows) * kBadgeHeight + static_cast<float>(rows - 1) * kBadgeGapY;
        const float area_top = top + kHeaderHeight + kSectionGap;
        const float area_h = top + ContentHeight() - area_top;
        const float wall_x = left + (header_w - wall_w) * 0.5f;
        const float wall_y = area_top + (area_h - wall_h) * 0.5f;
        for (std::size_t i = 0; i < badges_.size(); ++i) {
            badges_[i]->moveTo(wall_x + static_cast<float>(i % 2) * (kBadgeWidth + kBadgeGapX),
                               wall_y + static_cast<float>(i / 2) * (kBadgeHeight + kBadgeGapY));
        }

        // ---- 提示页：标题下面一行触发按钮 ----
        header_toasts_->position = ImVec2(left, top);
        header_toasts_->size.x = header_w;
        for (std::size_t i = 0; i < toast_buttons_.size(); ++i) {
            toast_buttons_[i]->moveTo(left + static_cast<float>(i) * (kControlSize + 12.0f),
                                      top + kHeaderHeight + kHeaderGap);
        }

        // ---- 导航页：标题下面居中放胶囊条 ----
        header_nav_->position = ImVec2(left, top);
        header_nav_->size.x = header_w;
        {
            const float nav_top = top + kHeaderHeight + kSectionGap;
            const float nav_h = top + ContentHeight() - nav_top;
            capsule_->position = ImVec2(left + (header_w - kCapsuleWidth) * 0.5f, nav_top + (nav_h - 50.0f) * 0.5f);
            capsule_->size.x = kCapsuleWidth;
        }
    }

    // 一键切浅色 / 深色：换调色板 + 约定样式，再让整棵组件树重新取色
    void ToggleTheme() {
        Theme::ToggleMode();
        Theme::ApplyToImGui();
        RefreshTheme(); // 内部：Global::ApplyTheme() + 根节点装饰复位 + 整棵树重新取色
        ApplyPanelColors(); // 面板底色是显式设的，不跟主题，这里补一次
        theme_button_->setIcon(Icons::Glyph(ThemeIcon()));
        theme_button_->setSubtitle(ThemeName());
    }

    void ToggleFocusStyle() {
        Global::pause_focus_frame = !Global::pause_focus_frame;
        focus_style_button_->setIcon(Icons::Glyph(FocusStyleIcon()));
        focus_style_button_->setSubtitle(FocusStyleName());
    }

    // 子页面里按 B：焦点回到左边的 tab 列（A 进内容、B 回列，形成来回）
    void OnInput() override {
        // 弹窗打开时页面级快捷键全部让位：B 已经由弹窗处理，这里不能再把焦点挪回 tab 列
        if (!Popups().Empty()) {
            return;
        }
        if (Global::pad.Pressed(InputAction::Cancel) && Global::Available(InputAction::Cancel)) {
            Widget* focused = Global::focused;
            if (focused != nullptr && content_panel_->ContainsDescendant(focused)) {
                tab_column_->FocusCurrentItem();
                Global::MarkConsumed(InputAction::Cancel);
            }
        }
        if (Global::pad.Pressed(InputAction::Menu) && Global::Available(InputAction::Menu)) {
            subtitle_on_ = !subtitle_on_;
            for (Button* button : buttons_) {
                button->showSubtitle(subtitle_on_);
            }
            Global::MarkConsumed(InputAction::Menu);
        }
    }

private:
    // 0 = 基础控件（不再放按钮），1 = 按钮 + 弹窗示例，后面是原有几页
    enum TabIndex { kTabBasics = 0, kTabPopups, kTabBadges, kTabToasts, kTabNav, kTabCount };

    // 子页面入 / 退场（对齐 examples/pause_menu 的菜单出场参数）
    struct PageAnim {
        float enter_time = 1.0e9f; // 入场已经播了多少秒
        float exit = 1.0f;         // 0..1 退场进度
    };

    static constexpr int kContentZone = 2;         // 内容区焦点分区（TabColumn 是 1）
    static constexpr float kMargin = 20.0f;        // 页面四边留白
    static constexpr float kPanelGap = 16.0f;      // 两个面板之间
    static constexpr float kTabPanelPadding = 12.0f;
    static constexpr float kContentPanelPadding = 18.0f;
    static constexpr float kDescHeight = 24.0f;  // 区块说明行高
    static constexpr float kPreviewGap = 12.0f;  // 说明与控件预览之间
    static constexpr float kCaptionHeight = 22.0f;
    static constexpr float kImageGap = 14.0f;    // 同一区块内多个图片预览之间
    static constexpr float kStackWidth = 396.0f;
    static constexpr float kToggleWidth = 340.0f;
    static constexpr float kControlSize = Theme::kControlHeight; // 统一控件尺寸（56）
    static constexpr float kCapsuleWidth = 440.0f;
    static constexpr float kHeaderHeight = 58.0f;
    static constexpr float kHeaderGap = 8.0f;   // 标题到本区块内容
    static constexpr float kRowGap = 8.0f;      // 行间距
    static constexpr float kSectionGap = 16.0f; // 上一块内容到下一个标题
    static constexpr float kPageEnterDuration = 0.22f; // 子页入场时长（快速弹入）
    static constexpr float kPageExitDuration = 0.12f;  // 子页退场时长
    static constexpr float kPageEnterOffset = 52.0f;   // 入场时从右边弹进来的距离
    static constexpr float kPageExitOffset = 18.0f;    // 退场时往左滑出

    // 面板的内容区尺寸（= 面板尺寸 - padding*2 - 边框*2）。位置都相对它，所以从 0 开始。
    // 注意边框也要减：Box 走 Global::component_style 有 1px 边框，不减的话内容会超出 2px
    // （表现为面板底部多出一条横向滚动条）。
    static float PanelInner(float panel_size, float padding) {
        return panel_size - (padding + Global::component_style.border_width) * 2.0f;
    }
    static float TabPanelWidth() { return Theme::kTabColumnWidth + kTabPanelPadding * 2.0f; }
    static float ContentPanelWidth() {
        return Global::canvas_size.x - kMargin * 2.0f - TabPanelWidth() - kPanelGap;
    }
    static float ContentWidth() { return PanelInner(ContentPanelWidth(), kContentPanelPadding); }
    static float ContentHeight() {
        return PanelInner(Global::canvas_size.y - kMargin * 2.0f, kContentPanelPadding);
    }
    static float TabInnerHeight() {
        return PanelInner(Global::canvas_size.y - kMargin * 2.0f, kTabPanelPadding);
    }

    static gui_dev::Icons::Material ThemeIcon() {
        // 图标显示「当前主题」：浅色 = 太阳，深色 = 月亮
        return Theme::IsLight() ? Icons::Material::LightMode : Icons::Material::DarkMode;
    }
    static const char* ThemeName() { return Theme::IsLight() ? "浅色" : "深色"; }
    static gui_dev::Icons::Material FocusStyleIcon() {
        return Global::pause_focus_frame ? Icons::Material::CheckBoxOutline : Icons::Material::SelectAll;
    }
    static const char* FocusStyleName() { return Global::pause_focus_frame ? "菜单框" : "流光框"; }

    // 徽标墙几何：两列，统一尺寸
    static constexpr float kBadgeWidth = 92.0f;
    static constexpr float kBadgeHeight = Theme::kBadgeHeight; // 统一标签高（26）
    static constexpr float kBadgeGapX = 14.0f;
    static constexpr float kBadgeGapY = 8.0f;

    // 基础控件页（tab 0）：单栏纵向，每个区块 = Section Title + 说明 + 控件预览
    Label* page_title_basics_ = nullptr;
    Label* page_title_popups_ = nullptr;
    Header* header_box_ = nullptr;
    Header* header_label_ = nullptr;
    Header* header_button_ = nullptr;
    Header* header_image_ = nullptr;
    Header* header_progress_ = nullptr;
    Header* header_selector_ = nullptr;
    Header* header_switch_ = nullptr;
    Header* header_slider_ = nullptr;
    Header* header_tabs_ = nullptr;
    Header* header_separator_ = nullptr;
    Label* desc_box_ = nullptr;
    Label* desc_label_ = nullptr;
    Label* desc_button_ = nullptr;
    Label* desc_image_ = nullptr;
    Label* desc_progress_ = nullptr;
    Label* desc_selector_ = nullptr;
    Label* desc_switch_ = nullptr;
    Label* desc_slider_ = nullptr;
    Label* desc_tabs_ = nullptr;
    Label* desc_separator_ = nullptr;
    TextButton* preview_button_ = nullptr;
    Label* label_title_ = nullptr;
    Label* label_body_ = nullptr;
    Label* label_multi_ = nullptr;
    Label* label_icon_ = nullptr;
    Separator* separator_ = nullptr;
    ProgressBar* progress_ = nullptr;
    ProgressBar* progress_wait_ = nullptr;
    // Image：四种能力各来一个预览（等比缩放 / 填满裁切 / 原始尺寸 / 限高居中）
    Label* image_cap_contain_ = nullptr;
    Label* image_cap_cover_ = nullptr;
    Label* image_cap_native_ = nullptr;
    Label* image_cap_limit_ = nullptr;
    Image* image_contain_ = nullptr;
    Image* image_cover_ = nullptr;
    Image* image_native_ = nullptr;
    Image* image_limited_ = nullptr;
    gui_dev::TextureRef test_tex_;              // assets/img/test.png
    gui_dev::TextureRef gradient_tex_;          // assets/img/border_gradient.png
    std::map<std::string, gui_dev::TextureRef> markdown_textures_; // Markdown 图片纹理（懒加载）
    ToggleButton* switch_ = nullptr;
    OptionButton* selector_ = nullptr;
    ValueButton* slider_ = nullptr;
    CapsuleTabs* tabs_demo_ = nullptr;
    float basics_progress_ = 0.0f;

    // 按钮 / 弹窗页（tab 1）
    Header* header_popups_ = nullptr;
    std::vector<IconTextButton*> popup_buttons_;

    // 进度弹窗的后台任务状态（后台线程只写原子量，UI 线程只读）
    std::atomic<int> scan_step_{0};
    std::atomic<bool> scan_finished_{false};
    std::thread scan_thread_;
    bool scan_active_ = false;

    std::vector<Widget*> pages_[kTabCount];
    PageAnim page_anim_[kTabCount];
    int active_tab_ = 0;
    int exiting_tab_ = -1;
    std::vector<Button*> buttons_;       // + 键统一切说明行 / 统一禁用
    std::vector<Button*> stack_buttons_; // 按钮页左侧那一列（按索引排位置）
    std::vector<IconButton*> toast_buttons_;
    std::vector<Badge*> badges_;
    Box* tab_panel_ = nullptr;
    Box* content_panel_ = nullptr;
    TabColumn* tab_column_ = nullptr;
    Header* header_buttons_ = nullptr;
    Header* header_icons_ = nullptr;
    Header* header_badges_ = nullptr;
    Header* header_toasts_ = nullptr;
    Header* header_nav_ = nullptr;
    ToggleButton* disable_toggle_ = nullptr;
    Box* box_ = nullptr;
    IconButton* icon_square_ = nullptr;
    IconButton* icon_circle_ = nullptr;
    CapsuleTabs* capsule_ = nullptr;
    IconButton* theme_button_ = nullptr;
    IconButton* focus_style_button_ = nullptr;
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
        // 帧率上限：GUI_DEV_MAX_FPS=30 / 60；0 = 不限
        if (const char* value = std::getenv("GUI_DEV_MAX_FPS")) {
            cfg.max_fps = std::atoi(value);
        }
#if defined(GUI_DEV_PLATFORM_switch)
        cfg.vsync = false; // Switch 由 libnx 垂直同步，SDL 再同步会拖帧
        if (cfg.max_fps <= 0) {
            cfg.max_fps = 60; // 不限帧的话 UI 会把 GPU 跑满，掌机上没必要
        }
#endif
    }

    void OnStart(gui_dev::UiContext& ui) override {
        // 720p 手持基准下整体放大 1.25 倍（右侧控制列的放大/缩小还能再调）
        ui.SetUiZoom(kDefaultZoom);
        // 默认浅色主题（桌面端白底看着舒服），右侧控制列的按钮还能一键切成深色
        // 初始主题：GUI_DEV_THEME=dark 可以深色启动（截图核对两套主题用）
        const bool start_dark = [] {
            const char* value = std::getenv("GUI_DEV_THEME");
            return value != nullptr && (value[0] == 'd' || value[0] == 'D');
        }();
        gui_dev::cv::Theme::SetMode(start_dark ? gui_dev::cv::Theme::ThemeMode::Dark
                                               : gui_dev::cv::Theme::ThemeMode::Light);
        gui_dev::cv::Theme::ApplyToImGui();
        gui_dev::cv::Global::ApplyTheme();
        Scenes().Reset(std::make_unique<HostScene>());

        page_ = std::make_unique<DemoPage>();
        page_->Bind(ui);

        if (const char* value = std::getenv("GUI_DEV_PERF")) {
            perf_ = value != nullptr && value[0] != '0';
        }
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

        // GUI_DEV_PERF=1：每秒打一行统计（帧时间 / FPS / draw call / 顶点数），排查性能用
        if (perf_) {
            perf_accum_ += dt;
            ++perf_frames_;
            if (perf_accum_ >= 1.0f) {
                std::printf("[perf] %.1f fps  frame=%.2f ms  drawcalls=%d  verts=%d  inds=%d\n",
                            static_cast<double>(perf_frames_ / perf_accum_),
                            static_cast<double>(perf_accum_ * 1000.0f / perf_frames_),
                            ui.LastDrawCalls(), ui.LastVertices(), ui.LastIndices());
                std::fflush(stdout);
                perf_accum_ = 0.0f;
                perf_frames_ = 0;
            }
        }

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
    bool perf_ = false;
    float perf_accum_ = 0.0f;
    int perf_frames_ = 0;
};

} // namespace

// SDL2main 在 Windows / iOS / Android 上都需要看到 SDL_main；在 Linux、macOS
// 等平台 SDL_main.h 不会改写普通 main。统一包含可避免不同依赖来源（pkg-config、
// vcpkg、FetchContent）对入口符号的差异。
#include <SDL_main.h>

int main(int, char**) {
    DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
