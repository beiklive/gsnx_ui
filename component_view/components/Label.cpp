#include "component_view/components/Label.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "component_view/Draw.h"

namespace gui_dev::cv {
namespace {

struct Utf8Char {
    const char* begin = nullptr;
    const char* end = nullptr;
};

// 按 UTF-8 边界切字符（不切断多字节序列）
Utf8Char NextChar(const char* cursor) {
    Utf8Char out;
    out.begin = cursor;
    if (cursor == nullptr || *cursor == '\0') {
        out.end = cursor;
        return out;
    }
    const char* next = cursor + 1;
    while (*next != '\0' && (static_cast<unsigned char>(*next) & 0xC0) == 0x80) {
        ++next;
    }
    out.end = next;
    return out;
}

float MeasureSpaced(ImFont* font, float font_size, const char* text, float letter_spacing, bool uppercase) {
    if (text == nullptr || text[0] == '\0') {
        return 0.0f;
    }
    if (letter_spacing <= 0.01f && !uppercase) {
        return Draw::MeasureText(font, font_size, text, 0.0f).x;
    }
    char buffer[8];
    float width = 0.0f;
    const char* cursor = text;
    while (*cursor != '\0') {
        const Utf8Char ch = NextChar(cursor);
        const std::size_t length = static_cast<std::size_t>(ch.end - ch.begin);
        if (length < sizeof(buffer)) {
            std::memcpy(buffer, ch.begin, length);
            buffer[length] = '\0';
            if (uppercase) {
                for (std::size_t i = 0; i < length; ++i) {
                    if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                        buffer[i] = static_cast<char>(buffer[i] - 'a' + 'A');
                    }
                }
            }
            width += Draw::MeasureText(font, font_size, buffer, 0.0f).x + letter_spacing;
        }
        cursor = ch.end;
    }
    return width > 0.0f ? width - letter_spacing : 0.0f;
}

void DrawSpaced(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text,
                float letter_spacing, bool uppercase, const ImVec4* clip) {
    if (text == nullptr || text[0] == '\0' || font_size <= 0.0f) {
        return;
    }
    if (letter_spacing <= 0.01f && !uppercase) {
        ImFont* use = font != nullptr ? font : Draw::CurrentFont();
        dl->AddText(use, font_size, pos, color, text, nullptr, 0.0f, clip);
        return;
    }
    char buffer[8];
    float x = pos.x;
    const char* cursor = text;
    while (*cursor != '\0') {
        const Utf8Char ch = NextChar(cursor);
        const std::size_t length = static_cast<std::size_t>(ch.end - ch.begin);
        if (length < sizeof(buffer)) {
            std::memcpy(buffer, ch.begin, length);
            buffer[length] = '\0';
            if (uppercase) {
                for (std::size_t i = 0; i < length; ++i) {
                    if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                        buffer[i] = static_cast<char>(buffer[i] - 'a' + 'A');
                    }
                }
            }
            ImFont* use = font != nullptr ? font : Draw::CurrentFont();
            dl->AddText(use, font_size, ImVec2(x, pos.y), color, buffer, nullptr, 0.0f, clip);
            x += Draw::MeasureText(font, font_size, buffer, 0.0f).x + letter_spacing;
        }
        cursor = ch.end;
    }
}

} // namespace

Label::Label() : Widget("label") {}

Label::Label(std::string value, float size, ImU32 text_color) : Widget("label") {
    text = std::move(value);
    font_size = size;
    color = text_color;
}

Label& Label::SetText(std::string value) {
    text = std::move(value);
    return *this;
}

Label& Label::SetColor(ImU32 value) {
    color = value;
    return *this;
}

Label& Label::SetFontSize(float value) {
    font_size = value;
    return *this;
}

Label& Label::SetWeight(int value) {
    font_weight = value;
    return *this;
}

Label& Label::SetAlign(TextAlign horizontal, VerticalAlign vertical) {
    text_align = horizontal;
    vertical_align = vertical;
    return *this;
}

Label& Label::SetWrap(float width) {
    wrap_width = width;
    if (width > 0.0f) {
        single_line = false;
    }
    return *this;
}

Label& Label::SetShadow(ImU32 value) {
    shadow_color = value;
    return *this;
}

Label& Label::SetOutline(ImU32 value) {
    outline_color = value;
    return *this;
}

