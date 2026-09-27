// Widget：所有组件的父类。
//
// 这一层把「坐标 / 位置 / 尺寸 / 圆角 / 边框 / 阴影 / 布局 / 溢出滚动 / 焦点动画 /
// 命中测试 / 事件」全部做完，子类（Box / Label / Button / List ...）只负责三件事：
//   1. MeasureContent()  —— 内容需要多大
//   2. OnDrawContent()   —— 内容画什么
//   3. OnUpdate()        —— 每帧状态（可选）
//
// 设计约定：
//   * position/anchor/pivot/offset 都是「相对父节点内容区」的，不是绝对屏幕坐标；
//     绝对矩形每帧由 LayoutTree() 算好后放在 rect 里。
//   * size 是**外框尺寸**（含 padding/border），0 表示按内容自适应。
//   * 不借用 ImGui 的 item 机制（不调 InvisibleButton）：命中测试自己做，
//     这样才能按 z_order / 子节点优先的顺序决定谁被点中，也让手柄焦点完全可控。
//   * Focus 是一等状态：focused / 焦点缩放 / 焦点位移 / 焦点框都由基类统一处理，
//     hover 只是「桌面平台的额外输入」。
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "component_view/Object.h"
#include "component_view/Theme.h"
#include "component_view/Types.h"
#include "platform/Input.h"

namespace gui_dev::cv {

using Theme::WidgetState; // 状态枚举与状态色都定义在 Theme（主题是唯一来源）

// 焦点框的「描述」：控件只说自己要什么样的焦点框（矩形 / 圆角 / 粗细 / 强度 …），
// 真正画在哪儿由页面级的 FocusRing 图层决定（见 component_view/FocusRing.h）。
// 这样焦点框与控件解耦，一帧只画一个，也不用每个控件都在自己的 OnDrawOverlay 里画。
struct FocusVisual {
    bool enabled = false;   // 不需要画（没焦点 / 组件不做焦点框）
    bool flowing = false;   // true = 流光闭合框，false = 单色圆角框
    Rect rect{};            // 已经是绘制坐标（外扩后的）
    float radius = 0.0f;
    float width = 2.0f;
    float alpha = 0.0f;     // 0..1（焦点强度 × 控件不透明度）
    ImU32 color = 0;        // 单色框颜色
    float phase = 0.0f;     // 流光相位
    float saturation = 0.75f;
    float brightness = 1.0f;
    bool pause_style = false;    // pause_menu 风格：角标 + 强调色外框
};

class Widget : public Object {
public:
    Widget();
    explicit Widget(std::string widget_name);
    virtual ~Widget() = default;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // ---- 标识 --------------------------------------------------------------
    std::string name;
    WidgetId id = 0;
    int z_order = 0;            // 越大越靠上（只影响绘制与命中顺序）
    bool visible = true;
    bool enabled = true;
    float opacity = 1.0f;       // 0..1，作用于自身填充/边框/文字
    bool interactive = true;    // 是否参与鼠标命中

    // ---- 坐标 / 位置（相对父内容区） ---------------------------------------
    ImVec2 position{0.0f, 0.0f}; // 相对父内容区左上角的偏移
    ImVec2 anchor{0.0f, 0.0f};   // 父内容区剩余空间的分配比例：0=靠 position，0.5=居中，1=贴右/下
    ImVec2 pivot{0.0f, 0.0f};    // 自身归一化枢轴：0=左上角对齐，0.5=自身中心对齐，1=右下角对齐
    ImVec2 offset{0.0f, 0.0f};   // 最后叠加的像素偏移（一般用 visual_translate 做动画）
    EdgeInsets margin;
    EdgeInsets padding;

    // ---- 尺寸 --------------------------------------------------------------
    ImVec2 size{0.0f, 0.0f};     // 0 = 该轴按内容自适应
    ImVec2 min_size{0.0f, 0.0f};
    ImVec2 max_size{0.0f, 0.0f}; // 0 = 不限制
    float aspect_ratio = 0.0f;   // >0 时约束宽高比

