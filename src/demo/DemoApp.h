// 示例应用：用统一组件库搭一个启动器风格的界面。
// 真正接入时把 DemoApp 换成各核心自己的 App 实现即可，UI 代码不用改。
#pragma once

#include <string>
#include <vector>

#include "core/App.h"
#include "ui/Components.h"
#include "ui/Scene.h"

namespace gui_dev::demo {

struct GameEntry {
    std::string title;
    std::string core;
    std::string release;
    std::string path;
};

class LibraryScene final : public Scene {
public:
    explicit LibraryScene(std::vector<GameEntry> games, std::string version);

    const char* Name() const override { return "library"; }
    void OnRender(UiContext& ui) override;

private:
    std::vector<GameEntry> games_;
    std::string version_;
    int selected_ = 0;
};

class DemoApp final : public App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;

private:
    std::string version_ = GUI_DEV_VERSION;
};

} // namespace gui_dev::demo
