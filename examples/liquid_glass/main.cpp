// 液态玻璃（Liquid Glass）演示：底部玻璃 Tab 条 + 文字/图片背景。
//
// 演示的链路（对应 Apple 那套「抓背景 → 模糊 → 玻璃材质 → 高光/边缘 → 内容」）：
//   背景图 + 底部文字/图片  →  GlassTabs 采样背景纹理做 13 抽头模糊 + 边缘 UV 折射
//                          →  染色/提亮/顶部高光/镜面光斑/内亮边  →  Tab 内容
//
// 操作：
//   拖动玻璃条（鼠标 / 触摸）—— 换位置看不同背景被模糊+折射的样子
//   ← / →（或 L / R）切 Tab，也可以直接点某个 Tab
//   GUI_DEV_WINDOW=WxH / GUI_DEV_ZOOM=x / GUI_DEV_EXIT_AFTER=n 支持（和其他 demo 一致）
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/components/Box.h"
#include "component_view/components/GlassTabs.h"
#include "component_view/pages/Page.h"
#include "core/App.h"
#include "ui/Icons.h"
#include "ui/Scene.h"
#include "ui/Texture.h"
#include "ui/UiContext.h"

namespace {

namespace Icons = gui_dev::Icons;
namespace Global = gui_dev::cv::Global;
namespace Theme = gui_dev::cv::Theme;
using gui_dev::cv::Box;
namespace Draw = gui_dev::cv::Draw;
using gui_dev::cv::GlassTabs;
using gui_dev::cv::Page;
using gui_dev::cv::Rect;
using gui_dev::TextureRef;

// 背景层：一张铺满画布的图片 + 底部的一张卡片图 + 底部菜单文字。
// 这些内容都是"玻璃背后的东西"，玻璃条会采样它们（图片走纹理采样，文字被玻璃的模糊层压住）。
class BackdropLayer : public Box {
public:
    BackdropLayer(TextureRef& photo, TextureRef& card) : Box("backdrop"), photo_(photo), card_(card) {}

protected:
    void OnDrawContent(ImDrawList* dl, const Rect& content) override {
        if (dl == nullptr) {
            return;
        }
        // 背景图铺满画布（cover）：可能超出画布，超出的部分靠玻璃采样时按同一矩形做 UV 映射
        if (photo_.Valid()) {
            const ImVec2 size = photo_.Size();
            const float scale = gui_dev::cv::Maxf(content.Width() / size.x, content.Height() / size.y);
            const ImVec2 drawn(size.x * scale, size.y * scale);
            photo_rect_ = Rect::FromPosSize(ImVec2(content.Center().x - drawn.x * 0.5f,
                                                   content.Center().y - drawn.y * 0.5f),
                                           drawn);
            dl->AddImage(photo_.ImGuiRef(), photo_rect_.min, photo_rect_.max);
        }

        // 底部卡片图（会被玻璃条盖住一部分 —— 玻璃里看到的就是它的模糊+折射版本）
        const Rect card = Rect::FromPosSize(ImVec2(56.0f, 486.0f), ImVec2(300.0f, 184.0f));
        if (card_.Valid()) {
            dl->AddImage(card_.ImGuiRef(), card.min, card.max);
        } else {
            Draw::RoundedRectFilled(dl, card, Theme::U32(Theme::kAccent, 0.6f), 12.0f, 12.0f, 12.0f, 12.0f);
        }

        // 底部菜单文字：第一行在玻璃条上方（清晰），后两行在玻璃后面（被模糊/压暗）
        DrawTextBlock(dl, ImVec2(392.0f, 470.0f));
    }

private:
    void DrawTextBlock(ImDrawList* dl, const ImVec2& origin) const {
        const char* left[3] = {"返回游戏", "保存状态", "读取状态"};
        const char* right[3] = {"独立设置", "全局设置", "重置"};
        for (int i = 0; i < 3; ++i) {
            const float y = origin.y + static_cast<float>(i) * 40.0f;
            Draw::Text(dl, nullptr, Theme::kFontHeader, ImVec2(origin.x, y), Theme::U32(Theme::kWhite, 0.96f), left[i]);
            Draw::Text(dl, nullptr, Theme::kFontHeader, ImVec2(origin.x + 220.0f, y),
                       Theme::U32(Theme::kWhite, 0.96f), right[i]);
        }
    }

public:
    // 背景图实际画出来的矩形（铺满时可能超出画布），给玻璃条做 UV 映射用
    const Rect& photoRect() const { return photo_rect_; }

private:
    TextureRef& photo_;
    TextureRef& card_;
    Rect photo_rect_{};
};

class GlassPage : public Page {
public:
    explicit GlassPage(TextureRef& photo, TextureRef& card) : photo_(photo), card_(card) {}

