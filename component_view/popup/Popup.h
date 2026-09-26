// Popup：统一的弹窗 / 模态框基类。
//
// 设计原则（对应需求「弹窗 = 主体内容 Content + 可选 Buttons + Popup 配置」）：
//   * **只有一套结构**：遮罩 + 窗口 Box + 类型条 + 标题 + 内容容器 + 按钮组。
//     Info / Confirm / Selection / Progress / 自定义页 都是同一套结构 + 不同的配置与内容，
//     不是几套独立渲染系统。
//   * **内容是可组合控件**：内容容器就是一个普通的 Box，里面塞 Label / RichText / Image /
//     ProgressBar / Checkbox / RadioGroup / Button / 可滚动容器…… 与页面里用的是同一批组件。
//   * **层级由 UILayer 决定**：PopupManager 按栈序给每个弹窗分配 5000~9000 的层级，
//     焦点框（9900）与 Toast（9999）永远在弹窗之上；见 component_view/UILayer.h。
//   * **生命周期是状态机**：Opening → Visible → Closing → Closed，关闭动画播完才由
//     PopupManager 真正移除，不能用 `if (open) draw()` 代替。
//
// 典型用法：
//   Popup* p = Popups().ShowConfirm("删除游戏", "删除后存档也会一起清掉，确定吗？", [this] { DeleteGame(); });
//   p->setDefaultFocus(0);   // 默认焦点放在「取消」上，避免误操作
//
// 进度弹窗（异步任务）：
//   Popup* p = Popups().ShowProgress("正在扫描 ROM", "已找到 0 个游戏");
//   ...
//   p->setProgress(0.42f)->setMessage("已找到 128 个游戏");   // UI 线程里调
//   p->Close();                                              // 任务结束
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Object.h"
#include "component_view/Markdown.h"
#include "component_view/UILayer.h"
#include "component_view/Widget.h"
#include "component_view/components/Content.h"

namespace gui_dev::cv {

class Box;
class Button;
class Label;
class ProgressBar;
class PopupManager;

// 弹窗语义类型：只决定「类型条颜色 + 默认按钮文案」，不决定结构。
enum class PopupKind {
    Custom,    // 自定义弹窗（灰）
    Info,      // 信息（蓝）
    Success,   // 成功（青绿）
    Warning,   // 警告（黄）
    Error,     // 错误（红）
    Confirm,   // 确认（蓝）
    Selection, // 选择（紫）
    Progress,  // 进度（蓝）
};

// 生命周期状态
enum class PopupState {
    Opening, // 打开动画播放中
    Visible, // 完全展开、等待交互
    Closing, // 关闭动画播放中（不再接受输入）
    Closed,  // 已结束，等 PopupManager 回收
};

// 弹窗作用域：切页时决定弹窗是关闭还是保留
enum class PopupScope {
    Page,   // 跟随页面（页面切换时由 ClosePageScoped() 关掉）
    Global, // 全局常驻（页面切换不影响；宿主自己的 manager 生命周期要够长）
};

enum class PopupButtonLayout {
    Horizontal, // 横排（默认：确认/取消）
    Vertical,   // 竖排（选项列表）
};

// 弹窗的尺寸与节奏。
//
// 视觉参数（圆角 / 边框 / 阴影 / 内边距 / 类型条高度 / 遮罩透明度）**不在控件里写死**：
// 构造时从 Global::component_style 同步（Popup::SyncVisualStyleFromGlobal），
// 想整体调就改那一个地方；单个弹窗也可以用 setStyle() 显式覆盖。
struct PopupStyle {
    // 尺寸（0 = 自适应；0<v<=1 = 画布比例；>1 = 像素）
    float width = 0.0f;
    float height = 0.0f;
    float max_width_ratio = 0.74f;
    float max_height_ratio = 0.82f;
    float min_width = 380.0f;
    float max_width = 640.0f;
    float min_height = 0.0f;
    float padding = 24.0f;      // 内边距：类型条/标题/内容/按钮都在它之内
    float gap = 16.0f;          // 各行间距

    // 外观（默认跟随 Global::component_style）
    float type_bar_height = 4.0f;    // 顶部类型条高度（在 padding 之内，左右留白相同）
    float corner_radius = 5.0f;
    float backdrop_alpha = 0.55f;
    bool backdrop = true;

    // 标题 / 正文
    float title_size = Theme::kFontHeader;
    float body_size = Theme::kFontBody;

