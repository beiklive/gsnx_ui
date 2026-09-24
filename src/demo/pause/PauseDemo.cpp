#include "demo/pause/PauseDemo.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <utility>

#include <imgui.h>

#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

namespace gui_dev::demo {
namespace {

using gamemenu::AddDiamond;
using gamemenu::AddSkewFilled;
using gamemenu::ColorWithAlpha;
using gamemenu::MakeRect;
using gamemenu::Rect;

constexpr float kPi = 3.14159265358979323846f;

// 底部 HUD 条高度：假游戏用
constexpr float kHudHeight = 54.0f;

void FormatPlayTime(char* buffer, std::size_t size, float seconds) {
    const int total = static_cast<int>(seconds);
    std::snprintf(buffer, size, "%02d:%02d:%02d", total / 3600, (total / 60) % 60, total % 60);
}

// 用固定字符串避免依赖系统时钟（Switch 上不一定初始化过时间服务）
void FormatStamp(char* buffer, std::size_t size, int save_index) {
    const std::time_t now = std::time(nullptr);
    if (now > 1000000000) {
        std::tm tm_buf{};
#if defined(_WIN32)
        localtime_s(&tm_buf, &now);
#else
        localtime_r(&now, &tm_buf);
#endif
        std::strftime(buffer, size, "%Y-%m-%d %H:%M", &tm_buf);
        return;
    }
    std::snprintf(buffer, size, "存档 #%d", save_index + 1);
}

// 游戏画面始终占满整屏（真实模拟器也是如此），菜单只是叠在上面；
// 这里只把 HUD 条让出来。玩家方块放在偏右，保证菜单打开时仍然可见。
gamemenu::Rect GameRegion() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    return MakeRect(0.0f, 0.0f, display.x, display.y - kHudHeight);
}

} // namespace

// ------------------------------------------------------------------ 场景 ----

PauseScene::PauseScene() {
    // 每次打开菜单都新建根视图：动画状态从零开始（需求 §28）
    menu_.SetRootViewFactory([this] { return std::make_unique<gamemenu::MainMenuView>(*this); });

    // 造几个"已有存档"用来展示槽位状态
    const int filled[kSlotCount] = {1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0};
    const char* playtimes[kSlotCount] = {"01:23:45", "00:12:08", "", "", "12:45:02", "", "",
                                         "00:03:31", "", "", "", ""};
    for (int i = 0; i < kSlotCount; ++i) {
        slots_[i].exists = filled[i] != 0;
        if (slots_[i].exists) {
            std::snprintf(slots_[i].time_text, sizeof(slots_[i].time_text), "2026-09-%02d 1%d:2%d",
                          10 + (i % 12), i % 10, i % 6);
            std::snprintf(slots_[i].play_text, sizeof(slots_[i].play_text), "%s", playtimes[i]);
        }
    }
}

void PauseScene::SetToast(const char* text) {
    std::snprintf(toast_, sizeof(toast_), "%s", text != nullptr ? text : "");
    toast_timer_ = 2.2f;
}

void PauseScene::OnUpdate(UiContext& ui, float dt) {
    // 菜单吃输入（含 ZL+ZR 热键）；暂停时游戏时间不推进
    menu_.Update(dt, ui.Pad());

    if (!menu_.Visible()) {
        game_.time += dt;
        game_.play_seconds += dt;
        game_.ship_angle += dt * 1.4f;
        // 小球只在右侧游戏区域内弹跳（左侧是菜单面板）
        const gamemenu::Rect region = GameRegion();
        const float limit_min_x = region.min.x + 36.0f;
        const float limit_max_x = region.max.x - 36.0f;
        const float limit_min_y = 36.0f;
        const float limit_max_y = region.max.y - 30.0f;
        for (int i = 0; i < 3; ++i) {
            game_.balls[i].x += game_.ball_vel[i].x * dt;
            game_.balls[i].y += game_.ball_vel[i].y * dt;
            if (game_.balls[i].x < limit_min_x || game_.balls[i].x > limit_max_x) {
                game_.ball_vel[i].x = -game_.ball_vel[i].x;
                game_.balls[i].x =
                    game_.balls[i].x < limit_min_x ? limit_min_x : limit_max_x;
                game_.score += 10;
            }
            if (game_.balls[i].y < limit_min_y || game_.balls[i].y > limit_max_y) {
                game_.ball_vel[i].y = -game_.ball_vel[i].y;
                game_.balls[i].y =
                    game_.balls[i].y < limit_min_y ? limit_min_y : limit_max_y;
                game_.score += 10;
            }
        }
    }

    if (toast_timer_ > 0.0f) {
        toast_timer_ -= dt;
        if (toast_timer_ < 0.0f) {
            toast_timer_ = 0.0f;
        }
    }
}

