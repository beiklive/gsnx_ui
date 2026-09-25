// Page：页面基类（Demo 的宿主）。
//
// 一个页面 = 一棵组件树（root Box）+ 页级输入/更新。现在只做最少的事：
//   Build 时往 Root() 里塞组件，之后每帧 Update（布局 + 命中测试 + 更新）与 Render（画出来）。
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "component_view/Widget.h"

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

    virtual const char* Title() const = 0;

    // ---- 生命周期（子类重写） ----------------------------------------------
    virtual void OnBuild() {}
    virtual void OnInput() {}
    virtual void OnUpdate(float dt) { (void)dt; }
    virtual void OnOverlay(ImDrawList* dl) { (void)dl; }

    // ---- 宿主调用 ----------------------------------------------------------
    void Bind(UiContext& ui) { ui_ = &ui; }
    void Update(float dt);
    void Render();

    // 页面根节点：位置相对页面左上角，铺满整个画布
    Box& Root();
    // 切主题后调一次：重置根节点装饰（整页容器本来就不画底/边框/阴影），
    // 再让整棵组件树重新取色。Global::ApplyTheme() 也一起做了。
    void RefreshTheme();
    UiContext& ui() const { return *ui_; }

private:
    std::unique_ptr<Box> root_;
    UiContext* ui_ = nullptr;
    std::vector<Widget*> focusables_;
    bool built_ = false;
};

} // namespace gui_dev::cv
