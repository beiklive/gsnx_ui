// 按键图标：任天堂私用区（U+E0xx / U+E1xx）编码。
//
// 字形来源按平台不同，但码位一致：
//   Switch : HOS 共享字体 PlSharedFontType_NintendoExt（见 platform/Fonts.h）
//   桌面   : assets/font/switch_icons.ttf
//
// 用法：Components 的按键提示直接传 Glyph()，不要手写 UTF-8 字节。
#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/Input.h"

namespace gui_dev::Icons {

enum class Button : std::uint8_t {
    A,
    B,
    X,
    Y,
    L,
    R,
    ZL,
    ZR,
    Plus,
    Minus,
    Up,
    Down,
    Left,
    Right,
    L3,
    R3,
    Count,
};

inline constexpr std::size_t kButtonCount = static_cast<std::size_t>(Button::Count);

// 私用区码位范围，供字体加载侧参考（图标字形本身走 imgui 动态光栅化）。
inline constexpr std::uint32_t kFirstCodePoint = 0xE0E0;
inline constexpr std::uint32_t kLastCodePoint = 0xE105;

// 字形的 UTF-8 字符串（3 字节 + 结束符），可直接交给 ImGui::Text 系列。
const char* Glyph(Button b);
// 可读名称："A" / "ZL" / "↑"
const char* Label(Button b);
// 私用区码位数值（0xE0E0…），用于字体覆盖率自检。
std::uint32_t Code(Button b);
// 码位文本："U+E0E0"。内部用 4 槽轮转缓冲，仅用于显示。
const char* CodePoint(Button b);
// 输入动作 -> 图标：Confirm->A、Cancel->B、Menu->Plus、PageLeft->L、PageRight->R、方向键同理。
// 无对应图标时返回 Button::Count。
Button FromAction(InputAction action);

// ---- Material Icons -------------------------------------------------------
// 谷歌 Material 图标，字形来自 assets/font/MaterialIcons-Regular.ttf
// （Switch 上打包进 NRO 的 romfs）。码位与 GBAStation/src/ui/utils/MaterialIcons.hpp
// 一致，并已逐个核对存在。
enum class Material : std::uint8_t {
    Edit,
    Image,
    InstallApp,
    Memory,
    Storage,
    Delete,
    DeleteSweep,
    Favorite,
    FavoriteBorder,
    CheckBox,
    SelectAll,
    Close,
    Play,
    Settings,
    Update,
    Description,
    Search,
    ImagePlaceholder,
    CheckBoxOutline,
    Wifi,
    WifiOff,
    Save,
    Backup,
    Restore,
    CloudUpload,
    CloudDownload,
    PhotoLibrary,
    Folder,
    Games,
    SportsEsports,
    VideogameAsset,
    PhoneAndroid,
    Archive,
    FileGame,
    HelpOutline,
    LightMode,
    DarkMode,
    Count,
};

inline constexpr std::size_t kMaterialCount = static_cast<std::size_t>(Material::Count);

// 字形（UTF-8）。返回指向内部轮转缓冲的指针，仅用于当帧绘制。
const char* Glyph(Material m);
// 可读名称："settings"
const char* Label(Material m);
// 码位数值，用于字体覆盖率自检。
std::uint32_t Code(Material m);

} // namespace gui_dev::Icons
