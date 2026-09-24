#include "ui/Texture.h"

#include <utility>

namespace gui_dev {

TextureRef::TextureRef(Backend& backend, const char* relative_asset_path) : backend_(&backend) {
    texture_ = backend_->LoadTexture(relative_asset_path);
}

TextureRef::~TextureRef() { Reset(); }

TextureRef::TextureRef(TextureRef&& other) noexcept
    : backend_(other.backend_), texture_(other.texture_) {
    other.backend_ = nullptr;
    other.texture_ = Texture{};
}

TextureRef& TextureRef::operator=(TextureRef&& other) noexcept {
    if (this != &other) {
        Reset();
        backend_ = other.backend_;
        texture_ = other.texture_;
        other.backend_ = nullptr;
        other.texture_ = Texture{};
    }
    return *this;
}

void TextureRef::Reset() {
    if (backend_ != nullptr && texture_.Valid()) {
        backend_->ReleaseTexture(texture_);
    }
    texture_ = Texture{};
}

} // namespace gui_dev
