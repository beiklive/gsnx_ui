#include "platform/backends/sdl2/Sdl2Backend.h"

#include <cstdio>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "platform/AssetPaths.h"
#include "platform/Fonts.h"
#include "platform/Platform.h"
#include "platform/backends/sdl2/PngLoader.h"

namespace gui_dev {
namespace {

inline std::size_t Idx(InputAction a) { return static_cast<std::size_t>(a); }

// 扳机轴转按键的阈值（ZL/ZR 在 SDL 里是模拟轴，不是按钮）
constexpr Sint16 kTriggerThreshold = 16384;

// SDL 的 keycode/button 到抽象动作的映射集中在这里，
// 上层（src/ui）永远只看到 InputAction。
struct KeyBinding {
    SDL_Keycode key;
    InputAction action;
};

constexpr KeyBinding kKeyBindings[] = {
    {SDLK_UP, InputAction::Up},
    {SDLK_w, InputAction::Up},
    {SDLK_DOWN, InputAction::Down},
    {SDLK_s, InputAction::Down},
    {SDLK_LEFT, InputAction::Left},
    {SDLK_a, InputAction::Left},
    {SDLK_RIGHT, InputAction::Right},
    {SDLK_d, InputAction::Right},
    {SDLK_RETURN, InputAction::Confirm},
    {SDLK_SPACE, InputAction::Confirm},
    {SDLK_ESCAPE, InputAction::Cancel},
    {SDLK_BACKSPACE, InputAction::Cancel},
    {SDLK_TAB, InputAction::Menu},
    {SDLK_q, InputAction::PageLeft},
    {SDLK_e, InputAction::PageRight},
    {SDLK_z, InputAction::TriggerLeft},
    {SDLK_c, InputAction::TriggerRight},
};

struct ButtonBinding {
    SDL_GameControllerButton button;
    InputAction action;
};

constexpr ButtonBinding kButtonBindings[] = {
    {SDL_CONTROLLER_BUTTON_DPAD_UP, InputAction::Up},
    {SDL_CONTROLLER_BUTTON_DPAD_DOWN, InputAction::Down},
    {SDL_CONTROLLER_BUTTON_DPAD_LEFT, InputAction::Left},
    {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, InputAction::Right},
    {SDL_CONTROLLER_BUTTON_A, InputAction::Confirm},
    {SDL_CONTROLLER_BUTTON_B, InputAction::Cancel},
    {SDL_CONTROLLER_BUTTON_START, InputAction::Menu},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, InputAction::PageLeft},
    {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, InputAction::PageRight},
};

} // namespace

Sdl2Backend::~Sdl2Backend() { Shutdown(); }

BackendStatus Sdl2Backend::Init(const BackendConfig& cfg) {
    cfg_ = cfg;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "[gui_dev] SDL_Init failed: %s\n", SDL_GetError());
        return BackendStatus::InitFailed;
    }

#if defined(GUI_DEV_PLATFORM_switch)
    // Switch：由 libnx 决定分辨率，忽略 cfg.width/height。
    Uint32 flags = SDL_WINDOW_FULLSCREEN;
    const int w = 1920;
    const int h = 1080;
