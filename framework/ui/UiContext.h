// UI 全局上下文：持有 ImGui 生命周期与后端引用，作为所有组件的入口。
//
// 约束：src/ui/ 与 src/core/ 下不允许 include 任何平台头文件（SDL/GLFW/libnx…），
// 平台能力一律通过 gui_dev::Backend 取。这条约束保证组件可在 mac / switch 复用。
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "platform/Backend.h"
#include "ui/Theme.h"

namespace gui_dev {

class UiContext {
public:
    explicit UiContext(Backend& backend);
    ~UiContext();

    UiContext(const UiContext&) = delete;
    UiContext& operator=(const UiContext&) = delete;

    // 每个主循环迭代各调用一次，顺序固定：BeginFrame -> 绘制 UI -> EndFrame。
    void BeginFrame();
    void EndFrame();

    // 分辨率/DPI 变化后需要重建字体图集。返回 true 表示本次发生了重建。
    bool RefreshIfDisplayChanged();

    // ---- 可用空间 ----------------------------------------------------------
    // 供组件做响应式布局（例如宽度 < 520 时切单列）。
    ImVec2 AvailableSize() const { return ImGui::GetContentRegionAvail(); }
    float AvailableWidth() const { return ImGui::GetContentRegionAvail().x; }

    bool Compact() const { return AvailableWidth() < Theme::kCompactBreakpoint; }

    // ---- 便捷绘制 ----------------------------------------------------------
    void Text(const char* fmt, ...) IM_FMTARGS(2);
    void TextDisabled(const char* fmt, ...) IM_FMTARGS(2);
    void SameLine() { ImGui::SameLine(); }
    void Spacing() { ImGui::Spacing(); }
    void Separator() { ImGui::Separator(); }

    // ---- 访问器 ------------------------------------------------------------
    Backend& GetBackend() const { return backend_; }
    PadState& Pad() { return input_.pad; }
    const PadState& Pad() const { return input_.pad; }
    float DeltaTime() const { return backend_.DeltaTime(); }
    std::uint32_t DisplayGeneration() const { return backend_.DisplayGeneration(); }
    // 用户缩放（放大 / 缩小按钮）：转给后端，切换后字体密度会跟着重建
    void SetUiZoom(float zoom) { backend_.SetUiZoom(zoom); }
    float UiZoom() const { return backend_.UiZoom(); }

    // ---- 上一帧的渲染统计（在 ImGui::Render() 之后采集，供 demo/性能面板用）----
    int LastDrawCalls() const { return last_draw_calls_; }
    int LastVertices() const { return last_vertices_; }
    int LastIndices() const { return last_indices_; }
    const char* AppVersion() const { return GUI_DEV_VERSION; }

private:
    void RebuildFonts();
    // 字体重建后自检按键图标 / Material 图标的字形是否齐全（缺了打印到 stderr）。
    void ReportGlyphCoverage() const;

    Backend& backend_;
    InputFrame input_;
    // 各字体源的 GlyphExcludeRanges 存储：imgui 只存指针，必须活到字体销毁。
    std::vector<std::vector<ImWchar>> exclusion_;
    std::uint32_t last_display_generation_ = 0;
    bool fonts_built_ = false;
    int last_draw_calls_ = 0;
    int last_vertices_ = 0;
    int last_indices_ = 0;
};

} // namespace gui_dev