    // ---- 圆角 / 背景 -------------------------------------------------------
    float corner_radius = 0.0f; // 四角统一值
    // 单角覆盖：-1 表示跟随 corner_radius
    float corner_tl = -1.0f;
    float corner_tr = -1.0f;
    float corner_bl = -1.0f;
    float corner_br = -1.0f;
    ImU32 background = 0;
    // 底色是否来自调色板（组件默认）→ 切主题时刷新；用户显式设过色就置 false
    bool background_follows_theme = false;
    // 底色是否跟着「状态」从主题取（Theme::ControlBackgroundColor(State())）：
    // 交互控件（Button 家族）打开；纯容器保持 false，视觉与以前完全一致。
    bool background_state_follows_theme = false;

    // ---- 边框 / 阴影 -------------------------------------------------------
    BorderStyle border;
    // 边框颜色是否跟着状态走（默认 false = 用显式色；按钮类打开，Normal 状态 = 主题默认边框色）
    bool border_state_follows_theme = false;
    ShadowStyle shadow;

    // ---- 溢出与滚动 --------------------------------------------------------
    Overflow overflow = Overflow::Visible;
    ImVec2 scroll{0.0f, 0.0f};         // 当前滚动量（平滑后）
    ImVec2 scroll_target{0.0f, 0.0f};  // 目标滚动量（EnsureVisible / 翻页设置）
    ImVec2 scroll_max{0.0f, 0.0f};     // 可滚动上限（布局时算出）
    ImVec2 content_extent{0.0f, 0.0f}; // 子节点占用的总尺寸
    bool scroll_enabled = true;
    float scroll_smoothing = 14.0f;    // 指数平滑速度
    bool scroll_overscroll = false;    // 越界回弹（超出后弹回，不硬夹）
    bool scroll_snap = false;          // 滚动目标吸附到页宽整数倍
    // ---- 拖动惯性（触摸/鼠标拖动松手后继续滑一段；两个轴都生效）-----------------
    // 拖动中由 Widget::UpdateScroll 记录手指速度，松手后按速度继续推进 scroll_target，
    // 速度按 exp(-friction*dt) 衰减；顶到边界（或没有可滚空间）就立刻停下。
    bool scroll_inertia = true;              // false = 松手即停（旧行为）
    ImVec2 scroll_velocity{0.0f, 0.0f};      // 当前速度 px/s（运行时状态，不用手设）
    float scroll_friction = 5.5f;            // 速度衰减速度（1/s，越大停得越快）
    float scroll_fling_threshold = 320.0f;   // 松手速度低于它就不进入惯性（px/s）
    float scroll_fling_max = 4200.0f;        // 速度上限（px/s，防止一甩到底）
    bool scroll_bar = true;
    bool scroll_bar_auto_hide = true;
    float scroll_bar_thickness = 5.0f;

    // ---- 子节点布局 --------------------------------------------------------
    LayoutMode layout = LayoutMode::Free;
    ImVec2 gap{0.0f, 0.0f};
    Align align_x = Align::Start;
    Align align_y = Align::Start;

    // ---- 焦点视觉（Focus 是核心状态） --------------------------------------
    bool focusable = false;
    // 临时退出焦点导航（但照常绘制/命中）：页面转场时旧页还在播退场动画、不想让它继续接焦点，
    // 或者某个区域被弹窗接管时，用这个比改 focusable/enabled 干净。
    bool focus_inert = false;
    bool focus_on_hover = false;        // 桌面平台：鼠标悬停即接管焦点
    // 焦点分区：0 = 不分区（全局导航）；不同分区的组件之间只有左右方向可以跨越。
    // 主机 UI 常用「左列标签 / 右列内容」两分区，避免上下键在分区之间乱跳。
    int focus_zone = 0;
    // 该控件是否自己消费方向键。
    // true = 全局焦点导航不抢这个方向，留给控件内部导航（列表 / 滑条 / 键盘……），
    // 离开这类控件用另一半方向键或 B 键。
    bool capture_horizontal = false;
    bool capture_vertical = false;
    // 焦点遍历时是否连子节点一起算：
    //   false（默认）= 容器语义 —— 自己可聚焦，子节点也各自是焦点停靠点
    //   true          = 复合控件语义 —— 只把自己当停靠点（列表/滑条那种内部自己导航）
    // 两者都要求 focusable = true 才生效。
    bool focus_only_self = false;
    bool focus_frame = false;           // 是否在自身外侧画焦点框
    float focus_scale = 1.0f;           // focused 时的缩放（1 = 不缩放）
    ImVec2 focus_translate{0.0f, 0.0f}; // focused 时的位移
    float focus_animation_speed = 16.0f;
    float focus_frame_width = 2.0f;
    float focus_frame_offset = 4.0f;
    ImU32 focus_frame_color = Theme::U32(Theme::kAccent);
    float disabled_opacity = 0.45f;

