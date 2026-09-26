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

// UI 设计基准（720p）。所有 UI 尺寸都按这个空间设计。
constexpr float kDesignWidth = 1280.0f;
constexpr float kDesignHeight = 720.0f;

// 这些平台的主指针是触摸屏。SDL 在部分 Android 设备上即使关闭了
// SDL_HINT_TOUCH_MOUSE_EVENTS，仍会额外投递一组普通 SDL_MOUSE* 事件（which 不一定
// 是 SDL_TOUCH_MOUSEID）。若和 SDL_FINGER* 一起送进 ImGui，同一次手势会产生两次
// press/release，第二次还可能使用 (0,0) 或旧坐标，表现为任意位置都会误触左上控件。
#if defined(GUI_DEV_PLATFORM_android) || defined(GUI_DEV_PLATFORM_ios) || defined(GUI_DEV_PLATFORM_switch)
constexpr bool kTouchFirstPlatform = true;
#else
constexpr bool kTouchFirstPlatform = false;
#endif

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
    {SDLK_EQUALS, InputAction::Menu},
    {SDLK_MINUS, InputAction::Minus},
    {SDLK_x, InputAction::ActionX},
    {SDLK_y, InputAction::ActionY},
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
    {SDL_CONTROLLER_BUTTON_X, InputAction::ActionX},
    {SDL_CONTROLLER_BUTTON_Y, InputAction::ActionY},
    {SDL_CONTROLLER_BUTTON_START, InputAction::Menu},
    {SDL_CONTROLLER_BUTTON_BACK, InputAction::Minus},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, InputAction::PageLeft},
    {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, InputAction::PageRight},
};

// 任天堂布局：机身上的 A 在右边、B 在下边、X 在上边、Y 在左边，
// 和 SDL 按位置报出来的 A(下)/B(右)/X(左)/Y(上) 正好对掉。
constexpr ButtonBinding kNintendoFaceBindings[] = {
    {SDL_CONTROLLER_BUTTON_A, InputAction::Cancel},  // 位置在下 = 机身的 B
    {SDL_CONTROLLER_BUTTON_B, InputAction::Confirm}, // 位置在右 = 机身的 A
    {SDL_CONTROLLER_BUTTON_X, InputAction::ActionY}, // 位置在左 = 机身的 Y
    {SDL_CONTROLLER_BUTTON_Y, InputAction::ActionX}, // 位置在上 = 机身的 X
};

} // namespace

Sdl2Backend::~Sdl2Backend() { Shutdown(); }

BackendStatus Sdl2Backend::Init(const BackendConfig& cfg) {
    cfg_ = cfg;

    // 必须在 SDL_Init / 创建窗口之前设置；放到 CreateWindow 之后时 Android video
    // driver 可能已经决定要生成触摸合成鼠标事件。PollEvents 里仍有去重兜底。
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#ifdef SDL_HINT_MOUSE_TOUCH_EVENTS
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "[gui_dev] SDL_Init failed: %s\n", SDL_GetError());
        return BackendStatus::InitFailed;
    }

#if defined(GUI_DEV_PLATFORM_switch)
    // Switch：由 libnx 决定分辨率，忽略 cfg.width/height。
    Uint32 flags = SDL_WINDOW_FULLSCREEN;
    const int w = 1920;
    const int h = 1080;
