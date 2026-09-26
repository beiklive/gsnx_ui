#include "component_view/Markdown.h"

#include <cstring>

namespace gui_dev::cv::Markdown {
namespace {

constexpr ImVec4 kFollowTheme{0.0f, 0.0f, 0.0f, 0.0f}; // alpha = 0 -> 主题正文色

bool StartsWith(const std::string& text, std::size_t index, const char* prefix) {
    const std::size_t length = std::strlen(prefix);
    return text.compare(index, length, prefix) == 0;
}

std::string Trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) {
        ++begin;
    }
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

void PushText(std::vector<RichText::Run>& out, const std::string& text, const ImVec4& color, bool bold = false,
              bool italic = false, float font_size = 0.0f, bool code = false) {
    if (text.empty()) {
        return;
    }
    RichText::Run run;
    run.text = text;
    run.color = color;
    run.bold = bold;
    run.italic = italic;
    run.font_size = font_size;
    run.code = code;
    out.push_back(std::move(run));
}

// 行内解析：**粗体** / *斜体* / `代码` / [文本](url) / ![alt](path)
void ParseInline(const std::string& text, const Options& options, const ImageLookup& lookup,
                 std::vector<RichText::Run>& out, float heading_size) {
    std::size_t index = 0;
    std::string plain;
    auto flush_plain = [&]() {
        if (!plain.empty()) {
            PushText(out, plain, kFollowTheme, heading_size > 0.0f, false, heading_size);
            plain.clear();
        }
    };

    while (index < text.size()) {
        // 图片：整段一行
        if (text[index] == '!' && StartsWith(text, index, "![")) {
            const std::size_t close = text.find("](", index + 2);
            const std::size_t paren = close == std::string::npos ? std::string::npos : text.find(')', close);
            if (close != std::string::npos && paren != std::string::npos) {
                const std::string alt = text.substr(index + 2, close - index - 2);
                const std::string path = Trim(text.substr(close + 2, paren - close - 2));
                flush_plain();
                const ImageAsset* asset = lookup ? lookup(path) : nullptr;
                if (asset != nullptr && asset->texture.GetTexID() != ImTextureID_Invalid) {
                    RichText::Run run;
                    run.texture = asset->texture;
                    run.image_width = asset->width;
                    run.image_height = asset->height;
                    out.push_back(std::move(run));
                } else {
                    PushText(out, "[图片缺失: " + (alt.empty() ? path : alt) + "]\n", Theme::kTextMuted, false, false,
                             0.0f);
                }
                index = paren + 1;
                continue;
            }
        }
        // 链接文本
        if (text[index] == '[') {
            const std::size_t close = text.find("](", index + 1);
            const std::size_t paren = close == std::string::npos ? std::string::npos : text.find(')', close);
            if (close != std::string::npos && paren != std::string::npos) {
                const std::string label = text.substr(index + 1, close - index - 1);
                const std::string url = text.substr(close + 2, paren - close - 2);
                flush_plain();
                PushText(out, label, options.link_color, heading_size > 0.0f, false, heading_size);
                if (options.show_link_url && !url.empty()) {
                    PushText(out, " (" + url + ")", Theme::kTextMuted, false, false, 0.0f);
                }
                index = paren + 1;
                continue;
            }
        }
        // 行内代码
        if (text[index] == '`') {
            const std::size_t close = text.find('`', index + 1);
            if (close != std::string::npos && close > index + 1) {
                flush_plain();
                PushText(out, text.substr(index + 1, close - index - 1), options.code_color, false, false, 0.0f, true);
                index = close + 1;
                continue;
            }
        }
        // 粗体 / 斜体
        if (text[index] == '*') {
            const bool strong = StartsWith(text, index, "**");
            const char* marker = strong ? "**" : "*";
            const std::size_t marker_length = strong ? 2u : 1u;
            const std::size_t close = text.find(marker, index + marker_length);
            if (close != std::string::npos && close > index + marker_length) {
                flush_plain();
                PushText(out, text.substr(index + marker_length, close - index - marker_length), kFollowTheme,
                         heading_size > 0.0f || strong, !strong, heading_size);
                index = close + marker_length;
                continue;
            }
        }
        plain += text[index];
        ++index;
    }
    flush_plain();
}

float HeadingSize(int level, const Options& options) {
    if (level <= 1) {
        return Theme::kFontTitle;
    }
    if (level == 2) {
        return Theme::kFontHeader;
    }
    return options.body_size > 0.0f ? options.body_size : Theme::kFontBody;
}

} // namespace

