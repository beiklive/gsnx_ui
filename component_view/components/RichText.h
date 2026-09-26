// RichText：独立单文件富文本 Widget（无第三方 Markdown 依赖）。
//
// 定位：只做「高性能显示」，不做编辑（无光标 / 选择 / 剪贴板 / 撤销）。
// 用途：弹窗内容、游戏说明、更新日志、README、Core 信息、错误说明、关于页……
//
// 架构（严格分离，不边解析边画）：
//
//     Markdown 文本
//         │  ParseMarkdown()          —— 只在 SetMarkdown 时跑一次
//         ▼
//     RichTextDocument（blocks → inline items → spans / image nodes）
//         │  Layout()                 —— 只在「内容 / 宽度 / 字号 / 主题」变化时跑
//         ▼
//     RichTextLayout（lines → glyphs，含每行矩形与图片矩形）
//         │  Render()                 —— 每帧
//         ▼
//     ImDrawList（复用 Draw:: 与 Theme::，不新造视觉体系）
//
// 支持范围：普通文本、段落、#~###### 标题、**粗体**、*斜体*、***粗斜体***、~~删除线~~、
//   `行内代码`、- / * 无序列表（两级缩进）、1. 有序列表、> 引用、
//   --- 分隔线、[文字](url) 链接、![alt](path) 行内/独占图片、[color=#RRGGBB(AA)]…[/color] 染色（可嵌套）。
//   （``` 代码块已按反馈移除：项目没有等宽字体，显示效果不理想。）
//
// 不做（明确边界）：HTML / CSS / DOM / Table / Mermaid / LaTeX / WebView / 编辑器。
//
// 性能（Switch 720p 是主要目标）：
//   * 解析只在 SetMarkdown 时发生一次；
//   * 布局带缓存，宽度/字号/主题没变就直接复用；
//   * 图片由外部 resolver 解析并在解析阶段缓存，绝不每帧加载；
//   * 渲染每帧走一遍已排好的 glyph 列表，不做字符串分配。
//
// 与框架的关系：本文件只依赖 Widget / Draw / Theme / Global（都是现有基础设施），
// 不修改 Widget、Theme、Button、Popup、Focus、Layout；滚动容器与焦点由外部现有系统负责。
#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/Widget.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

// ---------------------------------------------------------------- 资源 -----
// 图片来源由宿主提供（RichText 不认识 PNG / 文件系统 / SD 卡 / HTTP）
struct RichTextImage {
    ImTextureRef texture{};                // 无效 = 画占位
    ImVec2 size{0.0f, 0.0f};               // 原始像素尺寸
    bool valid = false;

    static RichTextImage Invalid() { return RichTextImage{}; }
};

// ------------------------------------------------------------ 文档模型 -----
// 一段同样式文字
struct RichTextSpan {
    std::string text;
    bool bold = false;
    bool italic = false;
    bool strike = false;
    bool code = false;                     // 行内代码：加底色带
    bool has_color = false;                // false = 用主题正文色
    ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool is_link = false;
    std::string link;                      // is_link 时的 URL
};

// 段落里的一个 inline 单元：文字 或 图片（图片可以出现在文字中间）
struct RichTextInline {
    enum class Kind { Text, Image };
    Kind kind = Kind::Text;
    RichTextSpan span;                     // kind == Text
    std::string image_source;              // kind == Image
    std::string image_alt;
    RichTextImage image;                   // 解析阶段由 resolver 填好，之后不再动
};

enum class RichTextBlockKind {
    Paragraph,
    Heading,
    Bullet,     // - / * / +
    Ordered,    // 1.
    Quote,      // >
    Divider,    // ---
};

struct RichTextBlock {
    RichTextBlockKind kind = RichTextBlockKind::Paragraph;
    int level = 0;                         // 标题级别 1..6 / 列表缩进 0..2
    std::string marker;                    // "•" 或 "1."（列表用）
    std::vector<RichTextInline> items;
};

struct RichTextDocument {
    std::vector<RichTextBlock> blocks;

    bool Empty() const { return blocks.empty(); }
    void Clear() { blocks.clear(); }
};

// ---------------------------------------------------------------- Widget ---
class RichText : public Widget {
public:
    // 图片解析回调：source → 纹理 + 尺寸。解析阶段调用一次，之后缓存。
    using ImageResolver = std::function<RichTextImage(const std::string& source)>;
    // 链接回调：点击链接时调用（RichText 不直接开浏览器）
    using LinkCallback = std::function<void(const std::string& url)>;

    RichText();
    explicit RichText(std::string markdown);

    // ---- 内容 ----
    RichText& SetMarkdown(std::string markdown);   // 解析 + 标记布局失效
    RichText& SetText(std::string markdown) { return SetMarkdown(std::move(markdown)); }
    const std::string& Markdown() const { return markdown_; }
    const RichTextDocument& Document() const { return document_; }

    // ---- 资源 / 交互 ----
    RichText& SetImageResolver(ImageResolver resolver);
    RichText& SetLinkCallback(LinkCallback callback);

    // ---- 排版参数（默认全部来自 Theme / Global::component_style）----
    RichText& SetTextScale(float scale);                       // 整体字号缩放
    RichText& SetLineGap(float gap);                           // 行距
    RichText& SetParagraphGap(float gap);                      // 段间距
    RichText& SetMaxImageSize(float max_width, float max_height);
    RichText& SetImagePlaceholder(bool enabled) { image_placeholder_ = enabled; return *this; }
    RichText& SetAutoHeight(bool enabled) { auto_height_ = enabled; return *this; }
    RichText& SetIndentWidth(float width) { indent_width_ = width; return *this; }
    // 上下键是否用来滚「外部」滚动容器：
    //   false（默认）= 方向键交给焦点导航（页面里多个 RichText 之间可以正常上下移动）；
    //   true          = 焦点停在本控件，上下键滚它所在的滚动容器（弹窗里读长文时用）。
    RichText& SetScrollKeys(bool enabled) {
        scroll_keys_ = enabled;
        capture_vertical = enabled;
        return *this;
    }
    bool ScrollKeys() const { return scroll_keys_; }