Label& Label::SetSpacing(float letter, float line) {
    letter_spacing = letter;
    line_spacing = line;
    return *this;
}

Label& Label::SetEllipsis(bool value) {
    ellipsis = value;
    return *this;
}

Label& Label::SetMarquee(bool value, float speed) {
    marquee = value;
    marquee_speed = speed;
    single_line = true;
    return *this;
}

Label& Label::SetFocusOverride(std::string text_when_focused, ImU32 color_when_focused) {
    focus_text = std::move(text_when_focused);
    focus_color = color_when_focused;
    return *this;
}

const char* Label::ResolvedText() const {
    if (focused && !focus_text.empty()) {
        return focus_text.c_str();
    }
    return text.c_str();
}

ImU32 Label::ResolvedColor() const {
    if (focused && ((focus_color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
        return focus_color;
    }
    return color;
}

float Label::EffectiveWrap(float available) const {
    if (wrap_width > 0.0f) {
        return wrap_width;
    }
    if (size.x > 0.0f) {
        return Maxf(size.x - padding.Horizontal(), 1.0f);
    }
    if (wrap_to_available) {
        return Maxf(available, 1.0f);
    }
    return 0.0f;
}

std::vector<std::string> Label::LayoutLines(float available_width) const {
    std::vector<std::string> lines;
    const char* source = ResolvedText();
    const float font_size_now = ResolvedFontSize();
    const float wrap = EffectiveWrap(available_width);
    if (source == nullptr) {
        return lines;
    }
    if (single_line || wrap <= 1.0f) {
        lines.emplace_back(source);
        return lines;
    }

    std::string current;
    std::string word;
    const char* cursor = source;
    while (true) {
        if (*cursor == '\n' || *cursor == '\0') {
            current += word;
            word.clear();
            lines.push_back(current);
            current.clear();
            if (*cursor == '\0') {
                break;
            }
            ++cursor;
            continue;
        }
        const Utf8Char ch = NextChar(cursor);
        const std::size_t length = static_cast<std::size_t>(ch.end - ch.begin);
        const std::string piece(ch.begin, ch.end);
        cursor = ch.end;

        // 英文按单词断行，CJK 逐字断行
        const bool is_space = piece == " ";
        word += piece;
        const std::string probe = current + word;
        if (MeasureSpaced(font, font_size_now, probe.c_str(), letter_spacing, uppercase) > wrap) {
            if (!is_space && length > 1) {
                // CJK：单字成行
                if (!current.empty()) {
                    lines.push_back(current);
                    current.clear();
                }
                current = piece;
                word.clear();
            } else {
                if (!current.empty()) {
                    lines.push_back(current);
                }
                current = word;
                while (!current.empty() && current.front() == ' ') {
                    current.erase(current.begin());
                }
                word.clear();
            }
        }
    }
    if (lines.empty()) {
        lines.emplace_back(source);
    }
    return lines;
}

ImVec2 Label::MeasureContent(const ImVec2& available) {
    const float size_now = ResolvedFontSize();
    const char* source = ResolvedText();
    if (single_line) {
        const float width = MeasureSpaced(font, size_now, source, letter_spacing, uppercase);
        return ImVec2(width, size_now);
    }
    const std::vector<std::string> lines = LayoutLines(available.x);
    float width = 0.0f;
    for (const std::string& line : lines) {
        width = Maxf(width, MeasureSpaced(font, size_now, line.c_str(), letter_spacing, uppercase));
    }
    return ImVec2(width, LineHeight() * static_cast<float>(lines.size()));
}

void Label::OnUpdate(float dt) {
    if (!marquee) {
        marquee_active_ = false;
        return;
    }
    // 只有超宽时才滚动；滚完停一下再从头开始
    const float text_width = MeasureSpaced(font, ResolvedFontSize(), ResolvedText(), letter_spacing, uppercase);
    const float available = content_rect.Width();
    marquee_active_ = text_width > available + 1.0f;
    if (marquee_active_) {
        marquee_time_ += dt;
    } else {
        marquee_time_ = 0.0f;
    }
}

void Label::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const char* source = ResolvedText();
    if (source == nullptr || source[0] == '\0') {
        return;
    }
    const float size_now = ResolvedFontSize();
    const ImU32 foreground = Tint(ResolvedColor());
    const bool bold = font_weight >= 600;

    auto paint = [&](const std::string& line, const ImVec2& pos, const ImVec4* clip) {
        if (((shadow_color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
            DrawSpaced(dl, font, size_now, ImVec2(pos.x + 1.0f, pos.y + 1.0f), Tint(shadow_color), line.c_str(),
                       letter_spacing, uppercase, clip);
        }
        if (((outline_color >> IM_COL32_A_SHIFT) & 0xFF) != 0) {
            for (int dy = -1; dy <= 1; dy += 2) {
                for (int dx = -1; dx <= 1; dx += 2) {
                    DrawSpaced(dl, font, size_now, ImVec2(pos.x + static_cast<float>(dx), pos.y + static_cast<float>(dy)),
                               Tint(outline_color), line.c_str(), letter_spacing, uppercase, clip);
                }
            }
        }
        DrawSpaced(dl, font, size_now, pos, foreground, line.c_str(), letter_spacing, uppercase, clip);
        if (bold) {
            DrawSpaced(dl, font, size_now, ImVec2(pos.x + 0.6f, pos.y), Tint(Theme::Alpha(ResolvedColor(), 0.55f)),
                       line.c_str(), letter_spacing, uppercase, clip);
        }
        if (disabled_strikethrough && !enabled) {
            const float width = MeasureSpaced(font, size_now, line.c_str(), letter_spacing, uppercase);
            dl->AddLine(ImVec2(pos.x, pos.y + size_now * 0.55f), ImVec2(pos.x + width, pos.y + size_now * 0.55f),
                        foreground, 1.0f);
        }
    };

    if (single_line) {
        std::string line(source);
        ImVec4 clip;
        const ImVec4* clip_ptr = nullptr;
        float offset_x = 0.0f;

        if (ellipsis && !marquee_active_) {
            line = Draw::Ellipsize(font, size_now, source, content.Width());
        }
        if (marquee_active_) {
            // 滚动条：进入→滚出→停顿，循环
            const float text_width = MeasureSpaced(font, size_now, source, letter_spacing, uppercase);
            const float span = text_width - content.Width() + 24.0f;
            const float cycle = (span + marquee_hold * marquee_speed) / marquee_speed;
            const float phase = std::fmod(marquee_time_, cycle);
            const float travel = Clampf((phase - marquee_hold) * marquee_speed, 0.0f, span);
            offset_x = -travel;
            clip = ImVec4(content.min.x, content.min.y, content.max.x, content.max.y);
            clip_ptr = &clip;
        }

        const float width = MeasureSpaced(font, size_now, line.c_str(), letter_spacing, uppercase);
        const float height = size_now;
        ImVec2 pos = content.min;
        switch (text_align) {
        case TextAlign::Left:
            pos.x = content.min.x;
            break;
        case TextAlign::Center:
            pos.x = content.Center().x - width * 0.5f;
            break;
        case TextAlign::Right:
            pos.x = content.max.x - width;
            break;
        }
        pos.x += offset_x;
        switch (vertical_align) {
        case VerticalAlign::Top:
            pos.y = content.min.y;
            break;
        case VerticalAlign::Middle:
            pos.y = content.Center().y - height * 0.5f;
            break;
        case VerticalAlign::Bottom:
            pos.y = content.max.y - height;
            break;
        }
        paint(line, pos, clip_ptr);
        return;
    }

    const std::vector<std::string> lines = LayoutLines(content.Width());
    const float line_height = LineHeight();
    const float total_height = line_height * static_cast<float>(lines.size());
    float y = content.min.y;
    switch (vertical_align) {
    case VerticalAlign::Top:
        y = content.min.y;
        break;
    case VerticalAlign::Middle:
        y = content.Center().y - total_height * 0.5f;
        break;
    case VerticalAlign::Bottom:
        y = content.max.y - total_height;
        break;
    }
    for (const std::string& line : lines) {
        const float width = MeasureSpaced(font, size_now, line.c_str(), letter_spacing, uppercase);
        float x = content.min.x;
        switch (text_align) {
        case TextAlign::Left:
            x = content.min.x;
            break;
        case TextAlign::Center:
            x = content.Center().x - width * 0.5f;
            break;
        case TextAlign::Right:
            x = content.max.x - width;
            break;
        }
        paint(line, ImVec2(x, y), nullptr);
        y += line_height;
    }
}

} // namespace gui_dev::cv
