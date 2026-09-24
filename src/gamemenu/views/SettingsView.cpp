#include "gamemenu/views/SettingsView.h"

#include <cstdio>

#include <imgui.h>

namespace gui_dev::gamemenu {
namespace {

// ---- 独立设置（当前游戏/核心）----------------------------------------------
const char* kAspect[] = {"16:9", "4:3", "整数缩放", "全屏拉伸", "原始"};
const char* kRotate[] = {"0°", "90°", "180°", "270°"};
const char* kShader[] = {"LCD", "CRT", "锐化", "无"};
const char* kFilter[] = {"无", "扫描线", "网点", "柔和"};
const char* kVolume[] = {"静音", "25%", "50%", "75%", "100%"};
const char* kLatency[] = {"低", "中", "高"};
const char* kDeadzone[] = {"5%", "10%", "15%", "20%"};
const char* kTurbo[] = {"关闭", "2x", "4x"};
const char* kSpeed[] = {"50%", "75%", "100%", "125%", "150%"};
const char* kFrameskip[] = {"0", "1", "2", "3"};
const char* kSlot[] = {"1", "2", "3", "4", "5", "6"};

struct OptionDef {
    const char* label;
    MenuOptionKind kind;
    const char* const* values;
    int value_count;
    int initial;
};

struct CategoryDef {
    const char* label;
    const OptionDef* options;
    int option_count;
};

// 独立设置：视频 / 音频 / 输入 / 模拟 / 存档 / 游戏
constexpr OptionDef kPerGameVideo[] = {
    {"屏幕比例", MenuOptionKind::Choice, kAspect, 5, 0},
    {"画面旋转", MenuOptionKind::Choice, kRotate, 4, 0},
    {"整数缩放", MenuOptionKind::Toggle, nullptr, 0, 0},
    {"着色器", MenuOptionKind::Choice, kShader, 4, 0},
    {"滤镜", MenuOptionKind::Choice, kFilter, 4, 0},
};
constexpr OptionDef kPerGameAudio[] = {
    {"主音量", MenuOptionKind::Choice, kVolume, 5, 4},
    {"音频延迟", MenuOptionKind::Choice, kLatency, 3, 0},
    {"静音", MenuOptionKind::Toggle, nullptr, 0, 0},
    {"音频输出", MenuOptionKind::Command, nullptr, 0, 0},
};
constexpr OptionDef kPerGameInput[] = {
    {"按键映射", MenuOptionKind::Command, nullptr, 0, 0},
    {"摇杆死区", MenuOptionKind::Choice, kDeadzone, 4, 1},
    {"连发 (Turbo)", MenuOptionKind::Choice, kTurbo, 3, 0},
    {"快捷键", MenuOptionKind::Command, nullptr, 0, 0},
};
constexpr OptionDef kPerGameEmu[] = {
    {"运行速度", MenuOptionKind::Choice, kSpeed, 5, 2},
    {"跳帧", MenuOptionKind::Choice, kFrameskip, 4, 0},
    {"金手指", MenuOptionKind::Command, nullptr, 0, 0},
    {"核心选项", MenuOptionKind::Command, nullptr, 0, 0},
};
constexpr OptionDef kPerGameSave[] = {
    {"自动存档", MenuOptionKind::Toggle, nullptr, 0, 1},
    {"存档槽位", MenuOptionKind::Choice, kSlot, 6, 0},
    {"存档压缩", MenuOptionKind::Toggle, nullptr, 0, 0},
};
constexpr OptionDef kPerGameGame[] = {
    {"游戏信息", MenuOptionKind::Command, nullptr, 0, 0},
    {"金手指列表", MenuOptionKind::Command, nullptr, 0, 0},
    {"清除本游戏进度", MenuOptionKind::Command, nullptr, 0, 0},
};

constexpr CategoryDef kPerGameCategories[] = {
    {"视频", kPerGameVideo, 5}, {"音频", kPerGameAudio, 4}, {"输入", kPerGameInput, 4},
    {"模拟", kPerGameEmu, 4},   {"存档", kPerGameSave, 3},  {"游戏", kPerGameGame, 3},
};

// ---- 全局设置（整个 GBAStation）--------------------------------------------
const char* kUiTheme[] = {"黑红 (默认)", "暗色", "亮色"};
const char* kUiScale[] = {"90%", "100%", "110%", "120%"};
const char* kLanguage[] = {"简体中文", "English", "日本語"};
const char* kConfirmExit[] = {"总是询问", "直接退出", "从不"};
const char* kStorageLoc[] = {"SD 卡", "内置存储"};
const char* kPower[] = {"性能优先", "均衡", "省电"};
const char* kLogLevel[] = {"关闭", "错误", "信息", "调试"};

constexpr OptionDef kGlobalUi[] = {
    {"界面主题", MenuOptionKind::Choice, kUiTheme, 3, 0},
    {"界面缩放", MenuOptionKind::Choice, kUiScale, 4, 1},
    {"动画效果", MenuOptionKind::Toggle, nullptr, 0, 1},
    {"语言", MenuOptionKind::Choice, kLanguage, 3, 0},
};
constexpr OptionDef kGlobalInput[] = {
    {"手柄校准", MenuOptionKind::Command, nullptr, 0, 0},
    {"按键映射 (全局)", MenuOptionKind::Command, nullptr, 0, 0},
    {"菜单快捷键", MenuOptionKind::Command, nullptr, 0, 0},
    {"退出确认", MenuOptionKind::Choice, kConfirmExit, 3, 0},
};
constexpr OptionDef kGlobalAudio[] = {
    {"系统音量", MenuOptionKind::Choice, kVolume, 5, 4},
    {"启动音效", MenuOptionKind::Toggle, nullptr, 0, 1},
    {"音频后端", MenuOptionKind::Command, nullptr, 0, 0},
};
constexpr OptionDef kGlobalVideo[] = {
    {"菜单透明度", MenuOptionKind::Choice, kLatency, 3, 1},
    {"锁定 60Hz", MenuOptionKind::Toggle, nullptr, 0, 0},
    {"截图格式", MenuOptionKind::Choice, kLanguage, 3, 0},
};
constexpr OptionDef kGlobalStorage[] = {
    {"默认 ROM 目录", MenuOptionKind::Command, nullptr, 0, 0},
    {"存储位置", MenuOptionKind::Choice, kStorageLoc, 2, 0},
    {"清空缓存", MenuOptionKind::Command, nullptr, 0, 0},
};
constexpr OptionDef kGlobalSystem[] = {
    {"系统信息", MenuOptionKind::Command, nullptr, 0, 0},
    {"电源管理", MenuOptionKind::Choice, kPower, 3, 1},
    {"后台下载", MenuOptionKind::Toggle, nullptr, 0, 1},
    {"日志级别", MenuOptionKind::Choice, kLogLevel, 4, 1},
};

constexpr CategoryDef kGlobalCategories[] = {
    {"界面", kGlobalUi, 4},   {"输入", kGlobalInput, 4}, {"音频", kGlobalAudio, 3},
    {"视频", kGlobalVideo, 3}, {"存储", kGlobalStorage, 3}, {"系统", kGlobalSystem, 4},
};

} // namespace

SettingsView::SettingsView(SettingsDelegate& delegate, bool per_game)
    : delegate_(delegate), per_game_(per_game) {
    const CategoryDef* categories = per_game ? kPerGameCategories : kGlobalCategories;
    category_count_ = per_game ? static_cast<int>(sizeof(kPerGameCategories) / sizeof(CategoryDef))
                               : static_cast<int>(sizeof(kGlobalCategories) / sizeof(CategoryDef));
    for (int i = 0; i < category_count_; ++i) {
        tabs_[i].Configure(categories[i].label);
    }
}

void SettingsView::LoadCategory(int index, float direction) {
    const CategoryDef* categories = per_game_ ? kPerGameCategories : kGlobalCategories;
    const CategoryDef& category = categories[index];
    option_count_ = category.option_count < kMaxOptions ? category.option_count : kMaxOptions;
    for (int i = 0; i < option_count_; ++i) {
        const OptionDef& def = category.options[i];
        MenuOptionRow& row = rows_[i];
        row.label = def.label;
        row.kind = def.kind;
        row.values = def.values;
        row.value_count = def.value_count;
        row.value = def.initial;
        row.Reset();
    }
    focus_ = 0;
    category_anim_ = 1.0f; // 切分类的内容滑动
    category_direction_ = direction >= 0.0f ? 1 : -1;
}

void SettingsView::SetStatus(const char* text) {
    std::snprintf(status_text_, sizeof(status_text_), "%s", text != nullptr ? text : "");
    status_timer_ = 1.8f;
}

void SettingsView::OnEnter(GameMenuContext& ctx) {
    (void)ctx;
    category_ = 0;
    focus_ = 0;
    category_anim_ = 0.0f;
    status_timer_ = 0.0f;
    status_text_[0] = '\0';
    for (GameMenuTab& tab : tabs_) {
        tab.Reset();
    }
    focus_frame_.Reset();
    LoadCategory(0, 1.0f);
    category_anim_ = 0.0f; // 首次进入不做滑动，交给整体入场动画
}

void SettingsView::Update(GameMenuContext& ctx) {
    const MenuAnimationConfig& cfg = ctx.theme->animation;
    category_anim_ = DecayOnce(category_anim_, cfg.sweep_duration, ctx.dt);
    if (status_timer_ > 0.0f) {
        status_timer_ -= ctx.dt;
        if (status_timer_ < 0.0f) {
            status_timer_ = 0.0f;
        }
    }

    for (int i = 0; i < category_count_; ++i) {
        tabs_[i].Update(ctx.dt, i == category_, cfg);
    }
    for (int i = 0; i < option_count_; ++i) {
        rows_[i].Update(ctx.dt, cfg);
    }

    // 鼠标辅助：点分类切分类，点选项行聚焦，点左右区域改值
    const ImVec2 mouse = ImGui::GetMousePos();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        for (int i = 0; i < category_count_; ++i) {
            if (tab_rects_[i].Contains(mouse) && i != category_) {
                LoadCategory(i, i > category_ ? 1.0f : -1.0f);
                category_ = i;
                return;
            }
        }
        for (int i = 0; i < option_count_; ++i) {
            if (row_rects_[i].Contains(mouse)) {
                focus_ = i;
                return;
            }
        }
    }

