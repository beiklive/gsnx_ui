// 示例应用：清空原有的启动器界面，从「可聚焦 Box」开始搭组件。
// 接入真实核心时把 DemoApp 换成自己的 App 实现，组件用法一致。
#pragma once

#include <string>

#include "core/App.h"
#include "ui/Components.h"
#include "ui/Scene.h"
#include "ui/Texture.h"

namespace gui_dev::demo {

// 首页：网格布局的可聚焦 Box。焦点索引由场景维护，
// 手柄方向键与鼠标悬停共用同一个焦点。
class HomeScene final : public Scene {
public:
    HomeScene(TextureRef flow_texture, std::string version);

    const char* Name() const override { return "home"; }
    void OnUpdate(UiContext& ui, float dt) override;
    void OnRender(UiContext& ui) override;

private:
    void FocusMove(int delta);

    TextureRef flow_texture_;
    std::string version_;
    std::string launched_;
    int focus_ = 0;
    int columns_ = 4;
};

class DemoApp final : public App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;

private:
    std::string version_ = GUI_DEV_VERSION;
};

} // namespace gui_dev::demo
