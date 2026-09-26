// Content：弹窗与页面共用的「内容型」基础控件 —— Label / Separator / ProgressBar / Image / RichText。
//
// 它们和 Button 是同一层的组件（都继承 Widget），所以：
//   * 布局、焦点、命中测试、裁剪、滚动、主题跟随全部复用基类；
//   * 放在页面里、放在弹窗内容区里、放在滚动容器里的行为完全一样。
//
// 这里刻意不引入「弹窗专用控件」：弹窗内容 = 这些控件 + 现有 Button/Box/Tab 等自由组合。
//
// 放在一个头里（和 Button.h 收 7 种按钮变体同一个理由）：它们是一族最小内容件，
// 单独拆 10 个文件只增加查找成本。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

// ------------------------------------------------------------------ Label ----
// 文本：单行 / 多行 / 自动换行，可居中、可省略号、可跑马灯（单行放不下时）。
//
// 用法：
//   auto* title = page.Emplace<Label>("游戏加载完成");
//   title->setFontSize(Theme::kFontHeader);
//   label->setWrap(true);          // 多行：按可用宽度自动换行
//   label->setAlign(TextAlign::Center);
class Label : public Widget {
public:
    Label();
    explicit Label(std::string value);

    std::string text;
    std::string icon;                 // 可选的 Material 字形，画在文字左边
    float icon_gap = 8.0f;
    float font_size = 0.0f;           // 0 = Theme::kFontBody
    ImVec4 text_color = Theme::kTextPrimary;
    bool text_color_follows_theme = true;
    TextAlign text_align = TextAlign::Left;
    VerticalAlign vertical_align = VerticalAlign::Middle;
    bool wrap = false;                // true = 按可用宽度换行（多行文本）
    float line_gap = 4.0f;            // 多行时的行距
    float max_lines = 0.0f;           // >0 = 最多画这么多行（超出省略）
    bool ellipsize = true;            // 单行放不下时用 "…"
    bool marquee = false;             // 单行放不下时横向滚动（优先于 ellipsize）
    float marquee_speed = 26.0f;

    Label& setText(std::string value);
    Label& setIcon(std::string glyph);
    Label& setFontSize(float value);
    Label& setColor(ImVec4 color);
    Label& setAlign(TextAlign value);
    Label& setVerticalAlign(VerticalAlign value);
    Label& setWrap(bool value, float gap = 4.0f);

    // 按当前换行设置算出来的文字块尺寸（外部排版/测量高度时可用）
    ImVec2 TextSize(float wrap_width) const;
    float LineHeight() const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnThemeChanged() override;

private:
    float ResolvedFontSize() const;
    // 换行后的行数（wrap = false 时按 '\n' 数）
    int CountLines(float wrap_width, std::vector<std::string>* out_lines) const;
};

// -------------------------------------------------------------- Separator ----
// 分隔线：水平（默认）或垂直。颜色跟主题，可以缩进（左右留白）。
//
//   page.Emplace<Separator>();                      // 水平线，宽度撑满父内容区
//   page.Emplace<Separator>(Separator::Orientation::Vertical);
class Separator : public Widget {
public:
    enum class Orientation { Horizontal, Vertical };

    Separator();
    explicit Separator(Orientation orientation);

    Orientation orientation = Orientation::Horizontal;
    float thickness = 1.0f;
    float length = 0.0f;    // 0 = 撑满父内容区（水平=宽度，垂直=高度）
    float inset_start = 0.0f;
    float inset_end = 0.0f;
    ImVec4 color{0.0f, 0.0f, 0.0f, 0.0f}; // alpha = 0 表示跟主题（kBorder）
    bool color_follows_theme = true;

    Separator& setThickness(float value);
    Separator& setLength(float value);
    Separator& setInset(float start, float end = -1.0f);
    Separator& setColor(ImVec4 value);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnThemeChanged() override;
};

// ------------------------------------------------------------ ProgressBar ----
// 进度条：确定进度（0..1）与不确定进度（indeterminate，跑动条）两种形态。
// ProgressPopup 的内容就是它；页面里也能直接用（扫描 ROM、编译 shader 等）。
//
//   ProgressBar* bar = ...; bar->setValue(0.62f); bar->setLabel("正在加载核心");
//   bar->setIndeterminate(true);
class ProgressBar : public Widget {
public:
    ProgressBar();

    float value = 0.0f;            // 0..1（显示值会平滑跟随，改 value 即可）
    bool indeterminate = false;
    bool show_percent = true;      // 右侧显示百分比
    bool show_label = false;       // 条形图上方/内部显示 label
    std::string label;
    float bar_height = 10.0f;
    float radius = 5.0f;
    float font_size = 0.0f;        // 0 = Theme::kFontSmall
    float label_gap = 8.0f;
    ImVec4 track_color{0.0f, 0.0f, 0.0f, 0.0f};  // alpha = 0 -> Theme::kTrack
    ImVec4 fill_color{0.0f, 0.0f, 0.0f, 0.0f};   // alpha = 0 -> Theme::kAccent
    ImVec4 text_color = Theme::kTextMuted;
    bool text_color_follows_theme = true;
    float smooth_speed = 12.0f;    // 显示值平滑速度（0 = 直接跳）

