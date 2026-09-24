#include "core/App.h"

#include <cstdio>

#include <imgui.h>

#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev {
namespace {

// ImGui 的 1.92 动态字体 / texture 后端能力声明。
void ConfigureImGuiIo() {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // 嵌入式环境不落盘 imgui.ini，布局由代码决定
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavMoveSetMousePos = false; // 手柄导航不要抢鼠标位置
    io.ConfigNavCaptureKeyboard = true;
}

} // namespace

AppRunner::AppRunner(App& app) : app_(app) {}

AppRunner::~AppRunner() = default;

int AppRunner::Run() {
    BackendConfig cfg;
    app_.Configure(cfg, kPlatform);

    backend_ = CreatePlatformBackend();
    if (!backend_) {
        std::fprintf(stderr, "[gui_dev] no backend available\n");
        return 2;
    }
    if (backend_->Init(cfg) != BackendStatus::Ok) {
        std::fprintf(stderr, "[gui_dev] backend init failed\n");
        return 1;
    }

    // 顺序要求：CreateContext -> 平台 ImGui 后端 -> 字体/IO 配置 -> 首帧。
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ConfigureImGuiIo();

    if (!backend_->InitImGuiBackend()) {
        std::fprintf(stderr, "[gui_dev] imgui backend init failed\n");
        ImGui::DestroyContext();
        backend_->Shutdown();
        return 1;
    }

    ui_ = std::make_unique<UiContext>(*backend_);
    Theme::Apply(); // 统一视觉规范，必须在后端 ImGui 初始化之后
    ui_->RefreshIfDisplayChanged();
    app_.OnStart(*ui_);

    int exit_code = 0;
    while (!backend_->ShouldQuit()) {
        ui_->BeginFrame();
        ui_->RefreshIfDisplayChanged();
        app_.Scenes().DispatchInput(*ui_);

        const float dt = ui_->DeltaTime();
        app_.OnFrame(*ui_, dt);
        app_.Scenes().UpdateAll(*ui_, dt);
        app_.Scenes().RenderAll(*ui_);
        ui_->EndFrame();

        if (app_.Scenes().ApplyClosures()) {
            break; // 栈空 = 应用结束
        }
    }

    app_.OnShutdown(*ui_);

    // 顺序很重要：场景可能持有纹理等后端资源，必须在
    // ui 与 backend 销毁**之前**析构，否则场景析构会去回调已释放的 Backend
    // （表现为进程正常退出时 SIGSEGV）。App/场景栈随后析构时已是空的。
    app_.Scenes().Clear();

    ui_.reset();
    backend_->ShutdownImGuiBackend();
    ImGui::DestroyContext();
    backend_->Shutdown();
    backend_.reset();
    return exit_code;
}

} // namespace gui_dev