#elif defined(GUI_DEV_PLATFORM_android)
    // SDL Android Activity 默认按非全屏样式启动；SDL_WINDOW_FULLSCREEN 会切换到
    // 沉浸模式并移除状态栏/导航栏，保证 native 画布与触摸坐标覆盖完整屏幕。
    Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (cfg_.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    const int w = cfg_.width;
    const int h = cfg_.height;
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


    // 非整数倍缩放时用线性过滤，避免锯齿
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
        controller_ = SDL_GameControllerOpen(0);
    }
    // 没有手柄也要跑一次：环境变量 / 平台默认值要生效，日志也才好排查
    UpdateFaceButtonSwap();

    GetDrawableSize(last_drawable_w_, last_drawable_h_);
    auto_scale_ = ComputeUiScale(false); // 只按分辨率算；用户缩放另外乘
    ui_scale_ = auto_scale_ * ui_zoom_;
    perf_counter_ = SDL_GetPerformanceCounter();

    // 驱动描述：只在这里拼一次（每帧读的是 c_str()，不产生分配）
    {
        SDL_version version{};
        SDL_GetVersion(&version);
        SDL_RendererInfo info{};
        const char* renderer_name = "?";
        if (SDL_GetRendererInfo(renderer_, &info) == 0 && info.name != nullptr) {
            renderer_name = info.name;
        }
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "SDL %d.%d.%d + %s", version.major, version.minor,
                      version.patch, renderer_name);
        driver_name_ = buffer;
    }

    // 平台服务：Switch 上是 pl:u（共享字体）与 romfs（打包资源）。
    // 失败只丢字体/图标，不阻止启动；必须在字体收集与纹理加载之前完成。
    PlatformServicesInit();
    return BackendStatus::Ok;
}

float Sdl2Backend::ComputeUiScale(bool with_zoom) const {
    // 设计基准：1280x720。UI 里写的一切尺寸都是这个空间里的值。
    //
    // scale = min(h/720, w/1280)：取两者中更受限的一个，于是逻辑画布恒为
    // 「>=1280x720」——设计布局永远能完整放下，不会因为画布变窄而被压扁，
    // 多出来的空间交给布局层自适应（面板居中、槽位改列数）。
    //   16:9 任意分辨率：两个比值相等 -> 纯等比放大（720p -> 1.0，1080p -> 1.5）
    //   4:3 / 更窄     ：受宽度限制    -> 逻辑画布变高，面板垂直居中
    int w = 0;
    int h = 0;
    GetDrawableSize(w, h);
    if (h <= 0 || w <= 0) {
        return 1.0f;
    }
    const float by_height = static_cast<float>(h) / kDesignHeight;
    const float by_width = static_cast<float>(w) / kDesignWidth;
    float scale = by_height < by_width ? by_height : by_width;
    // 用户缩放（「放大 / 缩小」按钮）乘在这里：逻辑画布 = drawable / scale，
    // 所以 zoom 变大 = 逻辑画布变小 = 界面变大。
    if (with_zoom) {
        scale *= ui_zoom_;
    }
    if (scale < 0.3f) {
        scale = 0.3f;
    }
    if (scale > 6.0f) {
        scale = 6.0f;
    }
    return scale;
}

ImVec2 Sdl2Backend::LogicalSizeNow() const {
    int drawable_w = 0;
    int drawable_h = 0;
    GetDrawableSize(drawable_w, drawable_h);
    const float scale = ui_scale_ > 0.0f ? ui_scale_ : 1.0f;
    return ImVec2(static_cast<float>(drawable_w) / scale, static_cast<float>(drawable_h) / scale);
}

