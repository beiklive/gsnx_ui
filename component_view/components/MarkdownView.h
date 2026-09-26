// MarkdownView：用第三方库 imgui_markdown（enkisoftware/imgui_markdown）渲染 Markdown。
//
// 为什么是"窗口里的子窗口"而不是纯 draw list？
//   imgui_markdown 是即时模式实现（内部用 ImGui::TextUnformatted / Indent / Bullet / Image
//   这些 item API 排版），它依赖一个真实的 ImGui 窗口与光标，无法直接喂 draw list。
//   所以我们给它一个铺在弹窗内容区上的无输入子窗口：
//     * 输入仍然完全由本框架处理（子窗口 NoInputs，命中测试/焦点/滚动都不受影响）；
//     * 排版用库自己的实现（换行、缩进、列表、代码块、图片、标题字号），不再自己写一套；
//     * 滚动仍然由我们的滚动容器负责（子窗口禁掉自己的滚动，我们按容器滚动量定位）。
//
// 局限（框架层约定）：放在滚动容器里时，控件高度必须由外部显式给定（Popup 每帧按
// renderedHeight() 回填），因为 Widget::Measure 会把自适应高度夹到视口高度。
#pragma once

#include <functional>
#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class MarkdownView : public Widget {
public:
    // 图片解析：Markdown 里的 ![alt](path) 会走到这里；返回 false 表示没有这张图
    using ImageResolver =
        std::function<bool(const std::string& path, ImTextureRef& texture, float& width, float& height)>;

    MarkdownView();

    std::string text;                     // Markdown 源文
    ImageResolver image_resolver;
    float text_scale = 1.0f;              // 整体字号缩放（跟随 UIScale 时用）
    float max_image_height = 220.0f;      // 图片最大高度（0 = 不限）
    float line_gap = 6.0f;                // 段落/行间距
    bool scroll_keys = true;              // 上下键滚最近的滚动容器
    bool heading_separator = true;        // 标题下面画一条分隔线
    ImVec4 link_color = Theme::kBlue;     // 链接文字颜色
    ImVec4 emphasis_color{0.0f, 0.0f, 0.0f, 0.0f}; // 斜体颜色（alpha=0 = 用正文色）

    MarkdownView& setText(std::string value);
    MarkdownView& setImageResolver(ImageResolver resolver);
    // 上一帧实际渲染出来的内容高度（滚动容器与弹窗布局用它定高）
    float renderedHeight() const { return rendered_height_; }
    // 包含本控件的滚动容器（没有返回 nullptr）
    Widget* ScrollHostOrNull() const;

protected:
    // 焦点框框住"可见的文本区"：Markdown 内容可比视口高得多，整块框住会被裁剪掉看不见
    FocusVisual BuildFocusVisual() const override;
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override {}

private:
    float rendered_height_ = 1.0f;
};

} // namespace gui_dev::cv