    const char* Title() const override { return "liquid_glass"; }

    void OnBuild() override {
        backdrop_ = Root().Emplace<BackdropLayer>(photo_, card_);
        backdrop_->SetName("backdrop");

        tabs_ = Root().Emplace<GlassTabs>();
        tabs_->SetName("glass_tabs");
        tabs_->SetZOrder(50); // 浮在背景之上
        tabs_->setItems({
            {Icons::Glyph(Icons::Material::VideogameAsset), "游戏"},
            {Icons::Glyph(Icons::Material::Save), "存档"},
            {Icons::Glyph(Icons::Material::Settings), "设置"},
            {Icons::Glyph(Icons::Material::Info), "关于"},
        });
        tabs_->size = ImVec2(kBarWidth, kBarHeight);
        tabs_->position = ImVec2((1280.0f - kBarWidth) * 0.5f, 540.0f);
        // 玻璃条一开始会自己找背景纹理（OnUpdate 里每帧刷新，画布尺寸变了也能跟上）
        connect(tabs_, &GlassTabs::selectionChanged, this, [this](int index) {
            std::printf("[glass_tabs] 选中 %d (%s)\n", index, tabs_->currentText());
            std::fflush(stdout);
        });
    }

    void OnUpdate(float dt) override {
        (void)dt;
        backdrop_->position = ImVec2(0.0f, 0.0f);
        backdrop_->size = Global::canvas_size;

        // 画布尺寸变化时把玻璃条按比例摆好（拖动过的话保持不动）
        if (!placed_ || last_canvas_.x != Global::canvas_size.x || last_canvas_.y != Global::canvas_size.y) {
            placed_ = true;
            last_canvas_ = Global::canvas_size;
            if (!tabs_->isDragging()) {
                tabs_->position = ImVec2((Global::canvas_size.x - kBarWidth) * 0.5f,
                                         Global::canvas_size.y - kBarHeight - 76.0f);
                tabs_->size = ImVec2(kBarWidth, kBarHeight);
            }
        }
        // 玻璃条背后的那张纹理（背景图）——这就是"抓背景"这一步
        tabs_->setBackdrop(photo_.ImGuiRef(), backdrop_->photoRect());
    }

    void OnOverlay(ImDrawList* dl) override {
        if (dl == nullptr) {
            return;
        }
        // 顶部说明（清晰，用来和玻璃里的模糊/折射做对照）
        Draw::Text(dl, nullptr, Theme::kFontTitle, ImVec2(40.0f, 34.0f), Theme::U32(Theme::kWhite, 0.98f),
                   "液态玻璃 TAB 条（多重采样模糊 + 边缘 UV 折射）");
        Draw::Text(dl, nullptr, Theme::kFontBody, ImVec2(40.0f, 72.0f), Theme::U32(Theme::kWhite, 0.82f),
                   "拖动玻璃条看不同背景；← / → 或直接点 Tab 切换；看边缘的折射与高光");
        Draw::Text(dl, nullptr, Theme::kFontBody, ImVec2(40.0f, 98.0f), Theme::U32(Theme::kWhite, 0.72f),
                   "底部文字/图片就是玻璃背后的内容：第一行清晰，后两行被玻璃模糊");
    }

private:
    static constexpr float kBarWidth = 760.0f;
    static constexpr float kBarHeight = 104.0f;