// 鼠标事件坐标 -> 逻辑画布坐标。
//
// 坑：mac 上用 homebrew 的 sdl2-compat（底层是 SDL3）时，SDL_RenderSetScale 会连带把
// 鼠标事件也换算进"渲染逻辑空间"（= drawable / 渲染缩放，正好等于 io.DisplaySize）；
// 真 SDL2（Switch / Android / iOS / Windows）不做这个换算，事件给的是窗口坐标。
// 两者差一个 window/logical 的因子，一律套 WindowToLogical 会在 sdl2-compat 上多缩
// 一次 —— 命中区比画出来的大 window/logical 倍，鼠标还没到控件右下角就触发聚焦。
//
// 判据：SDL_GetMouseState() 在两种实现下都返回**窗口坐标**（已用 Quartz 直接读物理
// 光标位置交叉验证），所以事件坐标与它一致 => 真 SDL2；不一致 => 事件已在逻辑空间。
// 注意：两个空间在原点重合，判错的后果只在靠近原点处发生，而那里两者本来就相等。
ImVec2 Sdl2Backend::MouseEventToLogical(const ImVec2& event_point) const {
    int state_x = 0;
    int state_y = 0;
    SDL_GetMouseState(&state_x, &state_y);
    const float dx = event_point.x - static_cast<float>(state_x);
    const float dy = event_point.y - static_cast<float>(state_y);
    const bool window_space = dx > -2.0f && dx < 2.0f && dy > -2.0f && dy < 2.0f;
    if (window_space) {
        return WindowToLogical(event_point);
    }
    // 已经是逻辑空间，只钳制边界
    const ImVec2 logical = LogicalSizeNow();
    const auto clamp = [](float value, float lo, float hi) {
        return value < lo ? lo : (value > hi ? hi : value);
    };
    return ImVec2(clamp(event_point.x, 0.0f, logical.x), clamp(event_point.y, 0.0f, logical.y));
}

ImVec2 Sdl2Backend::WindowToLogical(const ImVec2& window_point) const {
    int window_w = 0;
    int window_h = 0;
    if (window_ != nullptr) {
        SDL_GetWindowSize(window_, &window_w, &window_h);
    }
    const ImVec2 logical = LogicalSizeNow();
    const auto clamp = [](float value, float lo, float hi) {
        return value < lo ? lo : (value > hi ? hi : value);
    };
    if (window_w <= 0 || window_h <= 0) {
        return ImVec2(clamp(window_point.x, 0.0f, logical.x), clamp(window_point.y, 0.0f, logical.y));
    }
    return ImVec2(clamp(window_point.x * logical.x / static_cast<float>(window_w), 0.0f, logical.x),
                  clamp(window_point.y * logical.y / static_cast<float>(window_h), 0.0f, logical.y));
}

