// Page：页面基类。
//
// 一个页面 = 一棵组件树（root Box）+ 页级输入 + 页级 HUD。
// 你在 OnBuild() 里搭组件树，Update/Render 由宿主（demo.cpp）每帧调用。
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "component_view/Draw.h"
#include "component_view/Widget.h"
#include "ui/Icons.h"

namespace gui_dev {
class UiContext;
}

namespace gui_dev::cv {

class Box;

class Page : public Object {
public:
    Page();
    ~Page() override;

    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;

    // ---- 页面信息 ----------------------------------------------------------
    virtual const char* Title() const = 0;

    // ---- 生命周期 ----------------------------------------------------------
    // 搭组件树（只调用一次）。用 Root().Emplace<Box>(...) 加组件。
    virtual void OnBuild() {}
    // 每帧：布局与输入处理之后调用（动画、状态机）。
    virtual void OnUpdate(float dt) { (void)dt; }
    // 页面自定义按键（在焦点导航之前）
    virtual void OnInput() {}
    // 组件树上层的自定义绘制（例如拖拽指示线）
    virtual void OnOverlay(ImDrawList* dl) { (void)dl; }

    // ---- 宿主调用 ----------------------------------------------------------
    // 绑定 UiContext（宿主在第一次 Update 之前调用一次）。
    void Bind(UiContext& ui) { ui_ = &ui; }
    void Update(float dt);
    void Render();

    Box& Root();
    UiContext& ui() const { return *ui_; }

    // ---- HUD ---------------------------------------------------------------
    void AddHint(Icons::Button button, std::string label);
    void ClearHints() { hints_.clear(); }
    // 弹层容器（Dialog / 虚拟键盘）：画在 HUD 之上，默认隐藏
    Box& Overlay();
    void SetPageInfo(int index, int total);
    void SetShowHud(bool value) { show_hud_ = value; }

private:
    void DrawHud(ImDrawList* dl);

    std::unique_ptr<Box> root_;
    std::unique_ptr<Box> overlay_;
    UiContext* ui_ = nullptr;
    std::vector<Widget*> focusables_;
    std::vector<std::pair<Icons::Button, std::string>> hints_;
    int page_index_ = 0;
    int page_total_ = 0;
    bool built_ = false;
    bool show_hud_ = true;
};

} // namespace gui_dev::cv
