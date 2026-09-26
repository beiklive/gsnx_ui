// FunctionBar：功能按钮行（胶囊容器 + 圆形无边框按钮 + 聚焦项名称显示在容器下方）。
//
// 学习来源：GBAStation `src/ui/view/SwitchLayout.cpp` 的「功能按钮行」（_drawFunctions）
//   * 一条横向的胶囊条（左右两端是半圆、上下是直线），里面等距排 N 个功能项：大图标 + 名字；
//   * 焦点在项之间左右移动（首尾相接），当前项套一圈渐变流光；
//   * 按 A 先播一次「按下回弹」再执行动作（GBAStation 里延迟 0.38s 跳页）。
//
// 与 GBAStation 的差异（落进本组件库的约定）：
//   * 容器 = 普通 Box（圆角 = 高度的一半 → 胶囊；边框/阴影取 Global::component_style，底色 Theme::kBgWidget）；
//   * 按钮 = 现成的 IconButton（圆形形态）+ 去掉边框与底色 → 「无边框圆形按钮」，聚焦时是 Button 那套流光框；
//   * 名字**不再是按钮的说明行**：只在某个按钮拿到焦点时显示，画在胶囊容器下方居中（淡入淡出，不推动布局）；
//   * 焦点 → A → 触发、触摸点击、禁用置灰全部复用 Button，本组件只负责「排布 + 汇总 activated(index)」；
//   * 不做 GBAStation 的 0.38s 点击延迟动画：库里 A = 立即触发的语义要保持一致。
//
// 典型用法：
//   FunctionBar* bar = panel.Emplace<FunctionBar>();
//   bar->AddItem(Icons::Glyph(Icons::Material::Games), "游戏库", [this] { OpenLibrary(); });
//   connect(bar, &FunctionBar::activated, this, [](int i) { ... });
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/components/Box.h"

namespace gui_dev::cv {

class IconButton;

class FunctionBar : public Box {
public:
    struct Item {
        std::string icon;  // Material 字形
        std::string label; // 名字（只在聚焦时显示在胶囊下方）
        std::function<void()> on_activate;
    };

    struct Style {
        float item_size = 44.0f;       // 圆形按钮直径
        float capsule_padding = 10.0f; // 胶囊上下内边距（左右用 edge_padding）
        float edge_padding = 18.0f;    // 胶囊左右内边距（半圆两端留白）
        float gap = 22.0f;             // 相邻按钮最小间距（容器更宽时自动摊开）
        float label_gap = 6.0f;        // 名称与胶囊底边的间距
        float label_size = Theme::kFontSmall;
        float label_height = 24.0f;    // 名称行高度：常驻占位，聚焦时不会把布局顶动
        float label_fade = 12.0f;      // 名称淡入淡出速度（1/s）
        float radius = -1.0f;          // <0 = 胶囊（高度的一半）
    };

    FunctionBar();

    FunctionBar& SetItems(std::vector<Item> items);
    FunctionBar& AddItem(std::string icon, std::string label, std::function<void()> on_activate = {});
    FunctionBar& SetStyle(const Style& value);
    IconButton* itemAt(int index) const;
    int count() const { return static_cast<int>(items_.size()); }
    // 当前持有焦点的项下标（没有则 -1）
    int focusedIndex() const;
    // 胶囊自然宽度（不含外框；宿主想知道它占多宽时用）
    float CapsuleWidth() const;

signals:
    Signal<int> activated; // 某项被触发（A / 点击 / 触摸），带下标

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override; // 胶囊容器画在子按钮之下
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override; // 聚焦项名称画在子按钮之上
    void OnUpdate(float dt) override;
    void OnThemeChanged() override;

private:
    void Rebuild();
    float CapsuleHeight() const;
    float NaturalCapsuleWidth() const;

    std::vector<Item> items_;
    std::vector<IconButton*> buttons_;
    Style style;
    Rect capsule_rect_;        // 胶囊容器矩形（内容区局部坐标，MeasureContent 里算好）
    float label_alpha_ = 0.0f; // 名称淡入淡出
    int label_index_ = -1;     // 正在显示（或正在淡出）的名字是第几项
};

} // namespace gui_dev::cv
