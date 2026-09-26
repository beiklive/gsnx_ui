#include "platform/backends/sdl2/JpegLoader.h"

// stb_image 实现只放在这一个翻译单元里；只启用 JPEG，避免和 libpng 重复。
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "third_party/stb/stb_image.h"

namespace gui_dev {

bool DecodeJpeg(const char* path, PngImage& out) {
    out = PngImage{};
    if (path == nullptr) {
        return false;
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    // 统一转成 RGBA8888，和 PngLoader 的输出保持一致（stride == width * 4）
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return false;
    }
    out.pixels = reinterpret_cast<std::uint8_t*>(pixels);
    out.width = width;
    out.height = height;
    return true;
}

} // namespace gui_dev
