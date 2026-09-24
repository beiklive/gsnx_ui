// 平台后端接口：统一前端组件与操作系统/图形 API 之间唯一的接缝。
//
// 新增平台（Switch / Windows / Linux）只需实现 Backend 并在 CMake 里加一个
// 后端目标，src/ui 与 src/core 一行都不用改。
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <imgui.h>

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

// 已上传到 GPU 的纹理句柄。id 的解释权在后端（SDL2 下是 SDL_Texture*），
// 上层只把它当不透明标识交给 ImGui 画。
struct Texture {
    ImTextureID id = 0;
    int width = 0;
    int height = 0;

    bool Valid() const { return id != 0; }
};

// 与 Backend 生命周期绑定的存活标记。
//
// 长期持有 Backend 引用/指针的对象（典型是 TextureRef）必须一并保存它：
// Backend 一析构标记就置 false，于是对象即使活得更久也不会去回调已释放的
// Backend，而是安全跳过并打警告。这是为了避免「退出时崩溃」这类悬垂调用。
class BackendLiveness {
public:
    bool Alive() const { return *alive_; }

private:
    friend class Backend;
    void MarkDead() { *alive_ = false; }

    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

class Backend {
public:
    // 析构时置存活标记：让仍持有旧 Backend 引用的对象不会再回调进来。
    virtual ~Backend() { liveness_.MarkDead(); }

    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    virtual BackendStatus Init(const BackendConfig& cfg) = 0;
    virtual void Shutdown() = 0;

    // true = 上层应退出主循环（窗口关闭 / HOME 键等）。
    virtual bool ShouldQuit() const = 0;

    // 请求退出主循环（UI 里的「退出」入口、自动化冒烟测试等）。
    // 主循环会在下一轮检查 ShouldQuit() 后走正常退出流程。
    virtual void RequestQuit() = 0;

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

    // ---- 资源 --------------------------------------------------------------
    // 解析 assets/ 下的相对路径（见 platform/AssetPaths.h）。
    virtual std::string ResolveAssetPath(const char* relative_path) const = 0;

    // 加载 assets/ 下的图片为纹理。失败返回 Valid()==false，不抛异常。
    virtual Texture LoadTexture(const char* relative_asset_path) = 0;
    virtual void ReleaseTexture(Texture& texture) = 0;

    // 存活标记。任何生命周期可能长于 Backend 的对象都该存一份。
    const BackendLiveness& Liveness() const { return liveness_; }

protected:
    Backend() = default;

private:
    BackendLiveness liveness_;
};

// 编译期选择的后端实现（见 CMakeLists.txt 的 GUI_DEV_BACKEND）。
std::unique_ptr<Backend> CreatePlatformBackend();

} // namespace gui_dev