    // ---- 只读结果（外部滚动容器 / 弹窗布局用）----
    float ContentHeight() const { return layout_.height; }
    float ContentWidth() const { return layout_.width; }
    float LastRenderHeight() const { return rendered_height_; }
    bool Empty() const { return document_.Empty(); }

    // 已排版的链接命中区（矩形相对内容区左上角；命中测试时再加上 content_rect.min）
    struct LinkHit {
        Rect rect;
        std::string url;
    };
    const std::vector<LinkHit>& Links() const { return links_; }

signals:
    Signal<std::string> linkActivated; // 链接被点击（和 SetLinkCallback 等价，二选一）

protected:
    // 焦点框框住"可见的文本区"：Markdown 内容可能远高于视口，整块框住会被裁剪掉
    FocusVisual BuildFocusVisual() const override;
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override;

private:
    // ---------------------------------------------------------- 排版结果 ----
    struct Glyph {
        enum class Kind { Text, Image, Divider, Marker } kind = Kind::Text;
        const RichTextSpan* span = nullptr;   // Text/Marker：指向文档里的 span（文档稳定，可安全引用）
        std::string text;                     // 复制一份，避免文档变化时悬垂（仅图片占位/标记用）
        ImVec2 pos{0.0f, 0.0f};               // 相对排版原点
        ImVec2 size{0.0f, 0.0f};
        float font_size = 0.0f;
        ImVec4 color{};
        bool bold = false;
        bool italic = false;
        bool strike = false;
        bool code = false;
        bool is_link = false;
        std::string link;
        ImTextureRef texture{};
        bool image_valid = false;
        float baseline = 0.0f;                // 文本基线（相对行顶）
    };

    struct Line {
        std::vector<Glyph> glyphs;
        float width = 0.0f;
        float height = 0.0f;
        float y = 0.0f;
        float extra_gap = 0.0f;               // 行前的额外间距（段间距/标题间距）
        bool code_line = false;               // 整行代码：铺一条底色带
        bool quote_line = false;              // 引用：左侧竖线
        float indent = 0.0f;
    };

    struct Layout {
        std::vector<Line> lines;
        float width = 0.0f;
        float height = 0.0f;
    };

    // ---------------------------------------------------------------- 内部 --
    void InvalidateLayout() { layout_dirty_ = true; }
    void RebuildDocument();
    void RebuildLayout(float width);
    void RenderLines(ImDrawList* dl, const Rect& content);
    float HeadingSize(int level) const;
    float BodySize() const;
    float ResolvedScale() const;
    float ResolvedLineGap() const;
    float ResolvedParagraphGap() const;
    float ResolvedIndent() const;

    std::string markdown_;
    RichTextDocument document_;
    bool document_dirty_ = true;
    Layout layout_;
    bool layout_dirty_ = true;
    float layout_width_ = -1.0f;
    float layout_scale_ = -1.0f;
    float rendered_height_ = 0.0f;

    ImageResolver image_resolver_;
    LinkCallback link_callback_;
    std::vector<LinkHit> links_;
    std::string hovered_link_;
    std::string pressed_link_;

    float text_scale_ = 1.0f;
    float line_gap_ = -1.0f;               // <0 = Theme 默认
    float paragraph_gap_ = -1.0f;
    float indent_width_ = -1.0f;
    float max_image_width_ = 0.0f;         // 0 = 用内容宽度
    float max_image_height_ = 220.0f;      // 0 = 不限
    bool image_placeholder_ = true;
    bool auto_height_ = true;
    bool scroll_keys_ = false;
};

// ============================================================================
//  实现（单文件：全部 inline，避免拆 .cpp）
// ============================================================================

