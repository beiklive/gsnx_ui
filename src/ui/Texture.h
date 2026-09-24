// 图片纹理的 RAII 封装：src/ui 只认这个句柄，平台细节留在 Backend 里。
#pragma once

#include <imgui.h>

#include "platform/Backend.h"

namespace gui_dev {

// 由 Backend 加载的 assets 图片。析构时自动释放。
// 不可拷贝；可移动（用于放进 Scene/App 成员）。
class TextureRef {
public:
    TextureRef() = default;
    TextureRef(Backend& backend, const char* relative_asset_path);
    ~TextureRef();

    TextureRef(const TextureRef&) = delete;
    TextureRef& operator=(const TextureRef&) = delete;
    TextureRef(TextureRef&& other) noexcept;
    TextureRef& operator=(TextureRef&& other) noexcept;

    bool Valid() const { return texture_.Valid(); }
    int Width() const { return texture_.width; }
    int Height() const { return texture_.height; }
    ImVec2 Size() const {
        return ImVec2(static_cast<float>(texture_.width), static_cast<float>(texture_.height));
    }

    // 交给 ImGui::Image / AddImageQuad 用。无效纹理返回 0（画不出东西，但不崩）。
    ImTextureRef ImGuiRef() const { return ImTextureRef(texture_.id); }

    void Reset();

private:
    Backend* backend_ = nullptr;
    // Backend 可能先于本对象析构（例如场景比后端活得久）：
    // 靠存活标记判空，否则退出时会对已释放的 Backend 调 ReleaseTexture 而崩溃。
    BackendLiveness liveness_;
    Texture texture_{};
};

} // namespace gui_dev
