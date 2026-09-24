#include "demo/DemoApp.h"

#include <utility>

#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::demo {
namespace {

std::vector<GameEntry> SampleLibrary() {
    return {
        {"Mother 3 (Japan)", "GBA 内置核心", "2006-04-20", "sdmc:/roms/gba/mother3.gba"},
        {"Golden Sun", "GBA 内置核心", "2001-08-01", "sdmc:/roms/gba/golden_sun.gba"},
        {"Ridge Racer", "PSP 独立核心", "2004-12-12", "sdmc:/roms/psp/rr.iso"},
        {"Panzer Dragoon Saga", "Saturn 独立核心", "1998-01-29", "sdmc:/roms/ss/pds.bin"},
    };
}

const char* PlatformLabel() {
#if defined(GUI_DEV_PLATFORM_switch)
    return "Nintendo Switch";
#elif defined(GUI_DEV_PLATFORM_mac)
    return "macOS";
#else
    return "Desktop";
#endif
}

} // namespace

LibraryScene::LibraryScene(std::vector<GameEntry> games, std::string version)
    : games_(std::move(games)), version_(std::move(version)) {}

void LibraryScene::OnRender(UiContext& ui) {
    ui.RefreshIfDisplayChanged();

    // 页头副标题：平台 / 后端 / 分辨率 / 版本，全部来自统一上下文，
    // 各核心不需要自己拼字符串。
    int w = 0;
    int h = 0;
    ui.GetBackend().GetDrawableSize(w, h);
    const std::string subtitle = std::string("platform=") + PlatformLabel() + "  backend=" + PlatformName() +
                                 "  display=" + std::to_string(w) + "x" + std::to_string(h) + "  v" + version_;

    if (!Components::BeginPanel(ui, "GBAStation · 统一前端组件", subtitle.c_str())) {
        Components::EndPanel();
        return;
    }

    // ---- 游戏库列表 --------------------------------------------------------
    Components::SectionHeader(ui, "游戏库");

    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(games_.size());
    for (const GameEntry& g : games_) {
        rows.emplace_back(g.title, g.core + "   " + g.release);
    }

    if (rows.empty()) {
        Components::EmptyState(ui, "未找到游戏", "把 ROM 放到 sdmc:/roms 后重新扫描");
    } else {
        const int clicked = Components::SelectableList(ui, "##library", rows, selected_);
        if (clicked >= 0) {
            selected_ = clicked;
        }
        // 手柄语义：A 按下时表示启动当前项。
        if (ui.Pad().Pressed(InputAction::Confirm)) {
            Components::StatusBanner(ui, ("启动：" + games_[selected_].title).c_str(), false);
        }
    }

    // ---- 当前选中项详情 ----------------------------------------------------
    Components::SectionHeader(ui, "详情");
    if (selected_ >= 0 && selected_ < static_cast<int>(games_.size())) {
        const GameEntry& g = games_[static_cast<std::size_t>(selected_)];
        Components::LabeledRow(ui, "标题", g.title.c_str());
        Components::LabeledRow(ui, "核心", g.core.c_str());
        Components::LabeledRow(ui, "发售日", g.release.c_str());
        Components::LabeledRow(ui, "路径", g.path.c_str());
    }

    // ---- 响应式：窄屏（Switch 手持）单列，宽屏双列 -------------------------
    if (ui.Compact()) {
        Components::SectionHeader(ui, "选项");
        bool fast = true;
        Components::ToggleRow(ui, "快速启动", &fast, "(手柄 A 直接进入)");
        Components::ProgressRow(ui, "资源更新", 0.42f, "checking...");
    } else {
        if (ImGui::BeginTable("##options", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            Components::SectionHeader(ui, "选项");
            bool fast = true;
            Components::ToggleRow(ui, "快速启动", &fast, nullptr);
            ImGui::TableSetColumnIndex(1);
            Components::SectionHeader(ui, "长任务");
            Components::ProgressRow(ui, "资源更新", 0.42f, nullptr);
            ImGui::EndTable();
        }
    }

    Components::EndPanel();

    // 页脚必须在内容区结束后提交。
    Components::Footer(ui, {{"A", "启动"}, {"B", "返回"}, {"L/R", "翻页"}, {"+", "菜单"}});
}

void DemoApp::Configure(BackendConfig& cfg, PlatformKind kind) const {
    (void)kind;
    cfg.title = "GBAStation · GUI_DEV";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.vsync = true;
    cfg.resizable = true;
#if defined(GUI_DEV_PLATFORM_switch)
    cfg.vsync = false; // Switch 由 libnx 垂直同步，SDL 再同步会拖帧
#endif
}

void DemoApp::OnStart(UiContext& ui) {
    (void)ui;
    Scenes().Reset(std::make_unique<LibraryScene>(SampleLibrary(), version_));
}

} // namespace gui_dev::demo
