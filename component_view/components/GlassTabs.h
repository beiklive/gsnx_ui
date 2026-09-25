// GlassTabs：液态玻璃（Liquid Glass）风格的底部 Tab 条。
//
// 它做的事就是 Apple 那套的「抓背景 → 模糊 → 玻璃材质 → 高光/边缘 → 内容」，
// 但在 ImGui 的绘制模型里换成了不用着色器的实现方式（原因见 docs/component-view.md）：
//
//   1. 抓背景：宿主把「玻璃背后的那张纹理」交给组件（setBackdrop(纹理, 它在画布上的矩形)）。
//      模拟器里通常就是游戏帧纹理；demo 里是背景图。
//   2. 模糊：对这张纹理做多次带偏移的 AddImageRounded 采样（13 抽头环形核）——
//      不需要 render target、不需要 shader，mac / Switch 都一样，成本只是十几个贴图四边形。
//   3. 折射：从边缘往里画若干圈「UV 略微放大」的采样，越靠边放大越多、叠加权重越低，
//      于是边缘的背景被"掰弯"，中心保持原样 —— 这就是透镜感的来源（真的改 UV，不是画假高光）。
//   4. 玻璃材质：染色 + 提亮 + 顶部高光 + 镜面光斑 + 内亮边/外暗边。
//   5. 内容：Tab 项（图标 + 文字）+ 选中胶囊，画在材质之上。
//
// 因此它只需要一张背景纹理，就能在任何后端上出效果；真正的「采样当前帧缓冲」需要后端支持
// 离屏渲染 + UV 位移 shader（Switch 的 GL 可以做，mac 的 SDL_Renderer 不行）。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class GlassTabs : public Widget {
public:
    struct Item {
        std::string icon; // Material 字形
        std::string text;
    };

    struct Style {
        float height = 104.0f;        // 玻璃条高度
        float corner_radius = 30.0f;  // 玻璃圆角（大一点才显厚）
        float padding = 10.0f;        // 内容离玻璃边缘
        float item_gap = 4.0f;        // 项间距

        // ---- 模糊（多重采样，越大越糊也越贵）----
        int blur_taps = 13;           // 采样点数（1 = 不模糊）
        float blur_radius = 16.0f;    // 模糊半径（画布 px）
        float blur_alpha = 1.0f;      // 模糊层整体不透明度

        // ---- 折射（边缘 UV 放大）----
        int lens_bands = 3;           // 折射圈数
        float lens_inset = 4.0f;      // 每圈向内缩进（px）
        float lens_scale = 0.05f;     // 每圈额外放大比例
        float lens_alpha = 0.55f;     // 最外圈权重

        // ---- 材质 ----
        float tint_alpha = 0.34f;     // 染色
        float veil_alpha = 0.14f;     // 白雾（提亮/降饱和的观感）
        float rim_alpha = 0.80f;      // 内亮边
        float rim_width = 1.4f;
        float sheen_alpha = 0.18f;    // 顶部高光
        float specular_alpha = 0.13f; // 镜面光斑（弱一点更接近系统级玻璃）
        float capsule_alpha = 0.26f;  // 选中胶囊底
        float capsule_slide = 0.0f;   // >0 = 选中胶囊平滑滑动（秒），0 = 直接切换

        // ---- 内容 ----
        float icon_size = 26.0f;
        float text_size = Theme::kFontSmall;
        float item_radius = 16.0f;
    };

    GlassTabs();
    explicit GlassTabs(std::vector<Item> values);

    std::vector<Item> items() const { return items_; }
    int index() const { return index_; }
    int count() const { return static_cast<int>(items_.size()); }
    bool draggable = true;
    Style style;

    GlassTabs& setItems(std::vector<Item> values, int start_index = 0);
    GlassTabs& setIndex(int value, bool notify = true);
    const char* currentText() const;

    // 背景纹理 + 它在画布上覆盖的矩形。传空纹理时退化为「只有材质」的玻璃（不透背景）。
    GlassTabs& setBackdrop(ImTextureRef texture, const Rect& canvas_rect);
    bool hasBackdrop() const { return backdrop_.GetTexID() != ImTextureID_Invalid && backdrop_rect_.Valid(); }

    bool isDragging() const { return dragging_; }

signals:
    Signal<int> selectionChanged; // 选中项变了
    Signal<ImVec2> movedTo;       // 拖动后位置变化（相对父内容区）

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override; // 玻璃材质
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override; // Tab 内容
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    void DrawGlass(ImDrawList* dl, const Rect& body) const;
    void DrawBackdropSample(ImDrawList* dl, const Rect& dst, const ImVec2& uv_min, const ImVec2& uv_max, float radius,
                            ImU32 color) const;
    Rect ItemRect(int item) const;
    int ItemAtX(float x) const;
    void Activate();

    std::vector<Item> items_;
    ImTextureRef backdrop_ = ImTextureRef();
    Rect backdrop_rect_{};
    int index_ = 0;
    float capsule_x_ = -1.0f; // 选中胶囊的 x（<0 = 还没初始化）
    float capsule_w_ = 0.0f;

    bool dragging_ = false;
    ImVec2 drag_last_{0.0f, 0.0f};
    ImVec2 velocity_{0.0f, 0.0f};
    ImVec2 slosh_{0.0f, 0.0f};
    float phase_ = 0.0f; // 镜面光斑的轻微游走
};

} // namespace gui_dev::cv
