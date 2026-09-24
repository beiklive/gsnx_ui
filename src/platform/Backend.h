// 平台后端接口：统一前端组件与操作系统/图形 API 之间唯一的接缝。
//
// 新增平台（Switch / Windows / Linux）只需实现 Backend 并在 CMake 里加一个
// 后端目标，src/ui 与 src/core 一行都不用改。
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "platform/Input.h"

namespace gui_dev {

// 运行平台。由编译期宏决定，业务代码用它做平台差异分支，
// 取代散落的 #ifdef __SWITCH__ / __APPLE__。
enum class PlatformKind : std::uint8_t {
    Unknown = 0,
    Switch,
    Mac,
    Windows,
    Linux,
};

#if defined(GUI_DEV_PLATFORM_switch)
inline constexpr PlatformKind kPlatform = PlatformKind::Switch;
#elif defined(GUI_DEV_PLATFORM_mac)
inline constexpr PlatformKind kPlatform = PlatformKind::Mac;
#elif defined(GUI_DEV_PLATFORM_windows)
inline constexpr PlatformKind kPlatform = PlatformKind::Windows;
#elif defined(GUI_DEV_PLATFORM_linux)
inline constexpr PlatformKind kPlatform = PlatformKind::Linux;
#else
inline constexpr PlatformKind kPlatform = PlatformKind::Unknown;
#endif

inline constexpr bool kIsHandheld = kPlatform == PlatformKind::Switch;

inline const char* PlatformName() {
#if defined(GUI_DEV_PLATFORM_switch)
    return "switch";
#elif defined(GUI_DEV_PLATFORM_mac)
    return "mac";
#elif defined(GUI_DEV_PLATFORM_windows)
    return "windows";
#elif defined(GUI_DEV_PLATFORM_linux)
    return "linux";
#else
    return "unknown";
#endif
}

struct BackendConfig {
    std::string title = "GUI_DEV";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool resizable = true;
    // Switch 端：手持/底座模式由 libnx 决定分辨率，这里只在桌面端生效。
    bool high_dpi = true;
};

enum class BackendStatus : std::uint8_t {
    Ok = 0,
    InitFailed,
    Unsupported,
};

class Backend {
public:
    virtual ~Backend() = default;

    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    virtual BackendStatus Init(const BackendConfig& cfg) = 0;
    virtual void Shutdown() = 0;

    // true = 上层应退出主循环（窗口关闭 / HOME 键等）。
    virtual bool ShouldQuit() const = 0;

    // 事件泵 + 输入采样。每帧在 UiContext::BeginFrame() 之前调用一次。
    virtual void PollEvents(InputFrame& in) = 0;

    // 帧绘制：清屏 -> ImGui 渲染数据 -> 呈现。
    virtual void BeginRenderFrame() = 0;
    virtual void EndRenderFrame() = 0;

    // 与 ImGui backend 对接，由 UiContext 调用；实现里管住
    // ImGui_ImplSDL2_InitForSDLRenderer 这类调用。
    virtual bool InitImGuiBackend() = 0;
    virtual void ShutdownImGuiBackend() = 0;
    virtual void NewImGuiFrame() = 0;

    virtual float DeltaTime() const = 0;
    virtual void GetDrawableSize(int& w, int& h) const = 0;

    // 分辨率变化（Switch 手持<->底座切换、桌面窗口缩放）时递增，
    // 上层据此重建字体/布局。
    virtual std::uint32_t DisplayGeneration() const = 0;

    // 逻辑尺寸 = 设计基准分辨率，UI 布局按它排版，实现负责缩放。
    virtual float UiScale() const = 0;

protected:
    Backend() = default;
};

// 编译期选择的后端实现（见 CMakeLists.txt 的 GUI_DEV_BACKEND）。
std::unique_ptr<Backend> CreatePlatformBackend();

} // namespace gui_dev
