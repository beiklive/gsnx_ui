#include "demo/DemoApp.h"

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "ui/Icons.h"
#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::demo {
namespace {

struct Card {
    const char* title;
    const char* subtitle;
    Icons::Material icon;
};

constexpr Card kCards[] = {
    {"游戏库", "扫描并启动 ROM", Icons::Material::SportsEsports},
    {"存档", "备份与恢复存档", Icons::Material::Save},
    {"设置", "按键映射 / 画面", Icons::Material::Settings},
    {"更新", "核心与资源更新", Icons::Material::Update},
    {"金手指", "Cheat 代码管理", Icons::Material::Memory},
    {"截图", "查看与导出截图", Icons::Material::PhotoLibrary},
    {"关于", "版本与许可", Icons::Material::HelpOutline},
    {"退出", "返回 HOS", Icons::Material::Close},
};

constexpr int kCardCount = static_cast<int>(sizeof(kCards) / sizeof(kCards[0]));

std::string PlatformLabel() {
#if defined(GUI_DEV_PLATFORM_switch)
    return "Nintendo Switch";
#elif defined(GUI_DEV_PLATFORM_mac)
    return "macOS";
#else
    return "Desktop";
#endif
}

} // namespace

HomeScene::HomeScene(TextureRef flow_texture, std::string version)
    : flow_texture_(std::move(flow_texture)), version_(std::move(version)) {}

void HomeScene::FocusMove(int delta) {
    focus_ = (focus_ + delta) % kCardCount;
    if (focus_ < 0) {
        focus_ += kCardCount;
    }
}

void HomeScene::OnUpdate(UiContext& ui, float dt) {
    (void)dt;
    const PadState& pad = ui.Pad();

    if (pad.Pressed(InputAction::Left)) {
        FocusMove(-1);
    }
    if (pad.Pressed(InputAction::Right)) {
        FocusMove(1);
    }
    if (pad.Pressed(InputAction::Up)) {
        FocusMove(-columns_);
    }
    if (pad.Pressed(InputAction::Down)) {
        FocusMove(columns_);
    }
    if (pad.Pressed(InputAction::Confirm)) {
        launched_ = kCards[focus_].title;
    }
}

void HomeScene::OnRender(UiContext& ui) {
    int display_w = 0;
    int display_h = 0;
    ui.GetBackend().GetDrawableSize(display_w, display_h);

    const std::string subtitle = std::string(PlatformLabel()) + "  ·  " + std::to_string(display_w) + "x" +
                                 std::to_string(display_h) + "  ·  v" + version_;
    if (!Components::BeginPanel(ui, "GUI_DEV · 组件预览", subtitle.c_str())) {
        Components::EndPanel(ui);
        return;
    }

    columns_ = ui.Compact() ? 2 : 4;

    Components::SectionHeader(ui, "可聚焦 Box");
    if (launched_.empty()) {
        ui.TextDisabled("方向键移动焦点，A / 单击激活");
    } else {
        ui.Text("已激活：%s", launched_.c_str());
    }
    ui.Spacing();

    Components::BoxStyle style;
    style.flow_texture = flow_texture_.ImGuiRef();
    style.height = 128.0f;

    if (ImGui::BeginTable("##cards", columns_, ImGuiTableFlags_SizingStretchSame)) {
        for (int i = 0; i < kCardCount; ++i) {
            ImGui::TableNextColumn();

            ImGui::PushID(i);
            const Components::BoxResult result =
                Components::FocusableBox("card", focus_ == i, style, [&](const ImVec2& content) {
                    (void)content;
                    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.5f);
                    ImGui::TextColored(Theme::ToVec4(Theme::kAccent), "%s", Icons::Glyph(kCards[i].icon));
                    ImGui::PopFont();
                    ImGui::Spacing();
                    ImGui::TextUnformatted(kCards[i].title);
                    ImGui::TextDisabled("%s", kCards[i].subtitle);
                });
            ImGui::PopID();

            // 鼠标悬停即接管焦点，和手柄共用同一个焦点索引。
            if (result.hovered) {
                focus_ = i;
            }
            if (result.clicked) {
                launched_ = kCards[i].title;
            }
        }
        ImGui::EndTable();
    }

    Components::EndPanel(ui, {
        {Icons::Glyph(Icons::Button::A), "确定"},
        {Icons::Glyph(Icons::Button::B), "返回"},
        {Icons::Glyph(Icons::Button::L), "上一项"},
        {Icons::Glyph(Icons::Button::R), "下一项"},
    });
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

void DemoApp::OnFrame(UiContext& ui, float dt) {
    (void)dt;
    // GUI_DEV_EXIT_AFTER=<帧数>：跑满帧数后走正常退出流程。
    // 用来在 CI/脚本里验证「退出路径」不崩（timeout 杀进程是走不到析构的）。
    static const int exit_after = [] {
        const char* value = std::getenv("GUI_DEV_EXIT_AFTER");
        return value != nullptr ? std::atoi(value) : 0;
    }();
    if (exit_after > 0 && ++frame_ >= exit_after) {
        ui.GetBackend().RequestQuit();
    }
}

void DemoApp::OnStart(UiContext& ui) {
    // 流光贴图全局只有一份，用 TextureRef 管生命周期。
    TextureRef flow(ui.GetBackend(), "img/border_gradient.png");
    if (!flow.Valid()) {
        std::fprintf(stderr, "[gui_dev] border_gradient.png 加载失败，聚焦边框退化为纯色\n");
    }
    Scenes().Reset(std::make_unique<HomeScene>(std::move(flow), version_));
}

} // namespace gui_dev::demo