    // ---- 每帧由子类设置的即时视觉变换（按压缩放等） ------------------------
    ImVec2 visual_scale{1.0f, 1.0f};
    ImVec2 visual_translate{0.0f, 0.0f};

    // ---- 交互状态（每帧刷新，只读） ----------------------------------------
    bool hovered = false;
    bool down = false;    // 鼠标/按钮按下中（Qt 的 isDown()）
    bool focused = false; // 持有手柄焦点（Qt 的 hasFocus()）
    bool selected = false; // 由容器/页面设置：当前选中项
    float focus_mix = 0.0f; // 焦点动画进度 0..1（平滑）

    // ---- 信号（Qt 风格：emit clicked(); / connect(w, &Widget::clicked, ...)）----
signals:
    Signal<> clicked;      // 确认（A / 鼠标左键 / 键盘回车）
    Signal<> pressed;      // 按下开始
    Signal<> released;     // 按下结束（无论是否命中）
    Signal<> hoverEntered; // 鼠标进入（桌面端才有意义）
    Signal<> hoverLeft;
    Signal<> focusIn;      // 获得手柄焦点
    Signal<> focusOut;     // 失去手柄焦点
    Signal<> enabledChanged;

    // ---- 只读状态访问器（Qt 命名） ----------------------------------------
    bool isDown() const { return down; }
    bool hasFocus() const { return focused; }
    bool isHovered() const { return hovered; }
    bool isSelected() const { return selected; }
    bool isEnabled() const { return enabled; }
    bool isVisible() const { return visible; }

    // ---- 树 ----------------------------------------------------------------
    Widget* parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;

    Widget& Add(std::unique_ptr<Widget> child);
    template <typename T, typename... Args>
    T* Emplace(Args&&... args) {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = child.get();
        Add(std::move(child));
        return raw;
    }
    Widget* Find(const std::string& widget_name);
    template <typename T>
    T* FindAs(const std::string& widget_name) {
        return dynamic_cast<T*>(Find(widget_name));
    }
    void Remove(Widget* child);
    void Clear();
    bool ContainsDescendant(const Widget* target) const;

    // ---- 几何结果（LayoutTree 之后有效，均为布局坐标） ---------------------
    ImVec2 measured_size{0.0f, 0.0f};
    Rect rect;         // 屏幕矩形（外框）
    Rect content_rect; // 去掉 padding/border 的内容区（已含滚动偏移）
    Rect margin_rect;  // 含 margin 的外框

    bool Contains(const ImVec2& p) const { return rect.Contains(p); }
    Rect ContentRect() const { return content_rect; }

    // 已解析继承关系的四角圆角
    float CornerTL() const { return corner_tl >= 0.0f ? corner_tl : corner_radius; }
    float CornerTR() const { return corner_tr >= 0.0f ? corner_tr : corner_radius; }
    float CornerBL() const { return corner_bl >= 0.0f ? corner_bl : corner_radius; }
    float CornerBR() const { return corner_br >= 0.0f ? corner_br : corner_radius; }

    // 焦点框描述：默认按 focus_frame / focus_mix 给出单色框；用流光框的控件（Button）会重写。
    // 由页面级 FocusRing 图层读取并绘制（见 component_view/FocusRing.h）。
    virtual FocusVisual BuildFocusVisual() const;

    // ---- 绘制期变换（子类绘制时用；保证焦点缩放能作用于整棵子树） ---------
    Rect DrawRect() const { return draw_rect_; }
    Rect DrawContentRect() const { return draw_transform_.Apply(content_rect); }
    float DrawScale() const { return draw_transform_.AverageScale(); }

    // ---- 每帧流程 ----------------------------------------------------------
    // 测量 + 定位整棵子树。parent_content_pos/size 是父节点内容区的屏幕矩形。
    void LayoutTree(const ImVec2& parent_content_pos, const ImVec2& parent_content_size);
    void UpdateTree(float dt);
    void DrawTree(ImDrawList* dl);
    // 切主题后让「自己 + 所有子节点」重新取调色板里的颜色（宿主调一次即可）
    void RefreshThemeTree();
    void CollectFocusables(std::vector<Widget*>& out);
    // 鼠标命中：子节点优先 + z_order 高者优先。
    Widget* HitTest(const ImVec2& p);
    // 平移整棵子树（对齐修正用）
    void Move(const ImVec2& delta);
    // 焦点自动滚动：把 target 滚动进可见区（target 需在本子树内）
    bool EnsureVisible(Widget* target);
    // 焦点自动滚动：把任意矩形滚动进内容区（复合控件内部索引导航用）
    void EnsureRectVisible(const Rect& target_rect);
    // 滚动翻页（L/R、ZL/ZR）：direction=-1 上一页 / +1 下一页
    void ScrollPage(int direction, float scale = 1.0f);
    // 最近的滚动容器祖先（自己也算）
    Widget* ScrollHost();