    focus_frame_.Update(ctx.dt, row_rects_[focus_], cfg);
}

bool SettingsView::OnAction(GameMenuContext& ctx, InputAction action) {
    const char* category = per_game_ ? kPerGameCategories[category_].label
                                     : kGlobalCategories[category_].label;
    switch (action) {
    case InputAction::Up:
        focus_ = (focus_ + option_count_ - 1) % option_count_;
        return true;
    case InputAction::Down:
        focus_ = (focus_ + 1) % option_count_;
        return true;
    case InputAction::Left:
    case InputAction::Right: {
        MenuOptionRow& row = rows_[focus_];
        if (row.kind == MenuOptionKind::Command) {
            SetStatus("子页面：demo 未实现");
            return true;
        }
        const int direction = action == InputAction::Left ? -1 : 1;
        if (row.kind == MenuOptionKind::Toggle) {
            row.Change(0);
        } else {
            row.Change(direction);
        }
        delegate_.OnSettingChanged(per_game_, category, row.label, row.value);
        return true;
    }
    case InputAction::Confirm: {
        MenuOptionRow& row = rows_[focus_];
        if (row.kind == MenuOptionKind::Command) {
            SetStatus("子页面：demo 未实现");
            delegate_.OnSettingCommand(per_game_, category, row.label);
        } else if (row.kind == MenuOptionKind::Toggle) {
            row.Change(0);
            delegate_.OnSettingChanged(per_game_, category, row.label, row.value);
        } else {
            row.Change(1);
            delegate_.OnSettingChanged(per_game_, category, row.label, row.value);
        }
        return true;
    }
    case InputAction::PageLeft:
    case InputAction::PageRight: {
        const int direction = action == InputAction::PageLeft ? -1 : 1;
        category_ = (category_ + direction + category_count_) % category_count_;
        LoadCategory(category_, static_cast<float>(direction));
        return true;
    }
    default:
        return false;
    }
}