    // 按钮：与普通 Button 完全同一套尺寸/样式，只是等宽排列
    float button_height = Theme::kControlHeight;
    float button_min_width = 132.0f;
    float button_gap = 12.0f;
    float button_row_gap = 10.0f;

    // 动画（时长与缓动属于节奏，不属于视觉风格，可以单独配）
    float open_duration = 0.20f;
    float close_duration = 0.14f;
    float open_scale_from = 0.96f;
    float open_translate_y = 16.0f;
    bool animated = true;

    // 内容
    float rich_text_height = 240.0f; // 可滚动富文本视图的高度（0 = 按内容自适应）
    float image_max_height = 300.0f;
};

// 语义色（都取自 Theme，不在弹窗里写死色值）
ImVec4 PopupAccentColor(PopupKind kind);
ImU32 PopupAccentU32(PopupKind kind);

class Popup : public Object {
public:
    // 一个按钮的完整描述（文本 / 图标 / 回调 / 强调色）
    struct ButtonSpec {
        std::string text;
        std::string icon;
        std::function<void()> on_click;
        ImVec4 color{0.0f, 0.0f, 0.0f, 0.0f}; // alpha = 0 -> 默认按钮配色
        bool primary = false;                 // 主按钮（用语义色描边/文字）
        bool close_on_click = true;           // 点完是否自动关闭（未设自动关闭时忽略）
    };

    explicit Popup(std::string name = "popup", PopupKind kind = PopupKind::Custom);
    ~Popup() override;

    Popup(const Popup&) = delete;
    Popup& operator=(const Popup&) = delete;

    // ---- 基本信息 ----------------------------------------------------------
    const std::string& name() const { return name_; }
    Popup& setName(std::string value) {
        name_ = std::move(value);
        return *this;
    }
    PopupKind kind() const { return kind_; }
    PopupScope scope() const { return scope_; }
    PopupState state() const { return state_; }
    bool IsOpening() const { return state_ == PopupState::Opening; }
    // IsOpen() = 已展开或正在展开（都算「开着」）
    bool IsOpen() const { return state_ == PopupState::Opening || state_ == PopupState::Visible; }
    bool IsClosing() const { return state_ == PopupState::Closing; }
    bool IsClosed() const { return state_ == PopupState::Closed; }
    bool IsModal() const { return modal_; }
    float openProgress() const { return progress_; }
    int layerZ() const { return layer_z_; }

    // ---- 配置（全部返回 *this，可链式） -----------------------------------
    Popup& setKind(PopupKind value);
    Popup& setTitle(std::string value);
    Popup& setModal(bool value);
    Popup& setBackdrop(bool enabled, float alpha = -1.0f);
    Popup& setDismissOnBackdrop(bool enabled);
    Popup& setDismissOnCancel(bool enabled);
    Popup& setAutoCloseOnButton(bool enabled);
    Popup& setScope(PopupScope value);
    Popup& setSize(float width, float height); // 0=自适应；(0,1]=比例；>1=像素
    Popup& setMinSize(float width, float height);
    Popup& setMaxSize(float width, float height);
    Popup& setButtonLayout(PopupButtonLayout value);
    Popup& setDefaultFocus(int index); // 默认焦点按钮下标（Confirm 默认 0 = 取消）
    Popup& setAnimated(bool value);
    Popup& setStyle(const PopupStyle& value);
    PopupStyle& style() { return style_; }
    const PopupStyle& style() const { return style_; }

    // ---- 内容 --------------------------------------------------------------
    // 单行 / 多行文本（MultiLineText：自动换行）
    Popup& setText(std::string text);
    // 可滚动富文本（Log / 错误详情 / 更新说明 / License）
    Popup& setRichText(std::vector<RichText::Run> runs, float view_height = 0.0f);
    // Markdown（标题/粗体/列表/链接/代码块/图片）：内部就是 Markdown::Parse + setRichText
    Popup& setMarkdown(std::string markdown, Markdown::ImageLookup lookup = {}, float view_height = 0.0f);
    // 图片（等比缩放 / 居中 / 最大尺寸）
    Popup& setImage(ImTextureRef texture, float width, float height);
    // 进度内容（标题下方一行说明 + 进度条）
    Popup& setProgressContent(std::string message, bool indeterminate = false);
    // 自定义页面：builder 拿到内容容器（Box），随便塞现有控件
    Popup& setContentBuilder(std::function<void(Widget& content)> builder);
    // 内容容器（setContentBuilder 里那个；也可以直接往上加控件）
    Box& content();

