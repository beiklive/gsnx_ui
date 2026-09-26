// FunctionBar：功能按钮行（横排「图标 + 说明」的动作条）。
//
// 学习来源：GBAStation `src/ui/view/SwitchLayout.cpp` 的「功能按钮行」（_drawFunctions）
//   * 一条横向的胶囊条，里面等距排 N 个功能项：上面是大图标、下面是名字；
//   * 焦点在项之间左右移动（首尾相接），当前项放大 + 一圈渐变流光；
//   * 按 A 先播一次「按下回弹」再执行动作（延迟 0.38s 才真正跳页，避免动画被切掉）；
//   * 六个功能项固定：游戏库 / 文件列表 / 数据管理 / 设置 / 关于 / 退出。
//
// 与 GBAStation 的差异（落进本组件库的约定）：
//   * 不自己画：条 = 普通 Box 面板（圆角/边框/阴影取 Global::component_style），
//     项 = 现成的 Button（图标在上、说明行在下，`showSubtitle(true)`），
//     所以「焦点 → A → 触发」、触摸点击、流光焦点框、禁用置灰全都是 Button 那一套，
//     本组件只负责「等分宽度 + 把 clicked 汇总成一个带下标的信号」。
//   * 断点是「按当前 UI 风格」做的：面板底色 Theme::kBgPanel，项用 Button 的默认配色与流光框。
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

class Button;

class FunctionBar : public Box {
public:
    struct Item {
        std::string icon;  // Material 字形
        std::string label; // 名字（画在图标下方，走 Button 的说明行）
        std::function<void()> on_activate;
    };

    struct Style {
        float item_height = 84.0f; // 单项高（图标 + 名字）
        float item_min_width = 96.0f;
        float gap = 10.0f;         // 项间距
        float padding = -1.0f;     // <0 = 用 Global::component_style.content_padding
        float radius = -1.0f;      // <0 = 用 Global::component_style.corner_radius
        float icon_size = 30.0f;   // 图标方形格边长
    };

    FunctionBar();

    FunctionBar& SetItems(std::vector<Item> items);
    FunctionBar& AddItem(std::string icon, std::string label, std::function<void()> on_activate = {});
    FunctionBar& SetStyle(const Style& value);
    Button* itemAt(int index) const;
    int count() const { return static_cast<int>(items_.size()); }
    // 当前持有焦点的项下标（没有则 -1）
    int focusedIndex() const;

signals:
    Signal<int> activated; // 某项被触发（A / 点击 / 触摸），带下标

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnThemeChanged() override;

private:
    void Rebuild();
    std::vector<Item> items_;
    std::vector<Button*> buttons_;
    Style style;
};

} // namespace gui_dev::cv