void PauseScene::OnRender(UiContext& ui) {
    Components::BeginCanvas(ui, "pause_demo");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const Rect screen = MakeRect(0.0f, 0.0f, display.x, display.y);

    DrawFakeGame(draw_list, screen);
    DrawHud(draw_list, screen);
    menu_.Draw(draw_list, screen);
    DrawHotkeyHints(draw_list, screen);

    Components::EndCanvas();
}

void PauseScene::DrawFakeGame(ImDrawList* draw_list, const Rect& screen) {
    const float w = screen.Width();
    const float h = screen.Height() - kHudHeight;
    (void)w;

    // 背景：深底 + 顶部微亮（不用模糊/离屏，纯填充）
    draw_list->AddRectFilledMultiColor(ImVec2(0.0f, 0.0f), ImVec2(screen.max.x, h),
                                       IM_COL32(0x16, 0x1C, 0x2A, 0xFF), IM_COL32(0x10, 0x14, 0x1E, 0xFF),
                                       IM_COL32(0x08, 0x0A, 0x10, 0xFF), IM_COL32(0x0C, 0x10, 0x18, 0xFF));

    // 斜向流动条纹（假游戏的"速度感"）
    const float scroll = std::fmod(game_.time * 90.0f, 90.0f);
    for (int i = -8; i < 22; ++i) {
        const float x = static_cast<float>(i) * 90.0f + scroll;
        Rect stripe = MakeRect(x, 0.0f, 26.0f, h);
        AddSkewFilled(draw_list, stripe, 60.0f, IM_COL32(0xFF, 0xFF, 0xFF, 0x08));
    }

    // 网格
    for (int i = 0; i <= 12; ++i) {
        const float x = static_cast<float>(i) * 110.0f;
        draw_list->AddLine(ImVec2(x, 0.0f), ImVec2(x - 40.0f, h), IM_COL32(0x4F, 0xA3, 0xFF, 0x12));
    }
    for (int i = 0; i <= 7; ++i) {
        const float y = static_cast<float>(i) * 90.0f;
        draw_list->AddLine(ImVec2(0.0f, y), ImVec2(screen.max.x, y), IM_COL32(0x4F, 0xA3, 0xFF, 0x0E));
    }

    // "玩家"：旋转的红色方框（偏右，避免被左侧菜单面板盖住）
    const gamemenu::Rect region = GameRegion();
    const ImVec2 center(region.max.x * 0.62f, region.max.y * 0.42f);
    const float size = 46.0f;
    ImVec2 corners[4];
    for (int i = 0; i < 4; ++i) {
        const float angle = game_.ship_angle + kPi * 0.5f * static_cast<float>(i) + kPi * 0.25f;
        corners[i] = ImVec2(center.x + std::cos(angle) * size, center.y + std::sin(angle) * size);
    }
    draw_list->AddConvexPolyFilled(corners, 4, IM_COL32(0xE2, 0x1B, 0x25, 0xE0));
    draw_list->AddPolyline(corners, 4, IM_COL32(0xFF, 0xFF, 0xFF, 0xB0), ImDrawFlags_Closed, 2.0f);

    // 弹跳菱形
    for (int i = 0; i < 3; ++i) {
        const ImU32 col = i == 0 ? IM_COL32(0xFF, 0xFF, 0xFF, 0xD0)
                                 : (i == 1 ? IM_COL32(0xE2, 0x1B, 0x25, 0xC0)
                                           : IM_COL32(0x9A, 0xA1, 0xAC, 0xA0));
        AddDiamond(draw_list, game_.balls[i], 16.0f + static_cast<float>(i) * 3.0f, col);
    }
}

