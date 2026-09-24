// 通用 UI 元素：按钮 / 面板 / 分类签 / 数值选择器 / 存档槽 / 焦点框 / 选项行。
//
// 每个元素自己持有动画状态，动画只在 Update(dt) 里推进、只在 Draw 里读，
// 业务逻辑不在这里（需求 §4/§35）。
#pragma once

#include <cstdint>

#include <imgui.h>

#include "gamemenu/GameMenuDraw.h"
#include "gamemenu/GameMenuTheme.h"
#include "ui/Icons.h"

namespace gui_dev::gamemenu {

enum class MenuButtonKind : std::uint8_t {
    Normal = 0,
    Danger,    // 危险操作：常态即带红
    Separator, // 分组分隔线（不可聚焦，只画不规则线）
};

// ---------------------------------------------------------------- 焦点框 ----

// 平滑跟随当前焦点项的装饰框（独立元素，避免每个按钮各写一套跟随逻辑）。
class GameMenuFocusFrame {
public:
    void Reset();
    void SnapTo(const Rect& target);
    void Update(float dt, const Rect& target, const MenuAnimationConfig& cfg);
    void Draw(ImDrawList* draw_list, const GameMenuTheme& theme, float strength) const;

private:
    Rect rect_{};
    float alpha_ = 0.0f;
};

// ------------------------------------------------------------------ 按钮 ----

class GameMenuButton {
public:
    void Configure(const char* label, Icons::Material icon, MenuButtonKind kind);
    // OnEnter 时调用：清掉上一次的动画残留（需求 §28）
    void Reset();
    void Update(float dt, bool focused, const MenuAnimationConfig& cfg);
    void TriggerPress() { anim_.press = 1.0f; }

    Rect Draw(ImDrawList* draw_list, const GameMenuTheme& theme, ImVec2 origin, float width,
              float height, float time) const;

    const MenuAnimationState& Anim() const { return anim_; }
    const char* Label() const { return label_; }
    MenuButtonKind Kind() const { return kind_; }

private:
    const char* label_ = "";
    Icons::Material icon_ = Icons::Material::Count;
    MenuButtonKind kind_ = MenuButtonKind::Normal;
    MenuAnimationState anim_{};
    bool was_focused_ = false;
};

// ------------------------------------------------------------------ 面板 ----

class GameMenuPanel {
public:
    void Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect, const char* title,
              const char* subtitle, float enter) const;
};

// -------------------------------------------------------------- 分类签 ------

class GameMenuTab {
public:
    void Configure(const char* label);
    void Reset();
    void Update(float dt, bool selected, const MenuAnimationConfig& cfg);
    Rect Draw(ImDrawList* draw_list, const GameMenuTheme& theme, ImVec2 origin, float width,
              float height) const;

private:
    const char* label_ = "";
    MenuAnimationState anim_{};
    bool was_selected_ = false;
};

// ------------------------------------------------------- 数值选择器 / 开关 ----

class GameMenuSelector {
public:
    void Reset();
    void OnValueChanged(int direction); // -1 / +1，触发文字滑动
    void Update(float dt, const MenuAnimationConfig& cfg);
    void Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect, const char* value,
              bool focused, float time) const;

private:
    float slide_ = 0.0f; // 1 -> 0
    int direction_ = 0;
};

enum class MenuOptionKind : std::uint8_t {
    Choice = 0, // < v >
    Toggle,     // [ ON ] / [ OFF ]
    Command,    // 进入子页
};

// 设置页的一行。动画状态挂在行上，避免每帧新建任何东西。
struct MenuOptionRow {
    const char* label = "";
    MenuOptionKind kind = MenuOptionKind::Choice;
    const char* const* values = nullptr;
    int value_count = 0;
    int value = 0;
    MenuAnimationState anim{};
    float toggle = 0.0f;      // 0..1 开关填充
    GameMenuSelector selector; // Choice 行的 < v > 动画状态

    void Reset();
    void Update(float dt, const MenuAnimationConfig& cfg);
    void Change(int direction); // Choice: -1/+1 换值；Toggle: 翻转
    void Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect, bool focused,
              float time) const;
    const char* CurrentValue() const;
    bool ToggleOn() const { return value != 0; }
};

// -------------------------------------------------------------- 存档槽 ------

struct GameMenuSlotData {
    bool exists = false;
    const char* time_text = "";
    const char* play_text = "";
};

class GameMenuSaveSlot {
public:
    void Reset();
    void Update(float dt, bool focused, const MenuAnimationConfig& cfg);
    void TriggerPress() { anim_.press = 1.0f; }
    Rect Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect, int index,
              const GameMenuSlotData& data, float time) const;

    const MenuAnimationState& Anim() const { return anim_; }

private:
    MenuAnimationState anim_{};
    bool was_focused_ = false;
};

} // namespace gui_dev::gamemenu
