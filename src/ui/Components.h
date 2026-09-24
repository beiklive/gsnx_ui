// 统一组件库：所有模拟器核心都用这里的基础件，避免各写一套样式。
#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

namespace gui_dev {

class UiContext;

namespace Components {

// 页面骨架：页头（标题 + 版本/状态）与页脚（按键提示）。
// 内容区已自动 BeginChild，结尾处自动 EndChild。
bool BeginPanel(UiContext& ui, const char* title, const char* subtitle = nullptr);
void EndPanel();

// 页脚按键提示，例如 {{"A", "确定"}, {"B", "返回"}}。
void Footer(UiContext& ui, const std::vector<std::pair<const char*, const char*>>& hints);

// 分区标题，带一条分隔线。
void SectionHeader(UiContext& ui, const char* label);

// 标签/取值行，右对齐取值。value 为空时显示占位色。
void LabeledRow(UiContext& ui, const char* label, const char* value);

// 开关行。返回是否被点击切换；*value 已被就地更新。
bool ToggleRow(UiContext& ui, const char* label, bool* value, const char* hint = nullptr);

// 可选中列表：items 为 (主标题, 副标题)，返回被点击的索引，-1 表示本帧无点击。
// 选中项用 accent 高亮，宽度自适应。
int SelectableList(UiContext& ui, const char* id, const std::vector<std::pair<std::string, std::string>>& items,
                   int selected, float item_height = 0.0f);

// 一行工具栏按钮组。返回被点击按钮的索引，-1 表示无。
int Toolbar(UiContext& ui, const char* id, const std::vector<const char*>& labels, bool compact = false);

// 进度条 + 说明文字，用于资源更新、存档读写等长任务。
void ProgressRow(UiContext& ui, const char* label, float progress01, const char* detail = nullptr);

// 空态提示（无游戏、无存档等）。
void EmptyState(UiContext& ui, const char* message, const char* hint = nullptr);

// 模态确认框。open 为 true 时显示；返回 true 表示用户确认。
bool ConfirmModal(UiContext& ui, const char* id, const char* title, const char* message);

// 错误/信息提示条。
void StatusBanner(UiContext& ui, const char* message, bool is_error = false);

} // namespace Components
} // namespace gui_dev
