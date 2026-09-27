// Switch：打包资源（romfs）+ sdmc 覆盖。
//
// 字体全部来自仓库 assets/font/，随 NRO 打包：
//   switch_font.ttf            主文本字体
//   switch_icons.ttf           任天堂按键图标（NintendoExt 转出）
//   MaterialIcons-Regular.ttf  Material 图标
// **不使用 HOS 共享字体（pl:u）**：共享字体随固件/区域版本变化，排版会跟着变，还要额外依赖
// pl 服务；改用仓库里这份固定的字体后，掌机/底座与桌面、Android 完全一致。
// 代价是 NRO 大 ~11MB（romfs 里带字体），换来的是排版可控 + 没有系统字体依赖。
// sdmc:/switch/GUI_DEV/assets/font/ 下放同名文件可以覆盖 romfs 里的版本（查找顺序见 AssetPaths.cpp）。
#include <switch.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include "platform/AssetPaths.h"
#include "platform/Fonts.h"
#include "platform/Platform.h"

namespace gui_dev {
namespace {

bool g_romfs_initialized = false;

// 日志文件：和资源目录同一处（见 AssetPaths.cpp 的 sdmc:/switch/GUI_DEV/assets/）。
// 目录只尝试创建一次；SD 没插/没权限时静默降级（不写文件，控制台/ nxlink 仍可用）。
constexpr const char* kLogDir = "sdmc:/switch/GUI_DEV";
constexpr const char* kLogPath = "sdmc:/switch/GUI_DEV/gui_dev.log";
bool g_log_dir_ready = false;

// 从 romfs（或 sdmc 覆盖目录）拿一份字体文件：imgui 自己 fopen 读，不占内存。
void AddAssetFont(std::vector<FontSource>& out, const char* relative_path, FontRole role,
                  FontContent content, const char* what) {
    const std::string path = ResolveAssetPath(relative_path);
    if (path.empty()) {
        std::fprintf(stderr, "[gui_dev] 找不到字体 %s（%s）\n", relative_path, what);
        return;
    }
    FontSource source;
    source.path = path;
    source.size_pixels = 18.0f;
    source.role = role;
    source.content = content;
    out.push_back(std::move(source));
}

} // namespace

void PlatformLogLine(const char* line) {
    if (line == nullptr || line[0] == '\0') {
        return;
    }
    if (!g_log_dir_ready) {
        g_log_dir_ready = true;
        std::error_code ec;
        std::filesystem::create_directories(kLogDir, ec);
        if (ec) {
            // 一般 libnx 启动时已经挂好 sdmc:；没挂上就补挂一次再试
            fsdevMountSdmc();
            ec.clear();
            std::filesystem::create_directories(kLogDir, ec);
        }
    }
    if (std::FILE* file = std::fopen(kLogPath, "ab")) {
        std::fputs(line, file);
        std::fputc('\n', file);
        std::fclose(file);
    }
}

const char* PlatformLogPath() {
    return kLogPath;
}

bool PlatformServicesInit() {
    // 字体/图片全部走 romfs（打包进 NRO），不再初始化 pl 服务。
    g_romfs_initialized = R_SUCCEEDED(romfsInit());
    if (!g_romfs_initialized) {
        std::fprintf(stderr, "[gui_dev] romfsInit 失败，打包的字体/图片不可用（主字体退回 imgui 内置）\n");
    }
    return true;
}

void PlatformServicesShutdown() {
    if (g_romfs_initialized) {
        romfsExit();
        g_romfs_initialized = false;
    }
}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    // 顺序即优先级：主文本 -> 按键图标 -> Material 图标。
    // content 决定每个源负责哪些码位：switch_icons 覆盖了 1022 个私用区码位，
    // 不声明清楚就会把 Material 图标整片遮蔽。
    AddAssetFont(out, "font/switch_font.ttf", FontRole::Primary, FontContent::Text, "主文本字体");
    AddAssetFont(out, "font/switch_icons.ttf", FontRole::Merge, FontContent::ButtonIcons, "按键图标");
    AddAssetFont(out, "font/MaterialIcons-Regular.ttf", FontRole::Merge, FontContent::MaterialIcons,
                 "Material 图标");
    if (out.empty()) {
        std::fprintf(stderr, "[gui_dev] 字体资源全部缺失，主字体将退回 imgui 内置字体\n");
    }
}

} // namespace gui_dev
