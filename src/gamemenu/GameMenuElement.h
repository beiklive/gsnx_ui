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

// ------------------------------------------------------------ 错位色块 -----

// 聚焦时压在按钮下面的红色错位块。
// 每次获得焦点重新掷一次随机形状，再用 SmoothTo 过渡过去 —— 所以
// 「同一个条目再次聚焦」看到的错位方向/宽度也不一样（需求里的轻微位移/不规则边缘）。
struct AccentPlate {
    float dx = 0.0f;
    float dy = 0.0f;
    float extend = 0.0f;
    float skew_jitter = 0.0f;
    float target_dx = 0.0f;
    float target_dy = 0.0f;
    float target_extend = 0.0f;
    float target_skew_jitter = 0.0f;
    std::uint32_t rng = 0x9E3779B9u;

    struct Shape {
        Rect rect;
        float skew = 0.0f;
    };

    // 复位：只播随机种子，形状归零；真正掷形状在首次获得焦点时（那时才有真实主题）
    void Reset();
    // 重新掷一次（获得焦点时调用）
    void Reroll(const GameMenuTheme& theme);
    void Update(float dt, const GameMenuTheme& theme);
    // 依据本体矩形算出背板矩形与其斜切量
    Shape Apply(const Rect& rect, float base_skew) const;
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
    void Update(float dt, bool focused, const GameMenuTheme& theme);
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
    AccentPlate plate_{};
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
    void Update(float dt, bool selected, const GameMenuTheme& theme);
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
    void Update(float dt, const GameMenuTheme& theme);
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
    AccentPlate plate{};       // 聚焦时的红色错位块
    bool was_focused = false;

    void Reset();
    void Update(float dt, bool focused, const GameMenuTheme& theme);
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
    void Update(float dt, bool focused, const GameMenuTheme& theme);
    void TriggerPress() { anim_.press = 1.0f; }
    Rect Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect, int index,
              const GameMenuSlotData& data, float time) const;

    const MenuAnimationState& Anim() const { return anim_; }

private:
    MenuAnimationState anim_{};
    AccentPlate plate_{};
    bool was_focused_ = false;
};

} // namespace gui_dev::gamemenu
