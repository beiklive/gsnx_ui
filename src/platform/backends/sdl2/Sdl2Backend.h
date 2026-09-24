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

    std::string ResolveAssetPath(const char* relative_path) const override;
    Texture LoadTexture(const char* relative_asset_path) override;
    void ReleaseTexture(Texture& texture) override;

private:
    void ApplyAction(InputFrame& in, SDL_Keycode key, InputAction action, bool down);
    float ComputeUiScale() const;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_GameController* controller_ = nullptr;

    bool quit_ = false;
    bool imgui_backend_inited_ = false;
    std::uint64_t perf_counter_ = 0;
    float delta_time_ = 0.0f;

    std::uint32_t display_generation_ = 0;
    int last_drawable_w_ = 0;
    int last_drawable_h_ = 0;
    float ui_scale_ = 1.0f;

    BackendConfig cfg_{};
};

} // namespace gui_dev
