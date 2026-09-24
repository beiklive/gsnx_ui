#include "ui/Texture.h"

#include <cstdio>
#include <utility>

namespace gui_dev {

TextureRef::TextureRef(Backend& backend, const char* relative_asset_path)
    : backend_(&backend), liveness_(backend.Liveness()) {
    texture_ = backend_->LoadTexture(relative_asset_path);
}

TextureRef::~TextureRef() { Reset(); }

TextureRef::TextureRef(TextureRef&& other) noexcept
    : backend_(other.backend_), liveness_(other.liveness_), texture_(other.texture_) {
    other.backend_ = nullptr;
    other.texture_ = Texture{};
}

TextureRef& TextureRef::operator=(TextureRef&& other) noexcept {
    if (this != &other) {
        Reset();
        backend_ = other.backend_;
        liveness_ = other.liveness_;
        texture_ = other.texture_;
        other.backend_ = nullptr;
        other.texture_ = Texture{};
    }
    return *this;
}

void TextureRef::Reset() {
    if (texture_.Valid()) {
        if (backend_ != nullptr && liveness_.Alive()) {
            backend_->ReleaseTexture(texture_);
        } else {
            // 走到这里说明对象比 Backend 活得久，属于使用方式问题：
            // 正常应把持有纹理的场景在后端销毁前拆掉（AppRunner 已这样做）。
            std::fprintf(stderr,
                         "[gui_dev] 纹理在后端销毁之后才释放，已跳过（GPU 资源泄漏）。"
                         "请确保持有纹理的对象先于 Backend 析构。\n");
        }
    }
    texture_ = Texture{};
}

} // namespace gui_dev