void PauseScene::DrawHud(ImDrawList* draw_list, const Rect& screen) {
    const Rect bar = MakeRect(0.0f, screen.max.y - kHudHeight, screen.Width(), kHudHeight);
    draw_list->AddRectFilled(bar.min, bar.max, IM_COL32(0x0A, 0x0B, 0x0F, 0xF0));
    AddSkewFilled(draw_list, MakeRect(bar.max.x - 340.0f, bar.min.y, 340.0f, 4.0f), 30.0f,
                  IM_COL32(0xE2, 0x1B, 0x25, 0xFF));

    char play[16];
    FormatPlayTime(play, sizeof(play), game_.play_seconds);
    char score[32];
    std::snprintf(score, sizeof(score), "SCORE %06d", game_.score);

    char line[128];
    const bool paused = menu_.Visible();
    std::snprintf(line, sizeof(line), "存档 %d   %s   %s   %s", save_count_, score, play,
                  paused ? "|| PAUSED" : ">  RUNNING");

    // 面板在左侧，HUD 右对齐避免被盖住
    const float size = 20.0f;
    gamemenu::AddTextRight(draw_list, ImVec2(bar.max.x - 28.0f, bar.Center().y - size * 0.5f),
                           paused ? IM_COL32(0xE2, 0x1B, 0x25, 0xFF)
                                  : IM_COL32(0xF5, 0xF5, 0xF7, 0xE0),
                           size, line);
}

void PauseScene::DrawHotkeyHints(ImDrawList* draw_list, const Rect& screen) {
    const float y = screen.max.y - kHudHeight - 22.0f;
    const char* hint =
        "ZL+ZR (Z+C) 菜单   ↑↓←→ 移动   A (Enter) 确认   B (Esc) 返回   L/R (Q/E) 翻页   "
        "+ (Tab) 直接返回游戏";
    const float right = screen.max.x - 28.0f;
    gamemenu::AddTextRight(draw_list, ImVec2(right, y - 8.0f), IM_COL32(0xB0, 0xB6, 0xC0, 0xB0),
                           15.0f, hint);

    if (toast_timer_ > 0.0f) {
        const float a = gamemenu::Clamp01(toast_timer_ / 0.7f);
        gamemenu::AddTextRight(draw_list, ImVec2(right, y - 34.0f),
                               ColorWithAlpha(IM_COL32(0xE2, 0x1B, 0x25, 0xFF), a), 18.0f, toast_);
    }

    if (loaded_slot_ >= 0) {
        char text[64];
        std::snprintf(text, sizeof(text), "已读取槽位 %d", loaded_slot_ + 1);
        gamemenu::AddTextRight(draw_list, ImVec2(right, y - 56.0f),
                               IM_COL32(0xF5, 0xF5, 0xF7, 0xB0), 15.0f, text);
    }
}

// ------------------------------------------------------- MainMenuDelegate ----

void PauseScene::OnResume(gamemenu::GameMenuContext& ctx) {
    (void)ctx;
    SetToast("返回游戏");
}

void PauseScene::OnOpenStateSlots(gamemenu::GameMenuContext& ctx, bool saving) {
    ctx.host->RequestPush(std::make_unique<gamemenu::StateSlotView>(*this, saving));
}

void PauseScene::OnOpenSettings(gamemenu::GameMenuContext& ctx, bool per_game) {
    ctx.host->RequestPush(std::make_unique<gamemenu::SettingsView>(*this, per_game));
}

void PauseScene::OnRequestReset(gamemenu::GameMenuContext& ctx) {
    pending_dialog_ = 1;
    dialog_lines_[0] = "当前游戏将重新启动。";
    dialog_lines_[1] = "未保存的进度可能会丢失。";
    dialog_buttons_[0] = {"取消", false};
    dialog_buttons_[1] = {"重置", true};
    ctx.host->RequestPush(std::make_unique<gamemenu::DialogView>(
        "重置游戏？", dialog_lines_, 2, dialog_buttons_, 2, 0, *this));
}

void PauseScene::OnRequestExit(gamemenu::GameMenuContext& ctx) {
    // 未保存状态 -> 三选一；已保存 -> 二选一（需求 §23）
    const bool unsaved = save_count_ == 0;
    pending_dialog_ = unsaved ? 2 : 3;
    if (unsaved) {
        dialog_lines_[0] = "检测到当前游戏状态可能尚未保存。";
        dialog_buttons_[0] = {"保存并退出", false};
        dialog_buttons_[1] = {"直接退出", true};
        dialog_buttons_[2] = {"取消", false};
        ctx.host->RequestPush(std::make_unique<gamemenu::DialogView>(
            "退出游戏？", dialog_lines_, 1, dialog_buttons_, 3, 2, *this));
    } else {
        dialog_lines_[0] = "确定退出当前游戏？";
        dialog_buttons_[0] = {"退出", true};
        dialog_buttons_[1] = {"取消", false};
        ctx.host->RequestPush(std::make_unique<gamemenu::DialogView>(
            "退出游戏？", dialog_lines_, 1, dialog_buttons_, 2, 1, *this));
    }
}

// ------------------------------------------------------ StateSlotDelegate ----

