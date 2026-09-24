// 暂停菜单 Demo：假游戏画面 + Persona 风格动态暂停菜单。
//
// 目的只是展示 UI 效果与交互手感：所有存档/设置动作都是 Mock，
// 不接任何模拟核心（需求：先做 UI，不做核心接入）。
#pragma once

#include <cstdint>
#include <memory>

#include "core/App.h"
#include "gamemenu/GameMenuView.h"
#include "gamemenu/views/DialogView.h"
#include "gamemenu/views/MainMenuView.h"
#include "gamemenu/views/SettingsView.h"
#include "gamemenu/views/StateSlotView.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace gui_dev::demo {

// 假的"游戏画面"状态：菜单打开时冻结，用来演示暂停/恢复。
struct FakeGameState {
    float time = 0.0f;      // 仅在未暂停时推进
    float play_seconds = 0.0f;
    float ship_angle = 0.0f;
    ImVec2 balls[3] = {ImVec2(220.0f, 180.0f), ImVec2(520.0f, 300.0f), ImVec2(360.0f, 120.0f)};
    ImVec2 ball_vel[3] = {ImVec2(150.0f, 96.0f), ImVec2(-120.0f, 140.0f), ImVec2(90.0f, -130.0f)};
    int score = 12450;
};

struct MockSlot {
    bool exists = false;
    char time_text[24] = {};
    char play_text[16] = {};
};

class PauseScene final : public Scene,
                         public gamemenu::MainMenuDelegate,
                         public gamemenu::StateSlotDelegate,
                         public gamemenu::SettingsDelegate,
                         public gamemenu::DialogDelegate {
public:
    static constexpr int kSlotCount = 12;

    PauseScene();

    const char* Name() const override { return "pause"; }
    // 菜单自己吃输入；不往 ImGui 注入 nav 按键（菜单没有 ImGui 控件）
    void OnInput(UiContext& ui) override { (void)ui; }
    void OnUpdate(UiContext& ui, float dt) override;
    void OnRender(UiContext& ui) override;

    // MainMenuDelegate
    const char* GameTitle() const override { return "MOTHER 3 · GBA"; }
    void OnResume(gamemenu::GameMenuContext& ctx) override;
    void OnOpenStateSlots(gamemenu::GameMenuContext& ctx, bool saving) override;
    void OnOpenSettings(gamemenu::GameMenuContext& ctx, bool per_game) override;
    void OnRequestReset(gamemenu::GameMenuContext& ctx) override;
    void OnRequestExit(gamemenu::GameMenuContext& ctx) override;

    // StateSlotDelegate
    int SlotCount() const override { return kSlotCount; }
    void FillSlot(int slot, gamemenu::GameMenuSlotData& out) const override;
    bool SaveState(int slot) override;
    bool LoadState(int slot) override;

    // SettingsDelegate
    void OnSettingChanged(bool per_game, const char* category, const char* option,
                          int value) override;
    void OnSettingCommand(bool per_game, const char* category, const char* option) override;

    // DialogDelegate
    bool OnDialogResult(gamemenu::GameMenuContext& ctx, int button_index) override;

private:
    void DrawFakeGame(ImDrawList* draw_list, const gamemenu::Rect& screen);
    void DrawHud(ImDrawList* draw_list, const gamemenu::Rect& screen);
    void DrawHotkeyHints(ImDrawList* draw_list, const gamemenu::Rect& screen);
    void SetToast(const char* text);
    bool MenuOpen() const { return menu_.Visible(); }

    gamemenu::GameMenuHost menu_;
    FakeGameState game_{};
    MockSlot slots_[kSlotCount]{};
    gamemenu::DialogView::ButtonDef dialog_buttons_[3]{};
    const char* dialog_lines_[4]{};
    int pending_dialog_ = 0; // 0 无 1 重置 2 退出(有存档提示) 3 退出(无提示)
    char toast_[96] = {};
    float toast_timer_ = 0.0f;
    int save_count_ = 0;
    int loaded_slot_ = -1;
};

class PauseDemoApp final : public App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;
    void OnFrame(UiContext& ui, float dt) override;

private:
    std::string version_ = GUI_DEV_VERSION;
    int frame_ = 0;
};

} // namespace gui_dev::demo
