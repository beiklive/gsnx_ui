// GlassBox：液态玻璃（Liquid Glass）风格的浮层。
//
// 说明：这是**近似**效果，不是 Apple 那种真折射。原因很实在 —— ImDrawList 只能画几何 + 固定混合、
// 没有片元着色器，采样不到背后的帧缓冲，所以「模糊 / 折射 / 饱和增强」做不了。这里用可用的手段把它拼出来：
//   1. 半透明主体：一层冷色 veil + 一层白雾叠出「厚度」
//   2. 顶部高光：3 层从亮到透的圆角矩形堆出垂直渐变（圆角保形，不方角外露）
//   3. 镜面光斑：多层椭圆叠出软边高光，位置随拖动速度偏移（液面晃动）
//   4. 边缘透镜带：外沿压暗 1px + 内侧亮线 + 两圈很淡的边缘雾气，模拟光在玻璃边缘聚集
//   5. 拖拽：按住拖动，位置夹在画布内；拖得越快高光甩得越开，松手指数回正
//
// 真正的折射要等后端支持离屏渲染 + 模糊（Switch 的 GL 可以，mac 的 SDL_Renderer 只能降采样近似）。
#pragma once

#include <string>

#include "component_view/components/Box.h"

namespace gui_dev::cv {

class GlassBox : public Box {
public:
    struct Style {
        float corner_radius = 18.0f;   // 玻璃圆角（比普通控件大，玻璃要显得厚）
        float veil_alpha = 0.26f;      // 冷色 veil 的不透明度
        float body_alpha = 0.16f;      // 白雾层的不透明度
        float sheen_alpha = 0.20f;     // 顶部高光强度
        float highlight_alpha = 0.30f; // 镜面光斑强度
        float rim_alpha = 0.85f;       // 内侧亮线强度
        float rim_width = 1.4f;        // 内侧亮线宽度
        float edge_fog = 0.10f;        // 边缘雾气强度
        float drag_gain = 0.55f;       // 拖动速度 → 高光偏移的倍率
        float slosh_max = 14.0f;       // 高光最大偏移（px）
        float slosh_speed = 7.5f;      // 回正速度（指数趋近）
        float label_size = Theme::kFontSmall;
        float hint_size = Theme::kFontBody;
    };

    GlassBox();
    explicit GlassBox(std::string label);

    std::string label; // 左上角文字（可空）
    std::string hint;  // 居中提示文字（可空）
    bool draggable = true;
    Style style;

    GlassBox& setLabel(std::string value);
    GlassBox& setHint(std::string value);

    bool isDragging() const { return dragging_; }

signals:
    Signal<ImVec2> movedTo; // 拖动后位置变化（相对父内容区）

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    void OnThemeChanged() override;

private:
    void DrawMaterial(ImDrawList* dl, const Rect& body) const;

    bool dragging_ = false;
    ImVec2 drag_last_{0.0f, 0.0f};
    ImVec2 velocity_{0.0f, 0.0f}; // 拖动速度（松手后指数衰减）
    ImVec2 slosh_{0.0f, 0.0f};    // 高光偏移（液面晃动）
};

} // namespace gui_dev::cv