#else
    Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if (cfg_.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    const int w = cfg_.width;
    const int h = cfg_.height;
#endif

    window_ = SDL_CreateWindow(cfg_.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    if (!window_) {
        std::fprintf(stderr, "[gui_dev] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return BackendStatus::InitFailed;
    }

    // SDL_Renderer2：mac 走 Metal/OpenGL，Switch 走 libnx 的 GL 后端。
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (cfg_.vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    renderer_ = SDL_CreateRenderer(window_, -1, renderer_flags);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::fprintf(stderr, "[gui_dev] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return BackendStatus::InitFailed;
    }

    if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
        controller_ = SDL_GameControllerOpen(0);
    }

    GetDrawableSize(last_drawable_w_, last_drawable_h_);
    ui_scale_ = ComputeUiScale();
    perf_counter_ = SDL_GetPerformanceCounter();

    // 平台服务：Switch 上是 pl:u（共享字体）与 romfs（打包资源）。
    // 失败只丢字体/图标，不阻止启动；必须在字体收集与纹理加载之前完成。
    PlatformServicesInit();
    return BackendStatus::Ok;
}

float Sdl2Backend::ComputeUiScale() const {
    // 基准高度 720（手持）。大屏（底座/桌面高 DPI）只把字体放大，
    // 布局仍按物理像素排版，这样不会出现整块 UI 被拉成两倍大的现象。
    int w = 0;
    int h = 0;
    GetDrawableSize(w, h);
    if (h <= 0) {
        return 1.0f;
    }
    float scale = static_cast<float>(h) / 720.0f;
    if (scale < 1.0f) {
        scale = 1.0f;
    }
    if (scale > 2.0f) {
        scale = 2.0f;
    }
    return scale;
}

void Sdl2Backend::Shutdown() {
    if (imgui_backend_inited_) {
        ShutdownImGuiBackend();
    }
    // 平台服务要在 imgui 之后关：共享字体内存在 plExit 后就失效了。
    PlatformServicesShutdown();
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void Sdl2Backend::ApplyAction(InputFrame& in, SDL_Keycode key, InputAction action, bool down) {
    (void)key;
    const std::size_t i = Idx(action);
    if (i >= kInputActionCount) {
        return;
    }
    if (down) {
        if (!in.pad.held[i]) {
            in.pad.pressed[i] = true;
        }
        in.pad.held[i] = true;
    } else {
        in.pad.held[i] = false;
    }
}

void Sdl2Backend::PollEvents(InputFrame& in) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);

        switch (e.type) {
        case SDL_QUIT:
            quit_ = true;
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                quit_ = true;
            }
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_F11) {
                // 桌面端调试用：切换全屏。
                Uint32 flags = SDL_GetWindowFlags(window_);
                SDL_SetWindowFullscreen(window_, (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                break;
            }
            if (!e.key.repeat) {
                for (const KeyBinding& b : kKeyBindings) {
                    if (b.key == e.key.keysym.sym) {
                        ApplyAction(in, b.key, b.action, true);
                    }
                }
            }
            break;
        case SDL_KEYUP:
            for (const KeyBinding& b : kKeyBindings) {
                if (b.key == e.key.keysym.sym) {
                    ApplyAction(in, b.key, b.action, false);
                }
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (!controller_ && SDL_IsGameController(e.cdevice.which)) {
                controller_ = SDL_GameControllerOpen(e.cdevice.which);
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (controller_ && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
                SDL_GameControllerClose(controller_);
                controller_ = nullptr;
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            for (const ButtonBinding& b : kButtonBindings) {
                if (b.button == static_cast<SDL_GameControllerButton>(e.cbutton.button)) {
                    ApplyAction(in, 0, b.action, true);
                }
            }
            break;
        case SDL_CONTROLLERBUTTONUP:
            for (const ButtonBinding& b : kButtonBindings) {
                if (b.button == static_cast<SDL_GameControllerButton>(e.cbutton.button)) {
                    ApplyAction(in, 0, b.action, false);
                }
            }
            break;
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP: {
            // 触摸坐标是归一化的，换算到 drawable 像素。Switch 掌机模式会用到。
            int w = 0;
            int h = 0;
            GetDrawableSize(w, h);
            in.touch.x = e.tfinger.x * static_cast<float>(w);
            in.touch.y = e.tfinger.y * static_cast<float>(h);
            in.touch.down = (e.type != SDL_FINGERUP);
            break;
        }
        default:
            break;
        }
    }

    // ZL/ZR 是模拟轴，每帧按阈值采样成按键（带按住语义，松开即释放）。
    if (controller_ != nullptr) {
        const bool zl = SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >
                        kTriggerThreshold;
        const bool zr = SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >
                        kTriggerThreshold;
        ApplyAction(in, 0, InputAction::TriggerLeft, zl);
        ApplyAction(in, 0, InputAction::TriggerRight, zr);
    }

    // 分辨率变化检测：Switch 手持<->底座、桌面窗口缩放、Retina 迁移都会走到这里。
    int w = 0;
    int h = 0;
    GetDrawableSize(w, h);
    if (w != last_drawable_w_ || h != last_drawable_h_) {
        last_drawable_w_ = w;
        last_drawable_h_ = h;
        ui_scale_ = ComputeUiScale();
        ++display_generation_;
        // 分辨率切换后上一帧的计时无意义，避免 dt 尖峰。
        perf_counter_ = SDL_GetPerformanceCounter();
    }

    // 帧计时统一在这里推进。
    const std::uint64_t now = SDL_GetPerformanceCounter();
    delta_time_ = static_cast<float>(now - perf_counter_) / static_cast<float>(SDL_GetPerformanceFrequency());
    perf_counter_ = now;
}

void Sdl2Backend::BeginRenderFrame() {
    SDL_SetRenderDrawColor(renderer_, 0x11, 0x13, 0x16, 0xFF);
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
}

void Sdl2Backend::EndRenderFrame() { SDL_RenderPresent(renderer_); }


bool Sdl2Backend::InitImGuiBackend() {
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_)) {
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(renderer_)) {
        ImGui_ImplSDL2_Shutdown();
        return false;
    }
    imgui_backend_inited_ = true;
    // 注意：主题（Theme::Apply）由 gui_dev 侧在 AppRunner 里套用。
    // 后端不能反过来依赖 gui_dev，否则两个静态库互相引用会形成链接环。
    return true;
}

void Sdl2Backend::ShutdownImGuiBackend() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    imgui_backend_inited_ = false;
}

