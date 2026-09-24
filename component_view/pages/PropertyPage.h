// 属性演示页：坐标 / 位置 / 尺寸 / 圆角 / 边框 / 阴影 / z_order / 交互。
#pragma once

#include <string>

#include "component_view/components/Label.h"
#include "component_view/pages/Page.h"

namespace gui_dev::cv {

class PropertyPage : public Page {
public:
    const char* Title() const override { return "属性演示"; }

    void OnBuild() override;
    void OnUpdate(float dt) override;

private:
    Box* Section(const char* title, const ImVec2& position, const ImVec2& size);
    Box* CenteredBox(Widget* parent, const char* caption, const ImVec2& size, ImU32 background, ImU32 border_color);

    Label* status_label_ = nullptr;
    int count_ = 0;
};

} // namespace gui_dev::cv
