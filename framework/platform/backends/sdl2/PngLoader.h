// PNG -> RGBA8888 解码（libpng）。仅平台层使用。
//
// 不用 SDL_image：mac 上没装（homebrew 无 sdl2_image），而 libpng 在
// mac 与 Switch 的 portlibs 里都是现成的，两端走同一条解码路径。
#pragma once

#include <cstdint>

namespace gui_dev {

// 解码结果。pixels 用 malloc 分配，调用方负责 FreePngImage。
struct PngImage {
    std::uint8_t* pixels = nullptr; // RGBA8888，行紧密排列（stride == width * 4）
    int width = 0;
    int height = 0;
};

// 失败返回 false（文件不存在 / 不是 PNG / 内存不足），失败时不产生待释放内存。
bool DecodePng(const char* path, PngImage& out);
void FreePngImage(PngImage& image);

} // namespace gui_dev