void Sdl2Backend::NewImGuiFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    int w = 0;
    int h = 0;
    GetDrawableSize(w, h);
    // 布局按物理像素排版（Retina 上元素更小但更锐利），只按屏幕高度放大字号。
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    // 1.92 起全局字号缩放走 style.FontScaleMain（io.FontGlobalScale 已移除）。
    ImGui::GetStyle().FontScaleMain = ui_scale_;
    io.DeltaTime = delta_time_ > 0.0f ? delta_time_ : (1.0f / 60.0f);

    ImGui::NewFrame();
}

void Sdl2Backend::GetDrawableSize(int& w, int& h) const {
    w = cfg_.width;
    h = cfg_.height;
    if (renderer_) {
        SDL_GetRendererOutputSize(renderer_, &w, &h);
    }
}

std::string Sdl2Backend::ResolveAssetPath(const char* relative_path) const {
    return gui_dev::ResolveAssetPath(relative_path);
}

Texture Sdl2Backend::LoadTexture(const char* relative_asset_path) {
    Texture texture{};
    if (renderer_ == nullptr) {
        return texture;
    }

    const std::string path = ResolveAssetPath(relative_asset_path);
    if (path.empty()) {
        std::fprintf(stderr, "[gui_dev] 找不到资源：%s\n", relative_asset_path);
        return texture;
    }

    PngImage image;
    if (!DecodePng(path.c_str(), image)) {
        return texture;
    }

    // PngLoader 输出 RGBA8888；SDL_PIXELFORMAT_RGBA32 在小端上就是 ABGR8888，
    // 与内存里的 RGBA 字节序一致。
    SDL_Texture* sdl_texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                                SDL_TEXTUREACCESS_STATIC, image.width, image.height);
    if (sdl_texture == nullptr) {
        std::fprintf(stderr, "[gui_dev] SDL_CreateTexture 失败：%s\n", SDL_GetError());
        FreePngImage(image);
        return texture;
    }
    if (SDL_UpdateTexture(sdl_texture, nullptr, image.pixels, image.width * 4) != 0) {
        std::fprintf(stderr, "[gui_dev] SDL_UpdateTexture 失败：%s\n", SDL_GetError());
        SDL_DestroyTexture(sdl_texture);
        FreePngImage(image);
        return texture;
    }
    SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);

    texture.id = reinterpret_cast<ImTextureID>(sdl_texture);
    texture.width = image.width;
    texture.height = image.height;
    FreePngImage(image);

    std::fprintf(stderr, "[gui_dev] 纹理已加载 %s (%dx%d)\n", path.c_str(), texture.width, texture.height);
    return texture;
}

void Sdl2Backend::ReleaseTexture(Texture& texture) {
    if (texture.id != 0) {
        SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(texture.id));
        texture.id = 0;
    }
    texture.width = 0;
    texture.height = 0;
}

} // namespace gui_dev
