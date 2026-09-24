// 统一组件库：所有模拟器核心都用这里的基础件，避免各写一套样式。
#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

namespace gui_dev {

class UiContext;

namespace Components {

// 页面骨架：铺满屏幕的根画布 + 页头 + 可滚动内容区 + 底部按键提示。
//
// 用法（页脚必须在 EndPanel 里提交，因为它要画在画布内）：
//   Components::BeginPanel(ui, "标题", "副标题");
//   ... 内容 ...
//   Components::EndPanel(ui, {{Icons::Glyph(Icons::Button::A), "确定"}});
//
// BeginPanel 会把内容区（body child）打开，因此 BeginPanel 与 EndPanel 之间
// 绘制的一切都在可滚动内容区里；EndPanel 内部负责收尾与页脚定位。
bool BeginPanel(UiContext& ui, const char* title, const char* subtitle = nullptr);
void EndPanel(UiContext& ui,
              const std::vector<std::pair<const char*, const char*>>& footer_hints = {});

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

// ---- 可聚焦方框 -----------------------------------------------------------

// 方框样式。尺寸 <= 0 表示按可用空间自适应。
struct BoxStyle {
    ImVec2 size{0.0f, 132.0f}; // x<=0：填满可用宽度；y<=0：用 height 兜底
    float height = 132.0f;
    float rounding = 12.0f;
    float border_width = 3.0f;   // 聚焦边框粗细
    float glow_width = 8.0f;     // 外发光宽度
    float padding = 16.0f;       // 内容内边距
    float flow_speed = 0.30f;    // 流光速度：每秒沿边框转几圈
    float flow_cycles = 1.0f;    // 流光贴图沿周长平铺几遍
    float flow_dim_alpha = 0.10f; // 流光暗部不透明度（够低才能看清跑动的光斑）
    float flow_peak_alpha = 1.0f; // 流光光斑不透明度
    // 流光路径的等弧长采样步长（像素）。越小圆角越接近真圆弧；
    // 2.0 时一个 12px 半径圆角约 9 段，肉眼已看不出折线。
    float flow_segment_length = 2.0f;
    ImU32 fill_color = IM_COL32(0x1C, 0x21, 0x27, 0xFF);
    ImU32 idle_border_color = IM_COL32(0x2C, 0x32, 0x3A, 0xFF);
    ImU32 hover_border_color = IM_COL32(0x44, 0x4E, 0x5C, 0xFF);
    ImU32 focus_fallback_color = IM_COL32(0x4F, 0xA3, 0xFF, 0xFF); // 没有流光贴图时的聚焦色
    // 流光贴图：assets/img/border_gradient.png（横向周期渐变，逐行相同）。
    // 无效时退化为静态强调色边框。
    ImTextureRef flow_texture{};
};

// 内容绘制回调，参数是扣掉 padding 后的可用尺寸。
using BoxContentFn = std::function<void(const ImVec2& content_size)>;

struct BoxResult {
    bool clicked = false; // 本帧被点击激活
    bool hovered = false;
};

// 可聚焦方框。焦点由调用方维护（手柄/键盘选择列表里通常是焦点索引），
// focused 为 true 时用 border_gradient 画流光边框。
//
//   BoxResult r = Components::FocusableBox("card0", focus == 0, style, [](const ImVec2&) {
//       ImGui::TextUnformatted("标题");
//   });
//   if (r.hovered) focus = 0;
//   if (r.clicked) Launch(0);
BoxResult FocusableBox(const char* id, bool focused, const BoxStyle& style,
                       const BoxContentFn& draw_content = {});

} // namespace Components
} // namespace gui_dev