    // ---- 进度（异步任务用；UI 线程调用） -----------------------------------
    Popup& setProgress(float value);        // 0..1
    Popup& setIndeterminate(bool enabled);
    Popup& setMessage(std::string message);
    Popup& fail(std::string message);       // 变成错误态 + 不自动关闭 + 加一个关闭按钮

    // ---- 按钮 --------------------------------------------------------------
    Popup& addButton(ButtonSpec spec);
    Popup& addButton(std::string text, std::function<void()> on_click = {});
    int buttonCount() const { return static_cast<int>(buttons_.size()); }
    Button* buttonAt(int index) const;

    // ---- 生命周期 ----------------------------------------------------------
    void Open();      // Opening（无动画时直接 Visible）
    void Close();     // Closing（无动画时直接 Closed）
    void CloseNow();  // 立即 Closed

    // 从 Global::component_style 同步视觉参数（构造时 + 切主题时；单独改过 setStyle 的会被覆盖）
    void SyncVisualStyleFromGlobal();

    // ---- 由 PopupManager 驱动（宿主不要直接调） ---------------------------
    void SetLayerZ(int z) { layer_z_ = z; }
    void Layout();
    void UpdateTree(float dt);
    void Update(float dt);
    void Draw(ImDrawList* dl);
    Widget* HitTest(const ImVec2& point);
    bool EnsureVisible(Widget* target);
    void RefreshTheme();
    Widget* DefaultFocusTarget() const; // defaultFocus 指定的按钮，没有就给第一个可聚焦控件
    bool WantsDismiss() const { return dismiss_requested_; }
    void ClearDismissRequest() { dismiss_requested_ = false; }
    bool dismissOnCancel() const { return dismiss_on_cancel_; }
    // 请求关闭（等价于 Close，但会先发 dismissRequested；B 键 / 点遮罩走这里）
    void RequestDismiss();

    Widget& Root();
    Widget* window() const;
    Widget* contentWidget() const;

signals:
    Signal<> opening;
    Signal<> opened;
    Signal<> closing;
    Signal<> closed;
    Signal<int> buttonClicked;   // 按钮下标
    Signal<> dismissRequested;   // 用户请求关闭（B 键 / 点遮罩）

private:
    void BuildChrome();
    void RebuildContent();
    void ApplyKindColors();
    float ResolveWidth(float canvas_width) const;
    float ResolveHeightLimit(float canvas_height) const;
    void PositionChildren();

    std::string name_;
    PopupKind kind_ = PopupKind::Custom;
    PopupScope scope_ = PopupScope::Page;
    PopupState state_ = PopupState::Closed;
    PopupStyle style_;

    bool modal_ = true;
    bool dismiss_on_backdrop_ = false;
    bool dismiss_on_cancel_ = true;
    bool auto_close_on_button_ = true;
    int default_focus_ = -1;
    PopupButtonLayout button_layout_ = PopupButtonLayout::Horizontal;
    int layer_z_ = PopupLayerZ(0);

    float progress_ = 0.0f;      // 出场/退场动画进度（0..1）
    float width_ = 0.0f;         // 上一帧算出来的窗口尺寸（滚动判定要用）
    bool scrollable_ = false;

    std::unique_ptr<Box> root_owner_; // 弹窗自己那棵树的根（覆盖画布、透明）
    Box* root_ = nullptr;
    Box* backdrop_ = nullptr;        // 遮罩（z_order = -1：画在最下、命中测试最后）
    Box* window_ = nullptr;          // 弹窗窗口
    Box* type_bar_ = nullptr;        // 顶部类型条
    Label* title_ = nullptr;
    Box* content_host_ = nullptr;    // 内容容器（需要滚动时它自己就是 ScrollView）
    Box* buttons_box_ = nullptr;     // 按钮组容器
    Label* message_ = nullptr;       // 进度/说明文字
    ProgressBar* progress_bar_ = nullptr;
    Image* image_ = nullptr;
    RichText* rich_text_ = nullptr;
    Box* rich_scroll_ = nullptr;     // 富文本的滚动容器（Box + Overflow::Scroll）
    std::function<void(Widget&)> content_builder_;

    struct ButtonEntry {
        ButtonSpec spec;
        Button* button = nullptr;
    };
    std::vector<ButtonEntry> buttons_;
    bool dismiss_requested_ = false;
};

} // namespace gui_dev::cv