std::vector<std::string> ImagePaths(const std::string& text) {
    std::vector<std::string> paths;
    std::size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '!' && StartsWith(text, index, "![")) {
            const std::size_t close = text.find("](", index + 2);
            const std::size_t paren = close == std::string::npos ? std::string::npos : text.find(')', close);
            if (close != std::string::npos && paren != std::string::npos) {
                const std::string path = Trim(text.substr(close + 2, paren - close - 2));
                if (!path.empty()) {
                    bool exists = false;
                    for (const std::string& known : paths) {
                        if (known == path) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        paths.push_back(path);
                    }
                }
                index = paren + 1;
                continue;
            }
        }
        ++index;
    }
    return paths;
}

std::vector<RichText::Run> Parse(const std::string& text, const ImageLookup& lookup, const Options& options) {
    std::vector<RichText::Run> runs;
    std::size_t index = 0;
    bool in_code = false;
    std::string code_line;

    auto flush_code = [&]() {
        if (!code_line.empty()) {
            PushText(runs, code_line, options.code_color, false, false, 0.0f, true);
            code_line.clear();
        }
    };

    while (index <= text.size()) {
        std::size_t end = text.find('\n', index);
        const bool last = end == std::string::npos;
        std::string line = last ? text.substr(index) : text.substr(index, end - index);
        line = Trim(line);

        // 代码块围栏
        if (StartsWith(line, 0, "```")) {
            flush_code();
            in_code = !in_code;
            if (!in_code) {
                // 代码块结束后留一个空行
                PushText(runs, "\n", kFollowTheme);
            }
            index = last ? text.size() + 1 : end + 1;
            continue;
        }
        if (in_code) {
            code_line += "    " + line + "\n";
            index = last ? text.size() + 1 : end + 1;
            continue;
        }

        if (line.empty()) {
            // 空行 = 段落间距
            PushText(runs, "\n", kFollowTheme);
            index = last ? text.size() + 1 : end + 1;
            continue;
        }

        // 标题
        int level = 0;
        while (level < 6 && level < static_cast<int>(line.size()) && line[static_cast<std::size_t>(level)] == '#') {
            ++level;
        }
        if (level > 0 && static_cast<std::size_t>(level) < line.size() && line[static_cast<std::size_t>(level)] == ' ') {
            const float size = HeadingSize(level, options);
            const std::string content = line.substr(static_cast<std::size_t>(level) + 1);
            ParseInline(content, options, lookup, runs, size);
            PushText(runs, "\n", kFollowTheme);
            index = last ? text.size() + 1 : end + 1;
            continue;
        }

        // 无序列表 / 有序列表
        if (StartsWith(line, 0, "- ") || StartsWith(line, 0, "* ") || StartsWith(line, 0, "+ ")) {
            PushText(runs, "•  ", kFollowTheme);
            ParseInline(line.substr(2), options, lookup, runs, 0.0f);
            PushText(runs, "\n", kFollowTheme);
            index = last ? text.size() + 1 : end + 1;
            continue;
        }
        {
            std::size_t digits = 0;
            while (digits < line.size() && line[digits] >= '0' && line[digits] <= '9') {
                ++digits;
            }
            if (digits > 0 && digits + 1 < line.size() && line[digits] == '.' && line[digits + 1] == ' ') {
                PushText(runs, line.substr(0, digits) + ". ", kFollowTheme);
                ParseInline(line.substr(digits + 2), options, lookup, runs, 0.0f);
                PushText(runs, "\n", kFollowTheme);
                index = last ? text.size() + 1 : end + 1;
                continue;
            }
        }

        // 普通段落
        ParseInline(line, options, lookup, runs, 0.0f);
        PushText(runs, "\n", kFollowTheme);
        index = last ? text.size() + 1 : end + 1;
    }

    flush_code();
    return runs;
}

} // namespace gui_dev::cv::Markdown
