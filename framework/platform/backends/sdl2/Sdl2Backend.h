// SDL2 后端：mac 与 Switch 共用。
//
// - mac：homebrew 的 sdl2-compat（提供 SDL 2 API）+ SDL_Renderer2
// - Switch：devkitpro portlibs 的 sdl2 + SDL_Renderer2
// 两边都是 SDL2 API，所以不存在平台分支；真正需要分支的只有窗口 flag 与
// 手柄映射（Switch 用 libnx 的 HID 映射表，见 PollEvents）。
#pragma once

#include <SDL.h>

#include <cstdint>

#include "platform/Backend.h"

namespace gui_dev {

class Sdl2Backend final : public Backend {
public:
    Sdl2Backend() = default;
    ~Sdl2Backend() override;

    BackendStatus Init(const BackendConfig& cfg) override;
    void Shutdown() override;

    bool ShouldQuit() const override { return quit_; }
    void RequestQuit() override { quit_ = true; }
    void PollEvents(InputFrame& in) override;

    void BeginRenderFrame() override;
    void EndRenderFrame() override;

    bool InitImGuiBackend() override;
    void ShutdownImGuiBackend() override;
    void NewImGuiFrame() override;

    float DeltaTime() const override { return delta_time_; }
    void GetDrawableSize(int& w, int& h) const override;
    std::uint32_t DisplayGeneration() const override { return display_generation_; }
    float UiScale() const override { return ui_scale_; }
    void SetUiZoom(float zoom) override;
    float UiZoom() const override { return ui_zoom_; }
    const char* DriverName() const override { return driver_name_.c_str(); }

    std::string ResolveAssetPath(const char* relative_path) const override;
    Texture LoadTexture(const char* relative_asset_path) override;
    void ReleaseTexture(Texture& texture) override;

private:
    void ApplyAction(InputFrame& in, SDL_Keycode key, InputAction action, bool down);
    // 手柄面键 -> 动作。SDL 的手柄 API 是按「位置」报的（A=下、B=右、X=左、Y=上，即 Xbox 习惯），
    // 而 Switch 手柄机身上印的是 A=右、B=下、X=上、Y=左，所以识别到任天堂手柄时把面键对掉。
    InputAction ActionForButton(SDL_GameControllerButton button) const;
    void UpdateFaceButtonSwap();
    // with_zoom=false 时只按分辨率算（字体密度用这个，不含用户缩放）
    float ComputeUiScale(bool with_zoom = true) const;
    // 当前逻辑画布尺寸 = drawable / 渲染缩放。用当前值算，不依赖上一帧的 io.DisplaySize
    // （窗口尺寸/缩放刚变的那一帧，io.DisplaySize 还是旧值，触摸就会偏）。
    ImVec2 LogicalSizeNow() const;
    // 窗口点（SDL 事件坐标，Retina 上是「点」）-> 逻辑画布坐标。
    // 鼠标和触摸都走这一个换算，避免两条路径各算一套导致对不上。
    ImVec2 WindowToLogical(const ImVec2& window_point) const;
    ImVec2 MouseEventToLogical(const ImVec2& event_point) const;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_GameController* controller_ = nullptr;

    bool quit_ = false;
    bool imgui_backend_inited_ = false;
    // 触摸：只跟踪第一根手指，把 SDL_FINGER* 翻译成 ImGui 鼠标事件。
    // 注意：必须在 ImGui::NewFrame() 之前喂（见 NewImGuiFrame），
    // 否则会被 imgui_impl_sdl2 的鼠标更新覆盖掉。
    bool touch_engaged_ = false;
    bool touch_release_pending_ = false;
    SDL_FingerID touch_finger_ = 0;
    ImVec2 touch_pos_{0.0f, 0.0f};
    // 面键是否按任天堂布局对掉（A/B、X/Y）。由手柄类型 / 平台 / GUI_DEV_FACE_SWAP 决定
    bool swap_face_buttons_ = false;
    // 上一帧的按住状态：SDL 只在按下/松开时各发一次事件，held 要在帧之间继承（长按要用）
    bool pad_held_[static_cast<std::size_t>(InputAction::Count)] = {};
    std::uint64_t perf_counter_ = 0;
    float delta_time_ = 0.0f;

    std::uint32_t display_generation_ = 0;
    int last_drawable_w_ = 0;
    int last_drawable_h_ = 0;
    float ui_scale_ = 1.0f;  // 渲染缩放 = 自动缩放 × 用户缩放
    float auto_scale_ = 1.0f; // 只按分辨率算的自动缩放（字体光栅化密度用）
    float ui_zoom_ = 1.0f;    // 用户缩放（放大/缩小按钮），乘在自动缩放之上

    BackendConfig cfg_{};
    // Init 时构建一次（避免每帧分配）
    std::string driver_name_{"unknown"};
};

} // namespace gui_dev
