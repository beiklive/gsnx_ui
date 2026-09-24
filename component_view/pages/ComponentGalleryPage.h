// 组件总览页：Box / Label / Button / Image 四种基础组件 + VSCode 配色。
#pragma once

#include <string>

#include "component_view/components/Label.h"
#include "component_view/pages/Page.h"
#include "ui/Texture.h"

namespace gui_dev::cv {

class ComponentGalleryPage : public Page {
public:
    const char* Title() const override { return "组件总览"; }

    void OnBuild() override;
    void OnUpdate(float dt) override;

private:
    Box* Section(const char* title, const ImVec2& position, const ImVec2& size);
    Box* MiniBox(Box* parent, const char* caption, float radius, bool accent_border, bool soft_shadow);

    TextureRef flow_texture_;
    Label* status_label_ = nullptr;
    std::string last_action_;
    int click_count_ = 0;
    float elapsed_ = 0.0f;
};

} // namespace gui_dev::cv
