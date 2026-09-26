// JPEG -> RGBA8888 解码（stb_image，单头文件，public domain / MIT）。
// 与 PngLoader 输出同一种结构，Sdl2Backend::LoadTexture 按扩展名选解码器。
#pragma once

#include "platform/backends/sdl2/PngLoader.h" // 复用 PngImage 结构

namespace gui_dev {

// 失败返回 false（文件不存在 / 不是 JPEG）。成功时 pixels 由 stb 分配，用 FreePngImage 释放。
bool DecodeJpeg(const char* path, PngImage& out);

} // namespace gui_dev