    TextureRef& photo_;
    TextureRef& card_;
    BackdropLayer* backdrop_ = nullptr;
    GlassTabs* tabs_ = nullptr;
    ImVec2 last_canvas_{0.0f, 0.0f};
    bool placed_ = false;
};

class GlassScene : public gui_dev::Scene {
public:
    const char* Name() const override { return "liquid_glass"; }
    void OnRender(gui_dev::UiContext& ui) override { (void)ui; }
};

class GlassApp : public gui_dev::App {
public:
    void Configure(gui_dev::BackendConfig& cfg, gui_dev::PlatformKind kind) const override {
        (void)kind;
        cfg.title = "GUI_DEV · liquid glass tabs";
        cfg.width = 1280;
        cfg.height = 720;
        cfg.vsync = true;
        cfg.resizable = true;
        if (const char* size = std::getenv("GUI_DEV_WINDOW")) {
            int width = 0;
            int height = 0;
            if (std::sscanf(size, "%dx%d", &width, &height) == 2 && width > 0 && height > 0) {
                cfg.width = width;
                cfg.height = height;
            }
        }
        if (std::getenv("GUI_DEV_NO_VSYNC") != nullptr) {
            cfg.vsync = false;
        }
        if (const char* fps = std::getenv("GUI_DEV_MAX_FPS")) {
            cfg.max_fps = std::atoi(fps);
        }
#if defined(GUI_DEV_PLATFORM_switch)
        cfg.vsync = false; // Switch 由 libnx 垂直同步
        if (cfg.max_fps <= 0) {
            cfg.max_fps = 60;
        }
#endif
    }

    void OnStart(gui_dev::UiContext& ui) override {
        // 这个 demo 的默认缩放：1.2（手持上看得清），可用 GUI_DEV_ZOOM 覆盖
        float zoom = 1.2f;
        if (const char* value = std::getenv("GUI_DEV_ZOOM")) {
            const float parsed = static_cast<float>(std::atof(value));
            if (parsed > 0.0f) {
                zoom = parsed;
            }
        }
        ui.SetUiZoom(zoom);
        Theme::SetMode(Theme::ThemeMode::Dark); // 深色主题下玻璃更好看
        Theme::ApplyToImGui();
        Global::ApplyTheme();

        photo_ = TextureRef(ui.GetBackend(), "img/glass_backdrop.png");
        card_ = TextureRef(ui.GetBackend(), "img/glass_card.png");
        if (!photo_.Valid()) {
            std::fprintf(stderr, "[liquid_glass] 背景图没加载上，演示效果会打折\n");
        }

        Scenes().Reset(std::make_unique<GlassScene>());
        page_ = std::make_unique<GlassPage>(photo_, card_);
        page_->Bind(ui);
        if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
            exit_after_ = std::atoi(value);
        }
    }

    void OnFrame(gui_dev::UiContext& ui, float dt) override {
        Global::BeginFrame(ui);
        page_->Update(dt);
        page_->Render();
        Global::EndFrame();
        if (exit_after_ > 0 && ++frame_ >= exit_after_) {
            ui.GetBackend().RequestQuit();
        }
    }

    void OnShutdown(gui_dev::UiContext& ui) override {
        (void)ui;
        page_.reset();
        card_.Reset(); // 纹理要在后端关闭前释放
        photo_.Reset();
    }

private:
    std::unique_ptr<GlassPage> page_;
    TextureRef photo_;
    TextureRef card_;
    int frame_ = 0;
    int exit_after_ = 0;
};

} // namespace

int main(int, char**) {
    GlassApp app;
    return gui_dev::AppRunner(app).Run();
}