    // ---- 焦点 --------------------------------------------------------------
    void RequestFocus();
    void YieldFocus();
    // Qt 风格的便捷连接：button->onClicked(this, &Page::OnConfirm);
    template <typename Context, typename Callable>
    Connection onClicked(Context* context, Callable callable) {
        return connect(this, &Widget::clicked, context, std::move(callable));
    }

    // 子树里第一个可聚焦组件（对话框/页面切换后接管焦点用）
    Widget* FirstFocusable();
    // 给整棵子树的组件设置焦点分区
    void SetFocusZone(int zone);

    // ---- 链式设置 ----------------------------------------------------------
    // ---- 状态（状态色由主题统一给：Theme::ControlBackgroundColor / ControlInkColor …）----
    // 判定顺序：Disabled > Pressed(down) > Hovered > Normal。
    // 注意两个刻意的省略：
    //   * focused 不算 Hovered —— 键盘焦点已经由流光焦点框表达，避免叠加出新底色；
    //   * selected 不参与状态色 —— 「选中」是容器的语义（TabColumn 自己画选中底），
    //     按钮/控件不因 selected 变色。
    WidgetState State() const {
        if (!enabled) {
            return WidgetState::Disabled;
        }
        if (down) {
            return WidgetState::Pressed;
        }
        if (hovered) {
            return WidgetState::Hovered;
        }
        return WidgetState::Normal;
    }
    bool isFocusable() const { return focusable; }