void SettingsView::Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) {
    const GameMenuTheme& theme = *ctx.theme;
    const MenuAnimationConfig& cfg = theme.animation;

    // ---- 左侧分类 ----------------------------------------------------------
    const float tab_w = 108.0f;
    const float tab_h = 40.0f;
    float tab_y = area.min.y + 6.0f;
    for (int i = 0; i < category_count_; ++i) {
        tab_rects_[i] = tabs_[i].Draw(draw_list, theme, ImVec2(area.min.x, tab_y), tab_w, tab_h);
        tab_y += tab_h + 4.0f;
    }
    // 分类之间的竖分隔线（漫画切割感）
    AddSkewFilled(draw_list, MakeRect(area.min.x + tab_w + 16.0f, area.min.y + 4.0f, 2.0f,
                                      area.Height() - 30.0f),
                  2.0f, ColorWithAlpha(theme.white, 0.18f));

    // ---- 右侧选项 ----------------------------------------------------------
    const float opt_x = area.min.x + tab_w + 34.0f;
    const float opt_w = area.max.x - opt_x - 14.0f; // 留出聚焦右移的空间
    const float opt_h = 44.0f;

    // 切分类时整列内容滑动 + 淡出
    const float slide = EaseOutCubic(1.0f - category_anim_);
    const float content_dx = static_cast<float>(category_direction_) * (1.0f - slide) * 26.0f;
    const float content_alpha = 1.0f - category_anim_ * 0.85f;

    float row_y = area.min.y + 6.0f;
    for (int i = 0; i < option_count_; ++i) {
        const float stagger = EaseOutCubic(StaggerProgress(ctx.open_progress, i, cfg));
        const float alpha = stagger * content_alpha;
        const Rect cell = MakeRect(opt_x + content_dx + (1.0f - stagger) * 30.0f, row_y, opt_w, opt_h);
        row_rects_[i] = cell;
        rows_[i].Draw(draw_list, ThemeWithAlpha(theme, alpha), cell, i == focus_, ctx.time);
        row_y += opt_h + 8.0f;
    }
    (void)slide;

    focus_frame_.Draw(draw_list, theme, EaseOutCubic(ctx.open_progress));

    // ---- 底部操作说明 ------------------------------------------------------
    const float info_y = area.max.y - 16.0f;
    AddTextLeftVCentered(draw_list, ImVec2(area.min.x + 4.0f, info_y),
                         ColorWithAlpha(theme.white_dim, 0.85f), theme.small_size,
                         "L/R 分类   ↑↓ 选择   ←→ 修改   A 确认");
    if (status_timer_ > 0.0f) {
        const float a = Clamp01(status_timer_ / 0.7f);
        AddTextLeftVCentered(draw_list, ImVec2(opt_x, info_y), ColorWithAlpha(theme.red, a),
                             theme.small_size, status_text_);
    }
}

} // namespace gui_dev::gamemenu
