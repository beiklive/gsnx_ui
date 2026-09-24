// 平台字体来源：统一前端需要「文本字体 + 任天堂按键图标字体」两类来源，
// 各平台的获取方式不同（Switch 走 HOS 共享字体，桌面走 assets/ 里的 ttf），
// 但 src/ui 只认这里的 FontSource，不认平台 API。
//
// 每个后端目录下各提供一份实现：
//   backends/sdl2/SwitchFonts.cpp   -> pl:u 共享字体（PlSharedFontType_NintendoExt）
//   backends/sdl2/DesktopFonts.cpp  -> assets/font/*.ttf
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gui_dev {

enum class FontRole : std::uint8_t {
    Primary = 0, // 文本主字体
    Icons,       // 按键图标字体，合并进主字体
};

// 一个字体源。path 与 data 二选一，data 优先。
// data 指向的内存必须比字体图集活得更久（Switch 上是 pl 的共享内存）。
struct FontSource {
    std::string path;
    const void* data = nullptr;
    std::size_t size = 0;
    float size_pixels = 18.0f;
    FontRole role = FontRole::Primary;
};

// 平台字体服务生命周期。失败不致命：仅表示图标/系统字体缺失。
// Switch 上对应 plInitialize / plExit，必须在任何 CollectPlatformFontSources 之前。
bool PlatformFontsInit();
void PlatformFontsShutdown();

// 收集本平台的字体源，按优先级排列（先加入的优先）。
void CollectPlatformFontSources(std::vector<FontSource>& out);

} // namespace gui_dev