    ProgressBar& setValue(float next);
    ProgressBar& setLabel(std::string value);
    ProgressBar& setIndeterminate(bool enabled);
    ProgressBar& setBarHeight(float value);
    ProgressBar& setColors(ImVec4 fill, ImVec4 track);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    void OnThemeChanged() override;

private:
    float shown_ = 0.0f;    // 平滑后的显示值
    float phase_ = 0.0f;    // 不确定进度条的跑动相位
};

// ------------------------------------------------------------------ Image ----
// 图片：纹理由宿主（Page/App）通过后端加载好再交给它，组件本身不碰平台接口。
//
//   gui_dev::TextureRef tex(ui.GetBackend(), "img/border_gradient.png");
//   auto* image = page.Emplace<Image>();
//   image->setTexture(tex.ImGuiRef());   // 纹理对象必须活到控件销毁（宿主持有）
//   image->setFit(Image::Fit::Contain);
//
// 纹理无效（资源缺失）时画占位框 + 图标，不会崩。
class Image : public Widget {
public:
    enum class Fit {
        Contain, // 等比缩放，完整装进内容区（默认）
        Cover,   // 等比缩放，填满内容区（超出部分裁掉）
        Stretch, // 拉伸填满（不保持比例）
        None,    // 原始尺寸，居中
    };

    Image();

    ImTextureRef texture{}; // 无效 = ImTextureID_Invalid（画占位）
    float source_width = 0.0f;
    float source_height = 0.0f;
    Fit fit = Fit::Contain;
    ImVec4 tint = Theme::kWhite;
    bool tint_follows_theme = true;
    float radius = 0.0f;
    bool show_placeholder = true;   // 无纹理时画占位

    Image& setTexture(ImTextureRef ref, float width, float height);
    Image& setFit(Fit value);
    Image& setRadius(float value);
    Image& setTint(ImVec4 value);
    // 内容区里按 fit 算出来的实际贴图矩形
    Rect FitRect(const Rect& content) const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnThemeChanged() override;
};

// --------------------------------------------------------------- RichText ----
// 富文本：一段一段（run）带颜色 / 粗体 / 图标的文本，支持自动换行。
//
// 滚动怎么做？—— **不在这里做**。把它放进一个可滚动容器即可复用现有机制：
//
//   Box* view = host.Emplace<Box>("rich_scroll");
//   view->overflow = Overflow::Scroll;      // 这就是 ScrollView
//   RichText* text = view->Emplace<RichText>();
//   text->setRuns({{"无法加载核心", Theme::kError, true}, ...});
//
// 于是：手指/鼠标拖动滚动、滚动条、焦点自动滚动（Widget::EnsureVisible）全都由基类负责；
// RichText 额外提供「手柄上下键滚一行 / 翻一页」（capture_vertical = true 时生效）。
class RichText : public Widget {
public:
    // color 的 alpha = 0 表示「跟随主题正文色」（和 Box/Button 的约定一致），
    // 这样切主题时文本会自动换色；给了具体颜色就固定用它。
    struct Run {
        std::string text;
        ImVec4 color{0.0f, 0.0f, 0.0f, 0.0f};
        bool bold = false;
        std::string icon; // 这段前面画一个 Material 字形
    };

    RichText();

    std::vector<Run> runs;
    float font_size = 0.0f;   // 0 = Theme::kFontBody
    float line_gap = 6.0f;    // 行距
    float paragraph_gap = 10.0f; // 段落间距（空行）
    float icon_gap = 6.0f;
    bool color_follows_theme = true; // 未显式指定颜色的 run 跟随主题
    // 手柄上下键滚一行 / 左右键翻页（需要自己不是滚动容器时用）
    bool scroll_keys = true;
    float key_scroll_lines = 1.0f;

    RichText& setRuns(std::vector<Run> value);
    RichText& append(std::string text, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f), bool bold = false,
                     std::string icon = {});
    RichText& appendParagraph(std::string text);

    // 内容总高度 / 行数（容器测量用）
    float ContentHeight(float wrap_width) const;
    int LineCount(float wrap_width) const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override;

private:
    struct Glyph {
        std::string text;   // 单个词（可能含中文单字）
        ImVec4 color;
        bool bold = false;
        bool icon = false;
        float width = 0.0f;
        float space_after = 0.0f; // 词后是否需要空格
        bool line_break = false;  // 段落结束
        bool blank_line = false;  // 空行
    };
    struct Line {
        std::vector<Glyph> glyphs;
        float width = 0.0f;
        float height = 0.0f;
        float extra_gap = 0.0f; // 段落间距
    };
    void BuildLines(float wrap_width) const;
    float ResolvedFontSize() const;
    float LineHeight() const;
    // 内容指纹：文本/颜色/粗体/图标任一变化都要重排行
    std::size_t RunsHash() const;

    // 行布局缓存：内容或宽度没变就不重排（长文本每帧重排很贵）
    mutable std::vector<Line> lines_;
    mutable float cached_wrap_width_ = -1.0f;
    mutable std::size_t cached_hash_ = 0;
};

} // namespace gui_dev::cv