    Widget& SetName(std::string value) {
        name = std::move(value);
        return *this;
    }
    Widget& SetPosition(float x, float y) {
        position = ImVec2(x, y);
        return *this;
    }
    // 几何便捷：只动一根轴（不改另一轴的显式/自适应状态）
    Widget& SetX(float value) {
        position.x = value;
        return *this;
    }
    Widget& SetY(float value) {
        position.y = value;
        return *this;
    }
    Widget& SetWidth(float value) {
        size.x = value;
        return *this;
    }
    Widget& SetHeight(float value) {
        size.y = value;
        return *this;
    }
    Widget& SetAnchor(float x, float y) {
        anchor = ImVec2(x, y);
        return *this;
    }
    Widget& SetPivot(float x, float y) {
        pivot = ImVec2(x, y);
        return *this;
    }
    Widget& SetSize(float w, float h) {
        size = ImVec2(w, h);
        return *this;
    }
    Widget& SetMinSize(float w, float h) {
        min_size = ImVec2(w, h);
        return *this;
    }
    Widget& SetMaxSize(float w, float h) {
        max_size = ImVec2(w, h);
        return *this;
    }
    Widget& SetMargin(const EdgeInsets& value) {
        margin = value;
        return *this;
    }
    Widget& SetMargin(float left, float top, float right, float bottom) {
        margin = EdgeInsets{left, top, right, bottom};
        return *this;
    }
    Widget& SetMarginLeft(float value) {
        margin.left = value;
        return *this;
    }
    Widget& SetMarginTop(float value) {
        margin.top = value;
        return *this;
    }
    Widget& SetMarginRight(float value) {
        margin.right = value;
        return *this;
    }
    Widget& SetMarginBottom(float value) {
        margin.bottom = value;
        return *this;
    }
    Widget& SetPadding(const EdgeInsets& value) {
        padding = value;
        return *this;
    }
    Widget& SetPadding(float left, float top, float right, float bottom) {
        padding = EdgeInsets{left, top, right, bottom};
        return *this;
    }
    Widget& SetPaddingLeft(float value) {
        padding.left = value;
        return *this;
    }
    Widget& SetPaddingTop(float value) {
        padding.top = value;
        return *this;
    }
    Widget& SetPaddingRight(float value) {
        padding.right = value;
        return *this;
    }
    Widget& SetPaddingBottom(float value) {
        padding.bottom = value;
        return *this;
    }
    Widget& SetRadius(float value) {
        corner_radius = value;
        return *this;
    }
    // 单角圆角（<0 = 跟随统一圆角；四角统一时用 SetRadius(v) 即可）
    Widget& SetRadiusTopLeft(float value) {
        corner_tl = value;
        return *this;
    }
    Widget& SetRadiusTopRight(float value) {
        corner_tr = value;
        return *this;
    }
    Widget& SetRadiusBottomLeft(float value) {
        corner_bl = value;
        return *this;
    }
    Widget& SetRadiusBottomRight(float value) {
        corner_br = value;
        return *this;
    }
    Widget& SetRadius(float tl, float tr, float bl, float br) {
        corner_tl = tl;
        corner_tr = tr;
        corner_bl = bl;
        corner_br = br;
        corner_radius = Maxf(Maxf(Maxf(tl, tr), Maxf(bl, br)), corner_radius);
        return *this;
    }
    Widget& SetBackground(ImU32 color) {
        background = color;
        background_follows_theme = false;
        return *this;
    }
    // 直接吃 Theme 的 ImVec4（rgba()/rgb() 的结果），省掉一层 U32()
    Widget& SetBackground(const ImVec4& color) {
        background = Theme::U32(color);
        background_follows_theme = false;
        return *this;
    }
    // RGBA 四通道单独给（0..1）；a = 0 表示不填充
    Widget& SetBackgroundColor(float r, float g, float b, float a) {
        background = Theme::U32(ImVec4(r, g, b, a));
        background_follows_theme = false;
        return *this;
    }
    Widget& SetBorder(float width, ImU32 color) {
        border.width = width;
        border.color = color;
        return *this;
    }
    // 统一边框（颜色给 ImVec4，和 Theme 的用法一致；底层仍存 ImU32）
    Widget& SetBorder(float width, const ImVec4& color) {
        border.width = width;
        border.color = Theme::U32(color);
        return *this;
    }
    Widget& SetBorderWidth(float width) {
        border.width = width;
        return *this;
    }
    Widget& SetBorderColor(const ImVec4& color) {
        border.color = Theme::U32(color);
        return *this;
    }
    // 单边边框：宽度 <0 = 跟随统一宽度；alpha=0 的颜色 = 跟随统一颜色
    Widget& SetBorder(BorderSide side, float width, const ImVec4& color) {
        switch (side) {
        case BorderSide::Top:
            border.top_width = width;
            border.top_color = Theme::U32(color);
            break;
        case BorderSide::Right:
            border.right_width = width;
            border.right_color = Theme::U32(color);
            break;
        case BorderSide::Bottom:
            border.bottom_width = width;
            border.bottom_color = Theme::U32(color);
            break;
        case BorderSide::Left:
        default:
            border.left_width = width;
            border.left_color = Theme::U32(color);
            break;
        }
        return *this;
    }
    Widget& SetBorderTop(float width, const ImVec4& color) {
        return SetBorder(BorderSide::Top, width, color);
    }
    Widget& SetBorderRight(float width, const ImVec4& color) {
        return SetBorder(BorderSide::Right, width, color);
    }
    Widget& SetBorderBottom(float width, const ImVec4& color) {
        return SetBorder(BorderSide::Bottom, width, color);
    }
    Widget& SetBorderLeft(float width, const ImVec4& color) {
        return SetBorder(BorderSide::Left, width, color);
    }
    Widget& SetShadow(const ShadowStyle& value) {
        shadow = value;
        return *this;
    }
    // 阴影逐字段（offset 与 blur/spread 是两回事：offset = 位移，blur = 柔化，spread = 额外扩张）
    Widget& SetShadowEnabled(bool value) {
        shadow.enabled = value;
        return *this;
    }
    Widget& SetShadowOffset(float x, float y) {
        shadow.offset = ImVec2(x, y);
        shadow.enabled = true;
        return *this;
    }
    Widget& SetShadowBlur(float blur) {
        shadow.blur = blur;
        shadow.enabled = true;
        return *this;
    }
    Widget& SetShadowSpread(float spread) {
        shadow.spread = spread;
        shadow.enabled = true;
        return *this;
    }
    Widget& SetShadowColor(const ImVec4& color) {
        shadow.color = Theme::U32(color);
        shadow.enabled = true;
        return *this;
    }
    Widget& SetLayout(LayoutMode mode, const ImVec2& spacing = ImVec2(0.0f, 0.0f)) {
        layout = mode;
        gap = spacing;
        return *this;
    }
    Widget& SetAlign(Align x, Align y) {
        align_x = x;
        align_y = y;
        return *this;
    }
    Widget& SetAlignX(Align x) {
        align_x = x;
        return *this;
    }
    Widget& SetAlignY(Align y) {
        align_y = y;
        return *this;
    }
    Widget& SetOverflow(Overflow value) {
        overflow = value;
        return *this;
    }
    // 裁剪开关（true = 裁掉超出部分且不滚动，false = 不裁剪）
    Widget& SetClip(bool enabled) {
        overflow = enabled ? Overflow::Hidden : Overflow::Visible;
        return *this;
    }
    Widget& SetFocusable(bool value) {
        focusable = value;
        return *this;
    }
    Widget& SetFocusOnlySelf(bool value) {
        focus_only_self = value;
        return *this;
    }
    Widget& SetFocusOnHover(bool value) {
        focus_on_hover = value;
        return *this;
    }
    Widget& SetFocusFrame(bool value, float frame_offset = 3.0f) {
        focus_frame = value;
        focus_frame_offset = frame_offset;
        return *this;
    }
    Widget& SetFocusScale(float value) {
        focus_scale = value;
        return *this;
    }
    Widget& SetFocusVisual(float scale, const ImVec2& translate, bool frame) {
        focus_scale = scale;
        focus_translate = translate;
        focus_frame = frame;
        return *this;
    }
    Widget& SetVisible(bool value) {
        visible = value;
        return *this;
    }
    Widget& SetEnabled(bool value) {
        enabled = value;
        return *this;
    }
    Widget& SetZOrder(int value) {
        z_order = value;
        return *this;
    }
    Widget& SetOpacity(float value) {
        opacity = value;
        return *this;
    }

protected:
    // 套用 Global::component_style 的「框」：边框 / 圆角 / 阴影（Box / Button / Toast 共用）
    void ApplyComponentBoxStyle();

