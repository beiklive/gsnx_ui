#include "ui/Icons.h"

#include <cstdio>

namespace gui_dev::Icons {
namespace {

struct Entry {
    Button button;
    const char* glyph;
    const char* label;
    std::uint32_t code;
};

// 码位与字形来源的对应关系（与 HOS NintendoExt / switch_icons.ttf 一致）：
//   A=E0E0 B=E0E1 X=E0E2 Y=E0E3 L=E0E4 R=E0E5 ZL=E0E6 ZR=E0E7
//   +=E0EF −=E0F0 ↑=E0EB ↓=E0EC ←=E0ED →=E0EE L3=E104 R3=E105
constexpr Entry kTable[] = {
    {Button::A, "\uE0E0", "A", 0xE0E0},
    {Button::B, "\uE0E1", "B", 0xE0E1},
    {Button::X, "\uE0E2", "X", 0xE0E2},
    {Button::Y, "\uE0E3", "Y", 0xE0E3},
    {Button::L, "\uE0E4", "L", 0xE0E4},
    {Button::R, "\uE0E5", "R", 0xE0E5},
    {Button::ZL, "\uE0E6", "ZL", 0xE0E6},
    {Button::ZR, "\uE0E7", "ZR", 0xE0E7},
    {Button::Plus, "\uE0EF", "+", 0xE0EF},
    {Button::Minus, "\uE0F0", "\xE2\x88\x92", 0xE0F0}, // 减号 U+2212
    {Button::Up, "\uE0EB", "\xE2\x86\x91", 0xE0EB},    // ↑
    {Button::Down, "\uE0EC", "\xE2\x86\x93", 0xE0EC},  // ↓
    {Button::Left, "\uE0ED", "\xE2\x86\x90", 0xE0ED},  // ←
    {Button::Right, "\uE0EE", "\xE2\x86\x92", 0xE0EE}, // →
    {Button::L3, "\uE104", "L3", 0xE104},
    {Button::R3, "\uE105", "R3", 0xE105},
};

// 编译期自检：确认源文件/执行字符集是 UTF-8（\uE0E0 必须编码成 EE 83 A0），
// 否则图标会显示成乱码。任一平台的编译器不满足都要在这里先炸掉。
constexpr bool IsUtf8_E0E0(const char* s) {
    return static_cast<unsigned char>(s[0]) == 0xEE && static_cast<unsigned char>(s[1]) == 0x83 &&
           static_cast<unsigned char>(s[2]) == 0xA0 && s[3] == '\0';
}
static_assert(IsUtf8_E0E0(kTable[0].glyph), "执行字符集不是 UTF-8，\\u 转义无法得到正确字节");

static_assert(sizeof(kTable) / sizeof(kTable[0]) == kButtonCount, "图标表与 Button 枚举数量不一致");

// 查表用按钮值直接做下标，因此表顺序必须与枚举一致。
constexpr bool TableOrderOk() {
    for (std::size_t i = 0; i < kButtonCount; ++i) {
        if (static_cast<std::size_t>(kTable[i].button) != i) {
            return false;
        }
    }
    return true;
}
static_assert(TableOrderOk(), "kTable 顺序必须与 Button 枚举一致");

const Entry& Find(Button b) { return kTable[static_cast<std::size_t>(b)]; }

// ---- Material Icons -------------------------------------------------------

struct MaterialEntry {
    Material icon;
    const char* label;
    std::uint32_t code;
};

// 码位来自 GBAStation/src/ui/utils/MaterialIcons.hpp，已逐个核对在本字体中存在。
constexpr MaterialEntry kMaterialTable[] = {
    {Material::Edit, "edit", 0xE3C9},
    {Material::Image, "image", 0xE3F4},
    {Material::InstallApp, "install_app", 0xE884},
    {Material::Memory, "memory", 0xE322},
    {Material::Storage, "storage", 0xE1DB},
    {Material::Delete, "delete", 0xE872},
    {Material::DeleteSweep, "delete_sweep", 0xE16C},
    {Material::Favorite, "favorite", 0xE87D},
    {Material::FavoriteBorder, "favorite_border", 0xE87E},
    {Material::CheckBox, "check_box", 0xE834},
    {Material::SelectAll, "select_all", 0xE162},
    {Material::Close, "close", 0xE5CD},
    {Material::Play, "play_arrow", 0xE037},
    {Material::Settings, "settings", 0xE8B8},
    {Material::Update, "update", 0xE923},
    {Material::Description, "description", 0xE873},
    {Material::Search, "search", 0xE8B6},
    {Material::ImagePlaceholder, "image_placeholder", 0xE3F4},
    {Material::CheckBoxOutline, "check_box_outline", 0xE835},
    {Material::Wifi, "wifi", 0xE63E},
    {Material::WifiOff, "wifi_off", 0xE648},
    {Material::Save, "save", 0xE161},
    {Material::Backup, "backup", 0xE864},
    {Material::Restore, "restore", 0xE8B3},
    {Material::CloudUpload, "cloud_upload", 0xE2C6},
    {Material::CloudDownload, "cloud_download", 0xE2C4},
    {Material::PhotoLibrary, "photo_library", 0xE413},
    {Material::Folder, "folder", 0xE2C7},
    {Material::Games, "games", 0xE30F},
    {Material::SportsEsports, "sports_esports", 0xEAE2},
    {Material::VideogameAsset, "videogame_asset", 0xEA1F},
    {Material::PhoneAndroid, "phone_android", 0xE324},
    {Material::Archive, "archive", 0xE149},
    {Material::FileGame, "file_game", 0xE338},
    {Material::HelpOutline, "help_outline", 0xE8FD},
    {Material::LightMode, "light_mode", 0xE518},
    {Material::DarkMode, "dark_mode", 0xE51C},
    {Material::ZoomIn, "zoom_in", 0xE8FF},
    {Material::ZoomOut, "zoom_out", 0xE900},
    {Material::CheckCircle, "check_circle", 0xE86C},
    {Material::ErrorOutline, "error_outline", 0xE001},
    {Material::Info, "info", 0xE88E},
};

static_assert(sizeof(kMaterialTable) / sizeof(kMaterialTable[0]) == kMaterialCount,
              "Material 图标表与枚举数量不一致");

constexpr bool MaterialOrderOk() {
    for (std::size_t i = 0; i < kMaterialCount; ++i) {
        if (static_cast<std::size_t>(kMaterialTable[i].icon) != i) {
            return false;
        }
    }
    return true;
}
static_assert(MaterialOrderOk(), "kMaterialTable 顺序必须与 Material 枚举一致");

const MaterialEntry& Find(Material m) { return kMaterialTable[static_cast<std::size_t>(m)]; }

// Material 码位全部落在 U+0800..U+FFFF，UTF-8 固定 3 字节。
// 手写 35 条 \uXXXX 转义没必要，直接按码位编码更不容易错。
const char* Utf8FromBmpCodePoint(std::uint32_t code) {
    static char buffers[8][4];
    static unsigned next = 0;
    char* buffer = buffers[next++ % 8];
    buffer[0] = static_cast<char>(0xE0u | ((code >> 12) & 0x0Fu));
    buffer[1] = static_cast<char>(0x80u | ((code >> 6) & 0x3Fu));
    buffer[2] = static_cast<char>(0x80u | (code & 0x3Fu));
    buffer[3] = '\0';
    return buffer;
}

} // namespace

const char* Glyph(Button b) { return Find(b).glyph; }

const char* Label(Button b) { return Find(b).label; }

std::uint32_t Code(Button b) { return Find(b).code; }

const char* CodePoint(Button b) {
    // 4 槽轮转，避免同一表达式里多次调用互相覆盖。
    static char buffers[4][8];
    static unsigned next = 0;
    char* buffer = buffers[next++ % 4];
    std::snprintf(buffer, sizeof(buffers[0]), "U+%04X", static_cast<unsigned>(Find(b).code));
    return buffer;
}

Button FromAction(InputAction action) {
    switch (action) {
    case InputAction::Confirm:
        return Button::A;
    case InputAction::Cancel:
        return Button::B;
    case InputAction::ActionX:
        return Button::X;
    case InputAction::ActionY:
        return Button::Y;
    case InputAction::Menu:
        return Button::Plus;
    case InputAction::Minus:
        return Button::Minus;
    case InputAction::PageLeft:
        return Button::L;
    case InputAction::PageRight:
        return Button::R;
    case InputAction::TriggerLeft:
        return Button::ZL;
    case InputAction::TriggerRight:
        return Button::ZR;
    case InputAction::Up:
        return Button::Up;
    case InputAction::Down:
        return Button::Down;
    case InputAction::Left:
        return Button::Left;
    case InputAction::Right:
        return Button::Right;
    case InputAction::None:
    default:
        return Button::Count;
    }
}

const char* Glyph(Material m) { return Utf8FromBmpCodePoint(Find(m).code); }

const char* Label(Material m) { return Find(m).label; }

std::uint32_t Code(Material m) { return Find(m).code; }

} // namespace gui_dev::Icons