void PauseScene::FillSlot(int slot, gamemenu::GameMenuSlotData& out) const {
    if (slot < 0 || slot >= kSlotCount) {
        out = gamemenu::GameMenuSlotData{};
        return;
    }
    out.exists = slots_[slot].exists;
    out.time_text = slots_[slot].time_text;
    out.play_text = slots_[slot].play_text;
}

bool PauseScene::SaveState(int slot) {
    if (slot < 0 || slot >= kSlotCount) {
        return false;
    }
    MockSlot& target = slots_[slot];
    target.exists = true;
    FormatStamp(target.time_text, sizeof(target.time_text), slot);
    FormatPlayTime(target.play_text, sizeof(target.play_text), game_.play_seconds);
    ++save_count_;
    char message[64];
    std::snprintf(message, sizeof(message), "（Mock）已保存到槽位 %d", slot + 1);
    SetToast(message);
    return true;
}

bool PauseScene::LoadState(int slot) {
    if (slot < 0 || slot >= kSlotCount || !slots_[slot].exists) {
        return false;
    }
    loaded_slot_ = slot;
    char message[64];
    std::snprintf(message, sizeof(message), "（Mock）已读取槽位 %d", slot + 1);
    SetToast(message);
    return true;
}

// ------------------------------------------------------ SettingsDelegate ----

void PauseScene::OnSettingChanged(bool per_game, const char* category, const char* option, int value) {
    char message[96];
    std::snprintf(message, sizeof(message), "（Mock）%s / %s / %s = %d",
                  per_game ? "独立设置" : "全局设置", category, option, value);
    SetToast(message);
}

void PauseScene::OnSettingCommand(bool per_game, const char* category, const char* option) {
    char message[96];
    std::snprintf(message, sizeof(message), "（Mock）%s / %s / 打开子页 %s",
                  per_game ? "独立设置" : "全局设置", category, option);
    SetToast(message);
}

// -------------------------------------------------------- DialogDelegate ----

bool PauseScene::OnDialogResult(gamemenu::GameMenuContext& ctx, int button_index) {
    (void)ctx;
    switch (pending_dialog_) {
    case 1: // 重置
        if (button_index == 1) {
            game_.score = 0;
            game_.play_seconds = 0.0f;
            SetToast("（Mock）重置游戏");
        } else {
            SetToast("已取消");
        }
        break;
    case 2: // 退出（有未保存状态）
        if (button_index == 0) {
            SaveState(0);
            SetToast("（Mock）保存并退出");
        } else if (button_index == 1) {
            SetToast("（Mock）直接退出游戏");
        } else {
            SetToast("已取消");
        }
        break;
    case 3: // 退出（已保存）
        SetToast(button_index == 0 ? "（Mock）退出游戏" : "已取消");
        break;
    default:
        break;
    }
    pending_dialog_ = 0;
    if (button_index != 2 && pending_dialog_ == 0 && toast_timer_ > 0.0f) {
        // 退出/重置只是 Mock：真实实现在这里返回启动器或重启核心
        menu_.CloseMenu();
    }
    return true;
}

// --------------------------------------------------------------- App --------

void PauseDemoApp::Configure(BackendConfig& cfg, PlatformKind kind) const {
    (void)kind;
    cfg.title = "GBAStation · 暂停菜单 Demo";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.vsync = true;
    cfg.resizable = true;
#if defined(GUI_DEV_PLATFORM_switch)
    cfg.vsync = false;
#endif
    // 验证自适应用：GUI_DEV_WINDOW=1600x720 / 960x720 / 1920x1080 ...
    if (const char* window = std::getenv("GUI_DEV_WINDOW")) {
        int w = 0;
        int h = 0;
        if (std::sscanf(window, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            cfg.width = w;
            cfg.height = h;
        }
    }
}

void PauseDemoApp::OnStart(UiContext& ui) {
    (void)ui;
    Scenes().Reset(std::make_unique<PauseScene>());
}

void PauseDemoApp::OnFrame(UiContext& ui, float dt) {
    (void)dt;
    // 冒烟测试：GUI_DEV_EXIT_AFTER=<帧数> 跑满后走正常退出（验证退出路径不崩）
    static const int exit_after = [] {
        const char* value = std::getenv("GUI_DEV_EXIT_AFTER");
        return value != nullptr ? std::atoi(value) : 0;
    }();
    if (exit_after > 0 && ++frame_ >= exit_after) {
        ui.GetBackend().RequestQuit();
    }
}

} // namespace gui_dev::demo