    // 按当前状态从主题取底色 / 边框色（只有对应的 *_state_follows_theme 打开时才动）
    void ApplyStateColors();

    // ---- 子类接口 ----------------------------------------------------------
    // 内容自身需要的尺寸（不含 padding/border/margin）。默认 0。
    virtual ImVec2 MeasureContent(const ImVec2& available);
    // 内容绘制（背景/边框之后，子节点之前）。content 是**已变换**的内容区矩形。
    virtual void OnDrawContent(ImDrawList* dl, const Rect& content);
    // 叠加绘制（子节点之后：角标、自定义焦点框……）
    virtual void OnDrawOverlay(ImDrawList* dl, const Rect& content);
    virtual void OnUpdate(float dt);
    // 手柄 Confirm / 鼠标点击统一走这里，子类可重写以扩展行为
    virtual void Activate();
    // 手柄按键分发（仅当自己持有焦点）。返回 true 表示已消费。
    virtual bool OnPadAction(InputAction action);
    // 布局完成后调用（算滚动上限等）
    virtual void OnAfterLayout() {}
    // 主题（浅色/深色）切换：组件在这里重新取调色板里的颜色。
    // RefreshThemeTree() 会递归调用整棵子树，宿主切完主题调一次即可。
    virtual void OnThemeChanged();

    // 子类可用：自身填充色（已乘 opacity / disabled）
    ImU32 Tint(ImU32 color) const;
    float EffectiveOpacity() const;

private:
    void Measure(const ImVec2& available);
    void Place(const ImVec2& parent_content_pos, const ImVec2& parent_content_size);
    void PlaceChildren();
    ImVec2 FlowChildrenSize(const ImVec2& content_available);
    void UpdateScroll(float dt);
    void DrawBackground(ImDrawList* dl);
    void DrawScrollBar(ImDrawList* dl);
    void UpdateInteraction(float dt);
    void emitWidgetPressed();
    bool last_enabled_ = true; // enabled 跳变检测（发 enabledChanged）
    void DrawChildren(ImDrawList* dl);
    void DrawTree(ImDrawList* dl, const Transform2D& parent_transform);

    Transform2D draw_transform_{};
    Rect draw_rect_{};
    float scroll_bar_alpha_ = 0.0f;
};

} // namespace gui_dev::cv
