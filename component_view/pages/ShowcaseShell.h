// ShowcaseShell：Demo 主页面。
//
//   ┌──────────┬───────────────────────────────────────┐
//   │ > BOX    │  控件名 + 说明                        │
//   │   LABEL  │  ┌──────── Showcase（可操作）────────┐ │
//   │   ...    │  └───────────────────────────────────┘ │
//   │          │  Properties / Visual / State / Nav     │
//   └──────────┴───────────────────────────────────────┘
//
// 左侧 TabBar 是一个焦点停靠点，右侧展示区里的控件是各自独立的焦点停靠点：
//   ↑ ↓    在 Tab 之间移动（右侧时在控件之间移动）
//   → / A  从 Tab 进入内容（A 直接选中）
//   ← / B  从内容回到 Tab
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "component_view/pages/ControlPage.h"
#include "component_view/pages/Page.h"

namespace gui_dev::cv {

class ShowcaseShell : public Page {
public:
    const char* Title() const override { return "component_view · 控件库"; }

    void OnBuild() override;
    void OnUpdate(float dt) override;
    void OnInput() override;

    TabBar& Tabs() { return *tabs_; }
    ControlPage* ActivePage() const;

private:
    void SelectTab(int index, bool move_focus_to_content);
    void RefreshHints();

    std::vector<std::unique_ptr<ControlPage>> pages_;
    TabBar* tabs_ = nullptr;
    Box* showcase_ = nullptr;
    class PropertyPanel* properties_ = nullptr;
    class Label* title_label_ = nullptr;
    class Label* summary_label_ = nullptr;
    int active_ = -1;
};

} // namespace gui_dev::cv
