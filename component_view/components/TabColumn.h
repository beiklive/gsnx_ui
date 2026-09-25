// TabColumn：左侧纵向标签列（当 Tab 用）。
//
// 定位：单选导航列 —— 只负责「列」本身（项的排布、选中态、上下遍历、点选、溢出滚动），
// 选中之后切什么内容由页面接 selectionChanged 自己处理，组件不持有内容。
//
// 做法是**组合式**：容器 + 复用现成的 TextButton 当 item。
// 库里 Button 已经把「左侧图标正方形格 + 文字紧跟其右」、流光焦点框、命中测试、主题跟随
// 都做完了，所以这里只补四件事：
//   1. 单选状态（index）与信号
//   2. 选中视觉：选中项背后一块圆角底（Theme::kSelection）+ 左侧强调色条；
//      底色不做位移动画，切到哪一项就直接落在哪一项上（需求：背景不要移动动画）
//   3. 焦点分区：整列（含 item）一个 focus_zone，配合页面把内容区设成另一个 zone，
//      ↑↓ 就在列内、→ 才跨到内容区（Global::NavigateFocus 的规则）
//   4. 焦点自动滚动：全库没人调 EnsureVisible，这里补上（焦点项变化就滚进可见区）
//
// 动效（参照 examples/pause_menu 的菜单出场，见 component_view/Anim.h）：
//   * 入场：整列 0.28s 推进，逐项错开 25ms，从左侧滑入 + 淡入（EaseOutCubic）
//   * 焦点响应：固定 0.16s 推进，几何量套 EaseOutBack 产生轻微右移 + 文字提亮
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Button;

class TabColumn : public Widget {
public:
    struct Item {
        std::string icon; // Material 字形，可为空
        std::string text;
    };

    struct Style {
        float item_height = Theme::kControlHeight; // 每项高度（56）
        float item_gap = 4.0f;                     // 项间距
        float item_radius = 5.0f;                  // 选中底圆角（<=0 = 胶囊）
        float content_padding = 16.0f;             // 项内左边距（文字离左边缘；左侧色条就在这里）
        float padding_y = 12.0f;                   // 项内上下留白（图标格 = 高 - 2*它）
        float indicator_width = 4.0f;              // 左侧强调色条宽度，0 = 不画
        float indicator_inset = 10.0f;             // 色条离项左边缘
        float indicator_margin_y = 12.0f;          // 色条上下留白
        int focus_zone = 1;                        // 整列的焦点分区（内容区建议设成另一个值）

        // ---- 动效 ----
        float enter_duration = 0.28f;   // 整列入场时长（秒）
        float enter_stagger = 0.025f;   // 逐项错开（秒/项）
        float enter_offset = -30.0f;    // 入场时从左侧滑入的偏移（px）
        float focus_duration = 0.16f;   // 焦点切换响应时长（秒，参考值 120~220ms）
        float focus_offset = 4.0f;      // 焦点项右移量（px，套 EaseOutBack）
    };

    TabColumn();

    TabColumn& setItems(std::vector<Item> items);
    TabColumn& setIndex(int value, bool notify = true);
    int index() const { return index_; }
    int count() const { return static_cast<int>(items_.size()); }
    Button* itemAt(int index) const;
    const std::vector<Item>& items() const { return items_; }

    // → / R：把焦点交给内容区的入口控件（页面接一根线就行）
    TabColumn& setFocusTarget(Widget* target);
    Widget* focusTarget() const { return focus_target_; }

    // 焦点进子页面第一个可聚焦元素（按 A / 点确认时用）；没有目标返回 false
    bool EnterContent();
    // 焦点回到本列当前选中项（子页面里按 B 时用）
    void FocusCurrentItem();

    // 重播一次入场（页面出现 / 回到本页时调用）
    void PlayEnter();

    Style style;

signals:
    Signal<int> selectionChanged; // 选中项变了（带新下标）
    Signal<int> activated;        // 对已选中项再按 A / 再点一次

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    void SelectAt(int index); // item 被点击：不一样就切，一样就发 activated
    void ApplyItemLook(int index);
    int ClampIndex(int value) const;
    // 整列入场需要播多久（含逐项错开的尾巴）
    float TotalEnterTime() const;
    // 第 index 项当前的入场进度（已套 EaseOutCubic）
    float ItemEnter(int index) const;
    // 第 index 项当前的焦点动画量（已套 EaseOutBack）
    float ItemFocus(int index) const;
    // 把子项的布局矩形映射到本节点当前的绘制坐标（自身的焦点缩放/位移也算进去）
    Rect MapFromSelf(const Rect& r) const;

    std::vector<Item> items_;
    std::vector<Button*> item_buttons_;
    std::vector<float> focus_anim_; // 每项的焦点动画进度 0..1
    Widget* focus_target_ = nullptr;
    Widget* last_focus_ = nullptr;
    int index_ = 0;
    float enter_time_ = 1.0e9f; // 入场已经播了多少秒（>= TotalEnterTime() 表示播完）
};

} // namespace gui_dev::cv