namespace rich_text_detail {

// ---- UTF-8：只处理遍历与"能否逐字断行"，不引 ICU ----
inline bool IsAsciiSpace(char c) { return c == ' ' || c == '\t' || c == '\r'; }

inline std::string Trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && IsAsciiSpace(text[begin])) {
        ++begin;
    }
    while (end > begin && IsAsciiSpace(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// 返回下一个 codepoint 的起始下标（UTF-8 连续字节不切开）
inline std::size_t NextCodepoint(const std::string& text, std::size_t index) {
    std::size_t next = index + 1;
    while (next < text.size() && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
        ++next;
    }
    return next;
}

// 宽字符（CJK / 全角）：可以逐字换行；ASCII 词要整词换行
inline bool IsWideCodepoint(const std::string& text, std::size_t index) {
    return static_cast<unsigned char>(text[index]) >= 0xE0;
}

inline bool StartsWith(const std::string& text, std::size_t index, const char* prefix) {
    const std::size_t length = std::char_traits<char>::length(prefix);
    return text.compare(index, length, prefix) == 0;
}

// #RRGGBB / #RRGGBBAA -> ImVec4（分量 0..1）；失败返回 false
inline bool ParseHexColor(const std::string& text, ImVec4& out) {
    std::size_t index = 0;
    if (index < text.size() && text[index] == '#') {
        ++index;
    }
    unsigned value[8] = {0};
    std::size_t count = 0;
    while (index < text.size() && count < 8) {
        const char c = text[index];
        unsigned digit = 0;
        if (c >= '0' && c <= '9') {
            digit = static_cast<unsigned>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = static_cast<unsigned>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = static_cast<unsigned>(c - 'A' + 10);
        } else {
            break;
        }
        value[count++] = digit;
        ++index;
    }
    if (count != 6 && count != 8) {
        return false;
    }
    auto channel = [&](std::size_t offset) {
        return static_cast<float>(value[offset] * 16 + value[offset + 1]) / 255.0f;
    };
    out = ImVec4(channel(0), channel(2), channel(4), count == 8 ? channel(6) : 1.0f);
    return true;
}

// ---- 行内样式（解析期传递）----
struct InlineStyle {
    bool bold = false;
    bool italic = false;
    bool strike = false;
    bool code = false;
    bool has_color = false;
    ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool is_link = false;
    std::string link;
};

inline void FlushText(std::vector<RichTextInline>& out, std::string& pending, const InlineStyle& style) {
    if (pending.empty()) {
        return;
    }
    RichTextInline item;
    item.kind = RichTextInline::Kind::Text;
    item.span.text = std::move(pending);
    item.span.bold = style.bold;
    item.span.italic = style.italic;
    item.span.strike = style.strike;
    item.span.code = style.code;
    item.span.has_color = style.has_color;
    item.span.color = style.color;
    item.span.is_link = style.is_link;
    item.span.link = style.link;
    out.push_back(std::move(item));
    pending.clear();
}

// 找到与 start 处标记配对的结束标记（支持嵌套同种标记）；失败返回 npos
inline std::size_t FindClosing(const std::string& text, std::size_t content_start, const char* open,
                               const char* close, std::size_t end) {
    const std::size_t open_len = std::char_traits<char>::length(open);
    const std::size_t close_len = std::char_traits<char>::length(close);
    int depth = 1;
    std::size_t index = content_start;
    while (index < end) {
        // 先判 close 再判 open：** / * / ~~ 这类「开闭同形」的标记，
        // 若先判 open 会把收尾也当成新的开始，depth 永远回不到 0（原来 **粗体** 解析不出来就是这个原因）。
        if (StartsWith(text, index, close)) {
            --depth;
            if (depth == 0) {
                return index;
            }
            index += close_len;
            continue;
        }
        if (StartsWith(text, index, open)) {
            ++depth;
            index += open_len;
            continue;
        }
        index = NextCodepoint(text, index);
    }
    return std::string::npos;
}

// 行内解析：把 [start,end) 解析成 inline items（样式继承 + 递归）
inline void ParseInline(const std::string& text, std::size_t start, std::size_t end, const InlineStyle& style,
                        const RichText::ImageResolver& resolver, std::vector<RichTextInline>& out) {
    std::string pending;
    std::size_t index = start;
    while (index < end) {
        // ---- 图片：![alt](source) ----
        if (text[index] == '!' && StartsWith(text, index, "![")) {
            const std::size_t alt_end = text.find("](", index + 2);
            const std::size_t source_end = alt_end == std::string::npos ? std::string::npos : text.find(')', alt_end);
            if (alt_end != std::string::npos && source_end != std::string::npos && alt_end < end) {
                FlushText(out, pending, style);
                RichTextInline item;
                item.kind = RichTextInline::Kind::Image;
                item.image_alt = text.substr(index + 2, alt_end - index - 2);
                item.image_source = Trim(text.substr(alt_end + 2, source_end - alt_end - 2));
                if (resolver) {
                    item.image = resolver(item.image_source);
                }
                out.push_back(std::move(item));
                index = source_end + 1;
                continue;
            }
        }
        // ---- 链接：[文字](url) ----
        if (text[index] == '[' && !StartsWith(text, index, "[color=")) {
            const std::size_t label_end = text.find("](", index + 1);
            const std::size_t url_end = label_end == std::string::npos ? std::string::npos : text.find(')', label_end);
            if (label_end != std::string::npos && url_end != std::string::npos && label_end < end) {
                FlushText(out, pending, style);
                InlineStyle link_style = style;
                link_style.is_link = true;
                link_style.link = Trim(text.substr(label_end + 2, url_end - label_end - 2));
                ParseInline(text, index + 1, label_end, link_style, resolver, out);
                index = url_end + 1;
                continue;
            }
        }
        // ---- 染色：[color=#RRGGBB]...[/color]（可嵌套，内部照常解析） ----
        if (StartsWith(text, index, "[color=")) {
            const std::size_t value_end = text.find(']', index + 7);
            if (value_end != std::string::npos && value_end < end) {
                ImVec4 color;
                if (ParseHexColor(text.substr(index + 7, value_end - index - 7), color)) {
                    const std::size_t close = FindClosing(text, value_end + 1, "[color=", "[/color]", end);
                    if (close != std::string::npos) {
                        FlushText(out, pending, style);
                        InlineStyle colored = style;
                        colored.has_color = true;
                        colored.color = color;
                        ParseInline(text, value_end + 1, close, colored, resolver, out);
                        index = close + std::char_traits<char>::length("[/color]");
                        continue;
                    }
                }
            }
        }
        // ---- 粗体 / 斜体 / 粗斜体 ----
        if (StartsWith(text, index, "***") || StartsWith(text, index, "___")) {
            const char* marker = StartsWith(text, index, "***") ? "***" : "___";
            const std::size_t len = 3;
            const std::size_t close = FindClosing(text, index + len, marker, marker, end);
            if (close != std::string::npos) {
                FlushText(out, pending, style);
                InlineStyle strong = style;
                strong.bold = true;
                strong.italic = true;
                ParseInline(text, index + len, close, strong, resolver, out);
                index = close + len;
                continue;
            }
        }
        if (StartsWith(text, index, "**") || StartsWith(text, index, "__")) {
            const char* marker = StartsWith(text, index, "**") ? "**" : "__";
            const std::size_t len = 2;
            const std::size_t close = FindClosing(text, index + len, marker, marker, end);
            if (close != std::string::npos) {
                FlushText(out, pending, style);
                InlineStyle strong = style;
                strong.bold = true;
                ParseInline(text, index + len, close, strong, resolver, out);
                index = close + len;
                continue;
            }
        }
        if (text[index] == '*' || text[index] == '_') {
            const char marker_buf[2] = {text[index], '\0'};
            const std::size_t close = FindClosing(text, index + 1, marker_buf, marker_buf, end);
            if (close != std::string::npos && close > index + 1) {
                FlushText(out, pending, style);
                InlineStyle em = style;
                em.italic = true;
                ParseInline(text, index + 1, close, em, resolver, out);
                index = close + 1;
                continue;
            }
        }
        // ---- 删除线 ----
        if (StartsWith(text, index, "~~")) {
            const std::size_t close = FindClosing(text, index + 2, "~~", "~~", end);
            if (close != std::string::npos) {
                FlushText(out, pending, style);
                InlineStyle strike = style;
                strike.strike = true;
                ParseInline(text, index + 2, close, strike, resolver, out);
                index = close + 2;
                continue;
            }
        }
        // ---- 行内代码 ----
        if (text[index] == '`') {
            const std::size_t close = text.find('`', index + 1);
            if (close != std::string::npos && close < end && close > index + 1) {
                FlushText(out, pending, style);
                InlineStyle code = style;
                code.code = true;
                ParseInline(text, index + 1, close, code, resolver, out);
                index = close + 1;
                continue;
            }
        }
        // ---- 普通字符（按 codepoint 追加，保证 UTF-8 不被切开） ----
        const std::size_t next = NextCodepoint(text, index);
        pending.append(text, index, next - index);
        index = next;
    }
    FlushText(out, pending, style);
}

// ---- 整篇解析 ----
inline RichTextDocument ParseMarkdown(const std::string& markdown, const RichText::ImageResolver& resolver) {
    RichTextDocument document;
    std::size_t index = 0;
    RichTextBlock paragraph;
    auto flush_paragraph = [&]() {
        if (!paragraph.items.empty()) {
            document.blocks.push_back(std::move(paragraph));
            paragraph = RichTextBlock{};
        }
    };
    while (index <= markdown.size()) {
        const std::size_t newline = markdown.find('\n', index);
        const bool last = newline == std::string::npos;
        const std::string raw = last ? markdown.substr(index) : markdown.substr(index, newline - index);
        const std::string line = Trim(raw);
        index = last ? markdown.size() + 1 : newline + 1;

        if (line.empty()) {
            flush_paragraph();
            continue;
        }

        // 分隔线
        const bool divider = (line.size() >= 3) &&
                             (line.find_first_not_of('-') == std::string::npos ||
                              line.find_first_not_of('*') == std::string::npos ||
                              line.find_first_not_of('_') == std::string::npos);
        if (divider) {
            flush_paragraph();
            RichTextBlock block;
            block.kind = RichTextBlockKind::Divider;
            document.blocks.push_back(std::move(block));
            continue;
        }

        // 标题（# ~ ######）
        if (line[0] == '#') {
            int level = 0;
            while (level < 6 && static_cast<std::size_t>(level) < line.size() && line[level] == '#') {
                ++level;
            }
            if (static_cast<std::size_t>(level) < line.size() && line[level] == ' ') {
                flush_paragraph();
                RichTextBlock block;
                block.kind = RichTextBlockKind::Heading;
                block.level = level;
                ParseInline(line, static_cast<std::size_t>(level) + 1, line.size(), InlineStyle{}, resolver,
                            block.items);
                document.blocks.push_back(std::move(block));
                continue;
            }
        }

        // 引用
        if (StartsWith(line, 0, "> ")) {
            flush_paragraph();
            RichTextBlock block;
            block.kind = RichTextBlockKind::Quote;
            ParseInline(line, 2, line.size(), InlineStyle{}, resolver, block.items);
            document.blocks.push_back(std::move(block));
            continue;
        }

        // 无序列表（缩进按每 2 空格一级，最多两级）
        if (StartsWith(line, 0, "- ") || StartsWith(line, 0, "* ") || StartsWith(line, 0, "+ ")) {
            flush_paragraph();
            std::size_t spaces = 0;
            while (spaces < raw.size() && raw[spaces] == ' ') {
                ++spaces;
            }
            RichTextBlock block;
            block.kind = RichTextBlockKind::Bullet;
            block.level = static_cast<int>(spaces / 2) > 2 ? 2 : static_cast<int>(spaces / 2);
            block.marker = "•";
            ParseInline(line, 2, line.size(), InlineStyle{}, resolver, block.items);
            document.blocks.push_back(std::move(block));
            continue;
        }

        // 有序列表
        {
            std::size_t digits = 0;
            while (digits < line.size() && line[digits] >= '0' && line[digits] <= '9') {
                ++digits;
            }
            if (digits > 0 && digits + 1 < line.size() && line[digits] == '.' && line[digits + 1] == ' ') {
                flush_paragraph();
                RichTextBlock block;
                block.kind = RichTextBlockKind::Ordered;
                block.level = 0;
                block.marker = line.substr(0, digits) + ".";
                ParseInline(line, digits + 2, line.size(), InlineStyle{}, resolver, block.items);
                document.blocks.push_back(std::move(block));
                continue;
            }
        }

        // 普通段落（一行一段：单换行也换行，避免中英混排被拼在一起）
        if (!paragraph.items.empty()) {
            flush_paragraph();
        }
        paragraph.kind = RichTextBlockKind::Paragraph;
        ParseInline(line, 0, line.size(), InlineStyle{}, resolver, paragraph.items);
        document.blocks.push_back(std::move(paragraph));
        paragraph = RichTextBlock{};
    }

    flush_paragraph();
    return document;
}

} // namespace rich_text_detail

// ---------------------------------------------------------------- 构造 -----

inline RichText::RichText() : Widget("rich_text") {
    // 默认不参与焦点（规范：RichText 默认 Focusable = false）；
    // 需要用手柄滚动外部容器时：focusable = true + SetScrollKeys(true)。
    focusable = false;
    capture_vertical = false;
}

inline RichText::RichText(std::string markdown) : RichText() {
    SetMarkdown(std::move(markdown));
}

inline RichText& RichText::SetMarkdown(std::string markdown) {
    markdown_ = std::move(markdown);
    document_dirty_ = true;
    layout_dirty_ = true;
    return *this;
}

inline RichText& RichText::SetImageResolver(ImageResolver resolver) {
    image_resolver_ = std::move(resolver);
    document_dirty_ = true; // 图片要重新解析（解析阶段就解析好，之后缓存）
    layout_dirty_ = true;
    return *this;
}

inline RichText& RichText::SetLinkCallback(LinkCallback callback) {
    link_callback_ = std::move(callback);
    return *this;
}

inline RichText& RichText::SetTextScale(float scale) {
    if (text_scale_ != scale) {
        text_scale_ = scale;
        layout_dirty_ = true;
    }
    return *this;
}

inline RichText& RichText::SetLineGap(float gap) {
    line_gap_ = gap;
    layout_dirty_ = true;
    return *this;
}

inline RichText& RichText::SetParagraphGap(float gap) {
    paragraph_gap_ = gap;
    layout_dirty_ = true;
    return *this;
}

inline RichText& RichText::SetMaxImageSize(float max_width, float max_height) {
    max_image_width_ = max_width;
    max_image_height_ = max_height;
    layout_dirty_ = true;
    return *this;
}

// ---------------------------------------------------------------- 度量 -----

inline float RichText::ResolvedScale() const {
    return text_scale_ > 0.0f ? text_scale_ : 1.0f;
}

inline float RichText::BodySize() const {
    return Theme::kFontBody * ResolvedScale();
}

inline float RichText::HeadingSize(int level) const {
    const float scale = ResolvedScale();
    if (level <= 1) {
        return (Theme::kFontTitle + 4.0f) * scale;
    }
    if (level == 2) {
        return Theme::kFontHeader * scale;
    }
    return Theme::kFontBody * scale;
}

inline float RichText::ResolvedLineGap() const {
    return line_gap_ >= 0.0f ? line_gap_ : Theme::kGapSmall * 0.5f;
}

inline float RichText::ResolvedParagraphGap() const {
    return paragraph_gap_ >= 0.0f ? paragraph_gap_ : Theme::kGap;
}

inline float RichText::ResolvedIndent() const {
    return indent_width_ > 0.0f ? indent_width_ : Theme::kGap * 2.0f;
}

// ---------------------------------------------------------------- 解析 -----

inline void RichText::RebuildDocument() {
    if (!document_dirty_) {
        return;
    }
    document_ = rich_text_detail::ParseMarkdown(markdown_, image_resolver_);
    document_dirty_ = false;
    layout_dirty_ = true;
}

// ---------------------------------------------------------------- 布局 -----

inline void RichText::RebuildLayout(float width) {
    const float scale = ResolvedScale();
    if (!layout_dirty_ && layout_.width == width && layout_scale_ == scale) {
        return;
    }
    layout_.lines.clear();
    links_.clear();
    layout_.width = width;
    layout_scale_ = scale;
    layout_dirty_ = false;

    ImFont* font = Draw::CurrentFont();
    const float body_size = BodySize();
    const float line_gap = ResolvedLineGap();
    const float paragraph_gap = ResolvedParagraphGap();
    const float indent_step = ResolvedIndent();
    const float content_width = Maxf(width, 1.0f);

    float y = 0.0f;
    bool first_block = true;

    auto line_height_for = [&](float font_size) {
        return Draw::MeasureText(font, font_size, "Ag", 0.0f).y;
    };

    for (const RichTextBlock& block : document_.blocks) {
        if (block.kind == RichTextBlockKind::Divider) {
            Line line;
            line.height = Maxf(ResolvedParagraphGap(), 6.0f);
            line.y = y;
            Glyph glyph;
            glyph.kind = Glyph::Kind::Divider;
            glyph.pos = ImVec2(0.0f, line.height * 0.5f);
            glyph.size = ImVec2(content_width, 1.0f);
            glyph.color = Theme::kBorder;
            line.glyphs.push_back(std::move(glyph));
            line.width = content_width;
            y += line.height + paragraph_gap;
            first_block = false;
            layout_.lines.push_back(std::move(line));
            continue;
        }

        const bool is_heading = block.kind == RichTextBlockKind::Heading;
        const float block_font = is_heading ? HeadingSize(block.level) : body_size;
        const bool block_bold = is_heading || false;
        const ImVec4 block_color = is_heading ? Theme::kTextBright : Theme::kTextPrimary;

        float indent = 0.0f;
        float marker_width = 0.0f;
        std::string marker;
        if (block.kind == RichTextBlockKind::Bullet || block.kind == RichTextBlockKind::Ordered) {
            indent = indent_step * static_cast<float>(block.level + 1);
            marker = block.marker;
            marker_width = Draw::MeasureText(font, body_size, (marker + " ").c_str(), 0.0f).x;
        } else if (block.kind == RichTextBlockKind::Quote) {
            indent = indent_step * 0.5f;
        }

        const float available = Maxf(content_width - indent - marker_width, 24.0f);
        const bool block_gap = !first_block;
        float line_extra = 0.0f;
        if (block_gap) {
            line_extra = (is_heading ? paragraph_gap * 1.2f : paragraph_gap * 0.6f);
        }

        Line line;
        line.extra_gap = line_extra;
        line.indent = indent;
        line.quote_line = block.kind == RichTextBlockKind::Quote;
        line.y = y + line.extra_gap;
        float cursor = 0.0f;
        bool marker_pending = !marker.empty();
        bool line_started = false;

        auto flush_line = [&]() {
            if (!line_started) {
                return;
            }
            line.height = Maxf(line.height, line_height_for(block_font));
            y = line.y + line.height + line_gap;
            layout_.lines.push_back(std::move(line));
            line = Line{};
            line.indent = indent;
            line.y = y;
            cursor = 0.0f;
            line_started = false;
        };

        auto push_text = [&](const RichTextSpan& span, const std::string& text, float font_size, bool bold,
                             bool italic) {
            if (text.empty()) {
                return;
            }
            Glyph glyph;
            glyph.kind = Glyph::Kind::Text;
            glyph.span = &span;
            glyph.text = text;
            glyph.font_size = font_size;
            glyph.color = span.has_color ? span.color : block_color;
            glyph.bold = bold;
            glyph.italic = italic;
            glyph.strike = span.strike;
            glyph.code = span.code;
            glyph.is_link = span.is_link;
            glyph.link = span.link;
            glyph.size = Draw::MeasureText(font, font_size, text.c_str(), 0.0f);
            glyph.baseline = glyph.size.y * 0.8f;

            if (!line_started && marker_pending) {
                Glyph marker_glyph;
                marker_glyph.kind = Glyph::Kind::Marker;
                marker_glyph.text = marker;
                marker_glyph.font_size = body_size;
                marker_glyph.color = Theme::kTextMuted;
                marker_glyph.pos = ImVec2(indent, 0.0f);
                marker_glyph.size = Draw::MeasureText(font, body_size, marker.c_str(), 0.0f);
                marker_glyph.baseline = marker_glyph.size.y * 0.8f;
                line.glyphs.push_back(std::move(marker_glyph));
                marker_pending = false;
            }

            if (cursor + glyph.size.x > available && line_started) {
                flush_line();
            }
            glyph.pos = ImVec2(indent + marker_width + cursor, 0.0f);
            cursor += glyph.size.x;
            line.width = cursor;
            line.height = Maxf(line.height, glyph.size.y);
            line_started = true;

            line.glyphs.push_back(std::move(glyph));
        };

        for (const RichTextInline& item : block.items) {
            if (item.kind == RichTextInline::Kind::Image) {
                // 图片：等比缩放，不超过可用宽度 / 最大尺寸；参与行内布局
                Glyph glyph;
                glyph.kind = Glyph::Kind::Image;
                glyph.texture = item.image.texture;
                glyph.image_valid = item.image.valid && item.image.texture.GetTexID() != ImTextureID_Invalid;
                glyph.text = item.image_alt;
                glyph.color = Theme::kTextMuted;
                const bool standalone = block.items.size() == 1;
                float draw_width = item.image.size.x;
                float draw_height = item.image.size.y;
                if (glyph.image_valid && draw_width > 0.0f && draw_height > 0.0f) {
                    const float max_width = max_image_width_ > 0.0f ? Minf(max_image_width_, available) : available;
                    float scale = Minf(1.0f, max_width / draw_width);
                    if (max_image_height_ > 0.0f) {
                        scale = Minf(scale, max_image_height_ / draw_height);
                    }
                    draw_width *= scale;
                    draw_height *= scale;
                } else {
                    // 占位：一块和正文一样高的提示
                    draw_width = Minf(available, 240.0f);
                    draw_height = line_height_for(body_size);
                }
                glyph.size = ImVec2(draw_width, draw_height);
                glyph.baseline = draw_height;

                if (standalone) {
                    // 独占一段：居中
                    flush_line();
                    const float offset = Maxf((content_width - draw_width) * 0.5f, 0.0f);
                    glyph.pos = ImVec2(offset, 0.0f);
                    line.glyphs.push_back(std::move(glyph));
                    line.width = offset + draw_width;
                    line.height = draw_height;
                    line_started = true;
                    flush_line();
                } else {
                    if (cursor + draw_width > available && line_started) {
                        flush_line();
                    }
                    if (cursor + draw_width > available) {
                        const float shrink = Maxf(available - cursor, 24.0f) / Maxf(draw_width, 1.0f);
                        glyph.size.x *= shrink;
                        glyph.size.y *= shrink;
                    }
                    glyph.pos = ImVec2(indent + marker_width + cursor, 0.0f);
                    cursor += glyph.size.x;
                    line.width = cursor;
                    line.height = Maxf(line.height, glyph.size.y);
                    line_started = true;
                    line.glyphs.push_back(std::move(glyph));
                }
                continue;
            }

            // 文本：按「词」切分（拉丁整词、CJK 逐字），保证换行与字体测量一致
            const RichTextSpan& span = item.span;
            const float font_size = block_font;
            const bool bold = span.bold || block_bold;
            const std::string& text = span.text;
            std::size_t index = 0;
            std::size_t word_start = 0;
            auto flush_word = [&](std::size_t end) {
                if (end > word_start) {
                    push_text(span, text.substr(word_start, end - word_start), font_size, bold, span.italic);
                }
            };
            while (index < text.size()) {
                if (rich_text_detail::IsWideCodepoint(text, index)) {
                    flush_word(index);
                    const std::size_t next = rich_text_detail::NextCodepoint(text, index);
                    push_text(span, text.substr(index, next - index), font_size, bold, span.italic);
                    index = next;
                    word_start = index;
                    continue;
                }
                if (rich_text_detail::IsAsciiSpace(text[index])) {
                    flush_word(index);
                    push_text(span, " ", font_size, bold, span.italic);
                    index = rich_text_detail::NextCodepoint(text, index);
                    word_start = index;
                    continue;
                }
                index = rich_text_detail::NextCodepoint(text, index);
            }
            flush_word(text.size());
        }

        // 行内代码：整行都是代码时铺底色
        bool all_code = !line.glyphs.empty();
        for (const Glyph& glyph : line.glyphs) {
            if (glyph.kind == Glyph::Kind::Text && !glyph.code) {
                all_code = false;
                break;
            }
            if (glyph.kind != Glyph::Kind::Text) {
                all_code = false;
                break;
            }
        }
        line.code_line = all_code;
        flush_line();

        first_block = false;
    }

    layout_.height = y;
    rendered_height_ = layout_.height;

    // 链接命中区：按最终行位置修正（行 y 在布局中被多次移动）
    links_.clear();
    for (const Line& line : layout_.lines) {
        for (const Glyph& glyph : line.glyphs) {
            if (glyph.is_link && !glyph.link.empty()) {
                LinkHit hit;
                hit.rect = Rect{ImVec2(glyph.pos.x, line.y + glyph.pos.y),
                                ImVec2(glyph.pos.x + glyph.size.x, line.y + glyph.pos.y + glyph.size.y)};
                hit.url = glyph.link;
                links_.push_back(std::move(hit));
            }
        }
    }
}

// ------------------------------------------------------- 渲染 / 每帧 -----

inline FocusVisual RichText::BuildFocusVisual() const {
    FocusVisual visual = Widget::BuildFocusVisual();
    if (!visual.enabled) {
        return visual;
    }
    Widget* host = const_cast<RichText*>(this)->ScrollHost();
    if (host != nullptr && host != const_cast<RichText*>(this) && host->overflow == Overflow::Scroll) {
        const float scale = Maxf(DrawScale(), 0.001f);
        const float offset = Maxf(focus_frame_offset, 2.0f) * scale;
        visual.rect = host->DrawRect().Expanded(offset * 0.5f);
        visual.radius = Global::component_style.corner_radius * scale + offset;
    }
    return visual;
}

inline ImVec2 RichText::MeasureContent(const ImVec2& available) {
    RebuildDocument();
    const float width = Maxf(size.x > 0.0f ? size.x : available.x, 1.0f);
    RebuildLayout(width);
    const ImVec2 measured(width, Maxf(layout_.height, 1.0f));
    if (auto_height_) {
        // 自撑高：滚动容器需要「内容比视口高」才能滚；显式高度也不会被 available 夹住。
        // 调用方要固定高度时把 auto_height 关掉（SetAutoHeight(false)）即可。
        size.y = measured.y;
    }
    return measured;
}

inline void RichText::RenderLines(ImDrawList* dl, const Rect& content) {
    ImFont* font = Draw::CurrentFont();
    const float opacity = EffectiveOpacity();
    const float radius = Global::component_style.corner_radius;
    const float scale_draw = DrawScale();

    dl->PushClipRect(content.min, content.max, true);

    for (const Line& line : layout_.lines) {
        const float line_y = content.min.y + line.y;

        // 引用：左侧竖线
        if (line.quote_line) {
            const float bar_x = content.min.x + line.indent * 0.4f;
            dl->AddLine(ImVec2(bar_x, line_y), ImVec2(bar_x, line_y + line.height),
                        Theme::Alpha(Theme::U32(Theme::kBorderStrong), opacity), Maxf(2.0f, scale_draw * 1.5f));
        }

        // 代码行底色（从内容左边到右边，整行铺一条）
        if (line.code_line) {
            const Rect band{ImVec2(content.min.x, line_y), ImVec2(content.max.x, line_y + line.height)};
            Draw::RoundedRectFilled(dl, band, Theme::Alpha(Theme::U32(Theme::kBgInput), opacity * 0.85f),
                                    Minf(radius, 4.0f), Minf(radius, 4.0f), Minf(radius, 4.0f), Minf(radius, 4.0f));
        }

        for (const Glyph& glyph : line.glyphs) {
            const float x = content.min.x + glyph.pos.x;
            const float y = line_y + glyph.pos.y;

            switch (glyph.kind) {
            case Glyph::Kind::Divider: {
                const float divider_y = y;
                dl->AddLine(ImVec2(x, divider_y), ImVec2(x + glyph.size.x, divider_y),
                            Theme::Alpha(Theme::U32(glyph.color), opacity), Maxf(1.0f, scale_draw));
                break;
            }
            case Glyph::Kind::Marker: {
                Draw::Text(dl, font, glyph.font_size, ImVec2(x, y), Theme::Alpha(Theme::U32(glyph.color), opacity),
                           glyph.text.c_str(), 0.0f);
                break;
            }
            case Glyph::Kind::Image: {
                const Rect target{ImVec2(x, y), ImVec2(x + glyph.size.x, y + glyph.size.y)};
                if (glyph.image_valid) {
                    dl->AddImageRounded(glyph.texture, target.min, target.max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                        Theme::Alpha(Theme::U32(Theme::kWhite), opacity), radius, ImDrawFlags_None);
                    // 行内小图描一圈边，避免和底色糊在一起
                    Draw::RoundedRectOutline(dl, target, Theme::Alpha(Theme::U32(Theme::kBorder), opacity * 0.8f),
                                             1.0f, radius, radius, radius, radius);
                } else if (image_placeholder_) {
                    Draw::RoundedRectFilled(dl, target, Theme::Alpha(Theme::U32(Theme::kBgWidget), opacity * 0.6f),
                                            radius, radius, radius, radius);
                    Draw::RoundedRectOutline(dl, target, Theme::Alpha(Theme::U32(Theme::kBorder), opacity), 1.0f,
                                             radius, radius, radius, radius);
                    const char* icon = Icons::Glyph(Icons::Material::ImagePlaceholder);
                    const float icon_size = Minf(glyph.size.y * 0.7f, 20.0f);
                    const ImVec2 icon_size_measured = Draw::MeasureText(font, icon_size, icon, 0.0f);
                    Draw::Text(dl, font, icon_size,
                               ImVec2(target.Center().x - icon_size_measured.x * 0.5f,
                                      target.Center().y - icon_size_measured.y * 0.5f),
                               Theme::Alpha(Theme::U32(Theme::kTextMuted), opacity), icon, 0.0f);
                }
                break;
            }
            case Glyph::Kind::Text:
            default: {
                ImU32 color = Theme::Alpha(Theme::U32(glyph.color), opacity);
                if (glyph.is_link && glyph.link == hovered_link_) {
                    color = Theme::Alpha(Theme::U32(Theme::kAccentHover), opacity);
                }
                // 行内代码底色
                if (glyph.code && !line.code_line) {
                    const Rect band{ImVec2(x - 3.0f, y - 1.0f),
                                    ImVec2(x + glyph.size.x + 3.0f, y + glyph.size.y + 1.0f)};
                    Draw::RoundedRectFilled(dl, band, Theme::Alpha(Theme::U32(Theme::kBgInput), opacity * 0.85f), 3.0f,
                                            3.0f, 3.0f, 3.0f);
                }

                const int vtx_start = dl->VtxBuffer.Size;
                Draw::Text(dl, font, glyph.font_size, ImVec2(x, y), color, glyph.text.c_str(), 0.0f);
                if (glyph.bold) {
                    // 只有一套字重：半像素偏移再画一遍模拟粗体
                    Draw::Text(dl, font, glyph.font_size, ImVec2(x + 0.6f, y), color, glyph.text.c_str(), 0.0f);
                }
                if (glyph.italic) {
                    // 没有斜体字重：对刚写入的顶点做一次"绕基线剪切"（ImDrawList 顶点缓冲是公开 API）
                    const float baseline = y + glyph.baseline;
                    const float slant = 0.24f;
                    for (int i = vtx_start; i < dl->VtxBuffer.Size; ++i) {
                        dl->VtxBuffer[i].pos.x += slant * (baseline - dl->VtxBuffer[i].pos.y);
                    }
                }
                if (glyph.strike) {
                    const float strike_y = y + glyph.size.y * 0.55f;
                    dl->AddLine(ImVec2(x, strike_y), ImVec2(x + glyph.size.x, strike_y), color,
                                Maxf(1.0f, glyph.font_size * 0.07f));
                }
                if (glyph.is_link) {
                    const float underline_y = y + glyph.size.y * 0.92f;
                    dl->AddLine(ImVec2(x, underline_y), ImVec2(x + glyph.size.x, underline_y), color,
                                Maxf(1.0f, scale_draw));
                }
                break;
            }
            }
        }
    }

    dl->PopClipRect();
}

inline void RichText::OnDrawContent(ImDrawList* dl, const Rect& content) {
    RebuildDocument();
    RebuildLayout(Maxf(content.Width(), 1.0f));
    if (layout_.lines.empty()) {
        rendered_height_ = 0.0f;
        return;
    }
    RenderLines(dl, content);
    rendered_height_ = layout_.height;
}

// 链接交互（鼠标 / 触摸；RichText 不参与焦点系统）
inline void RichText::OnUpdate(float dt) {
    (void)dt;
    hovered_link_.clear();
    if (links_.empty() || !Global::mouse_available) {
        return;
    }
    const ImVec2 mouse = Global::mouse;
    for (const LinkHit& link : links_) {
        // links_ 里的矩形是内容区局部坐标，鼠标是画布坐标 → 要加上内容区原点
        const Rect screen = link.rect.Translate(content_rect.min);
        if (screen.Contains(mouse)) {
            hovered_link_ = link.url;
            if (Global::mouse_pressed[0]) {
                pressed_link_ = link.url;
            }
            if (Global::mouse_released[0] && pressed_link_ == link.url) {
                pressed_link_.clear();
                if (link_callback_) {
                    link_callback_(link.url);
                }
                emit linkActivated(link.url);
            }
            break;
        }
    }
    if (Global::mouse_released[0]) {
        pressed_link_.clear();
    }
}

inline bool RichText::OnPadAction(InputAction action) {
    // 只有调用方把 focusable 打开、并且 SetScrollKeys(true) 时才会走到这里：
    // 上下键滚「外部」的滚动容器（RichText 自己不做滚动容器）。
    if (!scroll_keys_) {
        return false;
    }
    int direction = 0;
    float step_lines = 1.0f;
    if (action == InputAction::Up) {
        direction = -1;
    } else if (action == InputAction::Down) {
        direction = 1;
    } else if (action == InputAction::PageLeft || action == InputAction::TriggerLeft) {
        direction = -1;
        step_lines = 4.0f;
    } else if (action == InputAction::PageRight || action == InputAction::TriggerRight) {
        direction = 1;
        step_lines = 4.0f;
    }
    if (direction == 0) {
        return false;
    }
    Widget* host = ScrollHost();
    if (host == nullptr || host == this || host->scroll_max.y <= 0.5f) {
        return false;
    }
    const float step = (BodySize() + ResolvedLineGap()) * step_lines;
    host->scroll_target.y =
        Clampf(host->scroll_target.y + static_cast<float>(direction) * step, 0.0f, host->scroll_max.y);
    return true;
}

inline void RichText::OnThemeChanged() {
    layout_dirty_ = true; // 颜色进了排版结果，主题变了要重排
}

} // namespace gui_dev::cv
