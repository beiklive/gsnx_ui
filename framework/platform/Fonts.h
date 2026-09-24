// 平台字体来源：统一前端需要「主文本字体 + 若干附加字形（中文补充 / 按键图标 /
// Material 图标）」两类来源，各平台获取方式不同，但 src/ui 只认这里的 FontSource。
//
// 各后端目录下各提供一份实现：
//   backends/sdl2/SwitchPlatform.cpp   -> pl:u 共享字体 + romfs
//   backends/sdl2/DesktopPlatform.cpp  -> assets/font/*.ttf
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gui_dev {

enum class FontRole : std::uint8_t {
    Primary = 0, // 主文本字体；多个 Primary 时取第一个加载成功的
    Merge,       // 合并进主字体；**按声明顺序优先**，先声明者抢占同码位字形
};

// 字体提供的内容类型。合并多个字体时，同一码位可能被前面的字体遮蔽
// （例如 NintendoExt 覆盖了整整 1022 个私用区码位，会把 Material 图标挡掉），
// 因此由 UI 层按这个标记生成 GlyphExcludeRanges，保证各字体只提供自己那份。
enum class FontContent : std::uint8_t {
    Text = 0,      // 文本字形
    ButtonIcons,   // 任天堂按键图标（U+E0E0…）
    MaterialIcons, // Material 图标
};

// 一个字体源。path 与 data 二选一，data 优先。
// data 指向的内存必须比字体图集活得更久（Switch 上是 pl 的共享内存）。
struct FontSource {
    std::string path;
    const void* data = nullptr;
    std::size_t size = 0;
    float size_pixels = 18.0f;
    FontRole role = FontRole::Primary;
    FontContent content = FontContent::Text;
};

// 收集本平台的字体源，按优先级排列（先加入的优先）。
void CollectPlatformFontSources(std::vector<FontSource>& out);

} // namespace gui_dev
