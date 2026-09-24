// Widget：所有组件的父类。
//
// 这一层把「坐标 / 位置 / 尺寸 / 圆角 / 边框 / 阴影 / 布局 / 交互」全部做完，
// 子类（Box / Label / Button / Image）只负责三件事：
//   1. MeasureContent()  —— 内容需要多大
//   2. OnDrawContent()   —— 内容画什么
//   3. OnUpdate()        —— 每帧状态（可选）
//
// 设计约定：
//   * position/anchor/pivot/offset 都是「相对父节点内容区」的，不是绝对屏幕坐标；
//     绝对矩形每帧由 LayoutTree() 算好后放在 rect 里。
//   * size 是**外框尺寸**（含 padding/border），0 表示按内容自适应。
//   * 不借用 ImGui 的 item 机制（不调 InvisibleButton）：命中测试自己做，
//     这样才能按 z_order / 子节点优先的顺序决定谁被点中。
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "component_view/Theme.h"
#include "component_view/Types.h"

namespace gui_dev::cv {

class Widget {
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
    bool clip_children = false; // 子节点超出自身矩形时裁剪
    bool interactive = true;    // 是否参与鼠标命中

    // ---- 坐标 / 位置（相对父内容区） ---------------------------------------
    ImVec2 position{0.0f, 0.0f}; // 相对父内容区左上角的偏移
    ImVec2 anchor{0.0f, 0.0f};   // 父内容区剩余空间的分配比例：0=靠 position，0.5=居中，1=贴右/下
    ImVec2 pivot{0.0f, 0.0f};    // 自身归一化枢轴：0=左上角对齐，0.5=自身中心对齐，1=右下角对齐
    ImVec2 offset{0.0f, 0.0f};   // 最后叠加的像素偏移（动画用，不参与布局计算）
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

    // ---- 边框 / 阴影 -------------------------------------------------------
    BorderStyle border;
    ShadowStyle shadow;

    // ---- 子节点布局 --------------------------------------------------------
    LayoutMode layout = LayoutMode::Free;
    ImVec2 gap{0.0f, 0.0f};
    Align align_x = Align::Start;
    Align align_y = Align::Start;

    // ---- 交互状态（每帧刷新，只读） ----------------------------------------
    bool focusable = false;
    bool focus_on_hover = false;
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;
    bool focused = false;

    // ---- 事件 --------------------------------------------------------------
    std::function<void(Widget&)> on_click;
    std::function<void(Widget&)> on_press;
    std::function<void(Widget&)> on_hover_enter;
    std::function<void(Widget&)> on_hover_leave;
    std::function<void(Widget&)> on_focus;
    std::function<void(Widget&)> on_blur;

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

    // ---- 几何结果（LayoutTree 之后有效） -----------------------------------
    ImVec2 measured_size{0.0f, 0.0f};
    Rect rect;         // 屏幕矩形（外框）
    Rect content_rect; // 去掉 padding/border 的内容区
    Rect margin_rect;  // 含 margin 的外框

    bool Contains(const ImVec2& p) const { return rect.Contains(p); }
    Rect ContentRect() const { return content_rect; }

    // 已解析继承关系的四角圆角
    float CornerTL() const { return corner_tl >= 0.0f ? corner_tl : corner_radius; }
    float CornerTR() const { return corner_tr >= 0.0f ? corner_tr : corner_radius; }
    float CornerBL() const { return corner_bl >= 0.0f ? corner_bl : corner_radius; }
    float CornerBR() const { return corner_br >= 0.0f ? corner_br : corner_radius; }

    // ---- 每帧流程 ----------------------------------------------------------
    // 测量 + 定位整棵子树。parent_content_pos/size 是父节点内容区的屏幕矩形。
    void LayoutTree(const ImVec2& parent_content_pos, const ImVec2& parent_content_size);
    void UpdateTree(float dt);
    void DrawTree(ImDrawList* dl);
    void CollectFocusables(std::vector<Widget*>& out);
    // 鼠标命中：子节点优先。
    Widget* HitTest(const ImVec2& p);
    // 平移整棵子树（对齐修正用）
    void Move(const ImVec2& delta);

    // ---- 焦点 --------------------------------------------------------------
    void RequestFocus();
    void YieldFocus();

    // ---- 链式设置（子类继续返回自身类型） ----------------------------------
    Widget& SetName(std::string value) {
        name = std::move(value);
        return *this;
    }
    Widget& SetPosition(float x, float y) {
        position = ImVec2(x, y);
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
    Widget& SetPadding(const EdgeInsets& value) {
        padding = value;
        return *this;
    }
    Widget& SetRadius(float value) {
        corner_radius = value;
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
        return *this;
    }
    Widget& SetBorder(float width, ImU32 color) {
        border.width = width;
        border.color = color;
        return *this;
    }
    Widget& SetShadow(const ShadowStyle& value) {
        shadow = value;
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
    Widget& SetFocusable(bool value) {
        focusable = value;
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
    Widget& SetOnClick(std::function<void(Widget&)> callback) {
        on_click = std::move(callback);
        return *this;
    }

protected:
    // ---- 子类接口 ----------------------------------------------------------
    // 内容自身需要的尺寸（不含 padding/border/margin）。默认 0。
    virtual ImVec2 MeasureContent(const ImVec2& available);
    // 内容绘制（背景/边框之后，子节点之前）
    virtual void OnDrawContent(ImDrawList* dl, const Rect& content);
    // 叠加绘制（子节点之后：焦点环、选中高亮、角标……）
    virtual void OnDrawOverlay(ImDrawList* dl, const Rect& content);
    virtual void OnUpdate(float dt);
    // 手柄 Confirm / 鼠标点击统一走这里，子类可重写以扩展行为
    virtual void Activate();

    // 子类可用：自身填充色（已乘 opacity）
    ImU32 Tint(ImU32 color) const { return Theme::Alpha(color, opacity); }

private:
    void Measure(const ImVec2& available);
    void Place(const ImVec2& parent_content_pos, const ImVec2& parent_content_size);
    void PlaceChildren();
    ImVec2 FlowChildrenSize(const ImVec2& content_available);
    void DrawBackground(ImDrawList* dl);
    void UpdateInteraction();
    void DrawChildren(ImDrawList* dl);
};

} // namespace gui_dev::cv