// 注意：运行期改渲染缩放会让 SDL 的几何/视口状态错乱（mac 的 sdl2-compat + Metal 实测
// 20 帧内必崩：AGX "Region width OOB"），所以它只用于启动前（或分辨率变化时）设定。
// demo 里由 kDefaultZoom / GUI_DEV_ZOOM 在 OnStart 里设定，界面上不再提供运行期缩放按钮。
void Sdl2Backend::SetUiZoom(float zoom) {
    const float next = zoom < 0.5f ? 0.5f : (zoom > 3.0f ? 3.0f : zoom);
    if (next == ui_zoom_) {
        return;
    }
    ui_zoom_ = next;
    ui_scale_ = auto_scale_ * ui_zoom_;
    // 注意：这里**不**递增 display_generation_。
    // 缩放只改「逻辑画布 + 光栅化密度」，imgui 1.92 的字形是按需烘焙的，
    // 密度变了它会自己烘一份新的，不需要（也不应该在运行期）ClearFonts() 重建设备图集
    // —— Switch 上点放大/缩小崩溃就是死在那条重建路径上。
    std::fprintf(stderr, "[gui_dev] UI 缩放 %.2fx（渲染缩放 %.3f）\n", static_cast<double>(ui_zoom_),
                 static_cast<double>(ui_scale_));
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

void Sdl2Backend::UpdateFaceButtonSwap() {
    // 优先级：环境变量显式指定 > 手柄类型（任天堂手柄）> 编译平台（Switch）
    bool swap = false;
    if (const char* value = std::getenv("GUI_DEV_FACE_SWAP")) {
        swap = value[0] != '0';
    } else if (controller_ != nullptr &&
               SDL_GameControllerGetType(controller_) == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO) {
        swap = true; // Switch Pro / Joy-Con / 掌机自带手柄都是这个类型
    }
#if defined(GUI_DEV_PLATFORM_switch)
    if (std::getenv("GUI_DEV_FACE_SWAP") == nullptr) {
        swap = true; // 掌机自带手柄固定是任天堂布局，类型识别失败也不吃亏
    }
#endif
    if (swap != swap_face_buttons_) {
        swap_face_buttons_ = swap;
        std::fprintf(stderr, "[gui_dev] 手柄面键：%s\n", swap ? "任天堂布局（A/B、X/Y 对掉）" : "Xbox 布局");
    }
}

InputAction Sdl2Backend::ActionForButton(SDL_GameControllerButton button) const {
    if (swap_face_buttons_) {
        for (const ButtonBinding& b : kNintendoFaceBindings) {
            if (b.button == button) {
                return b.action;
            }
        }
    }
    for (const ButtonBinding& b : kButtonBindings) {
        if (b.button == button) {
            return b.action;
        }
    }
    return InputAction::None;
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
    // held 是电平语义：SDL 只在按下那一刻发一次 KEYDOWN，所以先把上一帧的按住状态继承下来，
    // 事件里再更新；这样「按住不放」在每帧都成立（长按加速要用），
    // pressed 也才真的等于「本帧刚按下」（原来每帧清零，系统按键重复会被当成连续按下）。
    for (std::size_t i = 0; i < kInputActionCount; ++i) {
        in.pad.held[i] = pad_held_[i];
    }
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        const bool mouse_event = e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN ||
                                 e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEWHEEL;
        // 桌面触屏通常正确标 SDL_TOUCH_MOUSEID；移动端某些 SDL/系统组合却会报普通
        // mouse id，因此 touch-first 平台屏蔽全部 SDL_MOUSE*，只认 SDL_FINGER*。
        const bool synthesized_touch_mouse =
            (e.type == SDL_MOUSEMOTION && e.motion.which == SDL_TOUCH_MOUSEID) ||
            ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) &&
             e.button.which == SDL_TOUCH_MOUSEID) ||
            (e.type == SDL_MOUSEWHEEL && e.wheel.which == SDL_TOUCH_MOUSEID);
        // 移动端统一只走 finger 队列。部分 Android 驱动会先投递合成 mouse、再投递
        // FINGERDOWN，无法靠 touch_engaged_ 判断先后，必须整条屏蔽才不会重复点击。
        const bool duplicate_touch_mouse = synthesized_touch_mouse || (kTouchFirstPlatform && mouse_event);
        if (!duplicate_touch_mouse) {
            // SDL mouse event 是窗口坐标，ImGui/组件使用逻辑画布坐标。必须在事件
            // 入队前只转换一次；对 io.MousePos 每帧再转换会把旧坐标不断缩向 (0,0)。
            SDL_Event imgui_event = e;
            if (e.type == SDL_MOUSEMOTION) {
                const ImVec2 logical =
                    MouseEventToLogical(ImVec2(static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)));
                imgui_event.motion.x = static_cast<Sint32>(logical.x);
                imgui_event.motion.y = static_cast<Sint32>(logical.y);
            } else if (e.type == SDL_MOUSEWHEEL) {
            // 滚轮刻度累计进 InputFrame（ImGui 那边同样会收到），供 ImageViewer 这类控件做滚轮缩放
            in.wheel += e.wheel.preciseY != 0.0f ? e.wheel.preciseY : static_cast<float>(e.wheel.y);
        } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                const ImVec2 logical =
                    MouseEventToLogical(ImVec2(static_cast<float>(e.button.x), static_cast<float>(e.button.y)));
                imgui_event.button.x = static_cast<Sint32>(logical.x);
                imgui_event.button.y = static_cast<Sint32>(logical.y);
            }
            ImGui_ImplSDL2_ProcessEvent(&imgui_event);
        }

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
                UpdateFaceButtonSwap();
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (controller_ && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
                SDL_GameControllerClose(controller_);
                controller_ = nullptr;
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            const auto button = static_cast<SDL_GameControllerButton>(e.cbutton.button);
            const InputAction action = ActionForButton(button);
            if (action != InputAction::None) {
                ApplyAction(in, 0, action, e.type == SDL_CONTROLLERBUTTONDOWN);
            }
            break;
        }
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP: {
            // SDL finger 坐标相对于窗口归一化；先还原窗口点数，再映射到当前逻辑画布。
            int window_w = 0;
            int window_h = 0;
            SDL_GetWindowSize(window_, &window_w, &window_h);
            const ImVec2 logical = WindowToLogical(
                ImVec2(e.tfinger.x * static_cast<float>(window_w), e.tfinger.y * static_cast<float>(window_h)));
            const float x = logical.x;
            const float y = logical.y;
            in.touch.x = x;
            in.touch.y = y;
            in.touch.down = (e.type != SDL_FINGERUP);

            // 只跟第一根手指（多点触控以最先按下的为准）
            if (e.type == SDL_FINGERDOWN && !touch_engaged_) {
                touch_engaged_ = true;
                touch_finger_ = e.tfinger.fingerId;
                touch_release_pending_ = false;
            }
            if (touch_engaged_ && e.tfinger.fingerId == touch_finger_) {
                touch_pos_ = ImVec2(x, y);
                if (e.type == SDL_FINGERUP) {
                    touch_release_pending_ = true;
                }
            }
            break;
        }
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            break;
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

    // 存回本帧的按住状态，供下一帧继承
    for (std::size_t i = 0; i < kInputActionCount; ++i) {
        pad_held_[i] = in.pad.held[i];
    }

    // 分辨率变化检测：Switch 手持<->底座、桌面窗口缩放、Retina 迁移都会走到这里。
    int w = 0;
    int h = 0;
    GetDrawableSize(w, h);
    if (w != last_drawable_w_ || h != last_drawable_h_) {
        last_drawable_w_ = w;
        last_drawable_h_ = h;
        auto_scale_ = ComputeUiScale(false);
        ui_scale_ = auto_scale_ * ui_zoom_;
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

void Sdl2Backend::EndRenderFrame() {
    SDL_RenderPresent(renderer_);
}


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

    // 逻辑空间 = 1280x720 设计空间：DisplaySize 是逻辑尺寸。
    //
    // 几何缩放必须交给 SDL：imgui_impl_sdlrenderer2 只把 FramebufferScale 用在
    // 裁剪矩形上，顶点是原样交给 SDL_RenderGeometryRaw 的（后端注释里写明：
    // 用户若设了 SDL_RenderSetScale 就由 SDL 负责缩放）。所以这里显式设置。
    const float scale = ui_scale_ > 0.0f ? ui_scale_ : 1.0f;
    SDL_RenderSetScale(renderer_, scale, scale);
    io.DisplaySize = ImVec2(static_cast<float>(w) / scale, static_cast<float>(h) / scale);
    // FramebufferScale 是 drawable / 逻辑画布的完整比例（包含用户缩放）。
    // 这样高分屏字形按最终物理像素密度光栅化，避免先按 2x 烘焙、再被 2.4x 放大而发糊。
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    // 字号已经在设计空间里定死，这里不能再乘一次（否则会双重放大）。
    ImGui::GetStyle().FontScaleMain = 1.0f;
    io.DeltaTime = delta_time_ > 0.0f ? delta_time_ : (1.0f / 60.0f);

    // 触摸 → 鼠标：必须在这里喂（ImGui_ImplSDL2_NewFrame 之后、ImGui::NewFrame 之前）。
    // 放在 PollEvents 里喂会被 imgui_impl_sdl2 的 UpdateMouseData 覆盖（窗口没被真鼠标悬停时
    // 它会用全局鼠标位置或 -FLT_MAX 覆盖 io.MousePos）。
    if (touch_engaged_) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(touch_pos_.x, touch_pos_.y);
        io.AddMouseButtonEvent(0, !touch_release_pending_);
        if (touch_release_pending_) {
            touch_release_pending_ = false;
            touch_engaged_ = false;
        }
    }

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
