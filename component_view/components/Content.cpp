#include "component_view/components/Content.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

// 内容区可用宽度（外框尺寸已给定时按它算，否则用父给的 available）
float ContentWidthFor(const Widget& widget, const ImVec2& available) {
    const float chrome = widget.padding.Horizontal() + (widget.border.width + widget.border.inset) * 2.0f;
    if (widget.size.x > 0.0f) {
        return Maxf(widget.size.x - chrome, 1.0f);
    }
    return Maxf(available.x, 1.0f);
}

// UTF-8：返回 [index, next) 这个字符的字节长度（>=1）
std::size_t Utf8CharLength(const std::string& text, std::size_t index) {
    std::size_t next = index + 1;
    while (next < text.size() && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
        ++next;
    }
    return next - index;
}

bool Utf8IsSpace(const std::string& text, std::size_t index) {
    return text[index] == ' ' || text[index] == '\t';
}

// 中日韩字符可以逐字换行；拉丁字母/数字要连着当词
bool Utf8IsWide(const std::string& text, std::size_t index) {
    return static_cast<unsigned char>(text[index]) >= 0xE0;
}

// 用 imgui 自己的断行算法把一段文本拆成行（CJK / 拉丁都正确，含显式 '\n'）。
// 返回的每行是 [begin,end) 指针，直接喂给 Draw::Text 的 text_end 用。
std::vector<std::pair<const char*, const char*>> WrapLines(ImFont* font, float font_size, const char* begin,
                                                           const char* end, float wrap_width) {
    std::vector<std::pair<const char*, const char*>> lines;
    if (font == nullptr || begin == nullptr || end == nullptr || begin >= end) {
        return lines;
    }
    const char* paragraph = begin;
    while (paragraph <= end) {
        const char* newline = static_cast<const char*>(std::memchr(paragraph, '\n', static_cast<std::size_t>(end - paragraph)));
        const char* paragraph_end = newline != nullptr ? newline : end;
        if (wrap_width <= 0.0f) {
            lines.emplace_back(paragraph, paragraph_end);
        } else {
            const char* cursor = paragraph;
            while (cursor < paragraph_end) {
                
                const char* wrap_at = font->CalcWordWrapPosition(font_size, cursor, paragraph_end, wrap_width);
                if (wrap_at <= cursor) {
                    wrap_at = cursor + 1; // 宽度极小时防死循环
                }
                lines.emplace_back(cursor, wrap_at);
                cursor = wrap_at;
                while (cursor < paragraph_end && *cursor == ' ') {
                    ++cursor; // 行首空格不画
                }
            }
            if (paragraph == paragraph_end) {
                lines.emplace_back(paragraph, paragraph_end); // 空行
            }
        }
        if (newline == nullptr) {
            break;
        }
        paragraph = newline + 1;
    }
    return lines;
}

void DrawGlyphText(ImDrawList* dl, ImFont* font, float font_size, const ImVec2& pos, ImU32 color, const char* text,
                   const char* text_end, bool bold) {
    
    dl->AddText(font, font_size, pos, color, text, text_end, 0.0f);
    if (bold) {
        // 只有一套字重：半像素偏移再画一遍模拟粗体
        dl->AddText(font, font_size, ImVec2(pos.x + 0.6f, pos.y), color, text, text_end, 0.0f);
    }
}

std::size_t HashCombine(std::size_t seed, std::size_t value) {
    return seed ^ (value + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2));
}

} // namespace

// ================================================================= Label =====

Label::Label() : Widget("label") {
    text_color = Theme::kTextPrimary;
}

Label::Label(std::string value) : Label() {
    text = std::move(value);
}

Label& Label::setText(std::string value) {
    text = std::move(value);
    return *this;
}

Label& Label::setIcon(std::string glyph) {
    icon = std::move(glyph);
    return *this;
}

Label& Label::setFontSize(float value) {
    font_size = value;
    return *this;
}

Label& Label::setColor(ImVec4 color) {
    text_color = color;
    text_color_follows_theme = false;
    return *this;
}

Label& Label::setAlign(TextAlign value) {
    text_align = value;
    return *this;
}

Label& Label::setVerticalAlign(VerticalAlign value) {
    vertical_align = value;
    return *this;
}

Label& Label::setWrap(bool value, float gap) {
    wrap = value;
    line_gap = gap;
    return *this;
}

float Label::ResolvedFontSize() const {
    return font_size > 0.0f ? font_size : Theme::kFontBody;
}

float Label::LineHeight() const {
    return Draw::MeasureText(Draw::CurrentFont(), ResolvedFontSize(), "Ag", 0.0f).y;
}

ImVec2 Label::TextSize(float wrap_width) const {
    const float size = ResolvedFontSize();
    ImFont* font = Draw::CurrentFont();
    if (!wrap || wrap_width <= 0.0f) {
        return Draw::MeasureText(font, size, text.c_str(), 0.0f);
    }
    const auto lines = WrapLines(font, size, text.c_str(), text.c_str() + text.size(), wrap_width);
    if (lines.empty()) {
        return ImVec2(0.0f, LineHeight());
    }
    float width = 0.0f;
    for (const auto& line : lines) {
        // 只量这一行的子串：Draw::MeasureText 第 4 个参数是 wrap_width（不是长度），
        // 传字节长度会变成"按字节数换行"，量出来的宽度会明显偏小。
        const std::string line_text(line.first, line.second);
        width = Maxf(width, Draw::MeasureText(font, size, line_text.c_str(), 0.0f).x);
    }
    const float height = static_cast<float>(lines.size()) * LineHeight() +
                         static_cast<float>(lines.size() - 1) * line_gap;
    return ImVec2(width, height);
}

int Label::CountLines(float wrap_width, std::vector<std::string>* out_lines) const {
    if (text.empty()) {
        return 0;
    }
    const float size = ResolvedFontSize();
    ImFont* font = Draw::CurrentFont();
    if (!wrap || wrap_width <= 0.0f) {
        int lines = 1;
        for (char c : text) {
            if (c == '\n') {
                ++lines;
            }
        }
        if (out_lines != nullptr) {
            out_lines->clear();
            out_lines->push_back(text);
        }
        return lines;
    }
    const auto wrapped = WrapLines(font, size, text.c_str(), text.c_str() + text.size(), wrap_width);
    if (out_lines != nullptr) {
        out_lines->clear();
        for (const auto& line : wrapped) {
            out_lines->emplace_back(line.first, static_cast<std::size_t>(line.second - line.first));
        }
    }
    return static_cast<int>(wrapped.size());
}

ImVec2 Label::MeasureContent(const ImVec2& available) {
    const float size = ResolvedFontSize();
    const float wrap_width = wrap ? ContentWidthFor(*this, available) : 0.0f;
    ImVec2 measured = TextSize(wrap_width);
    if (!icon.empty()) {
        const float icon_width = Draw::MeasureText(Draw::CurrentFont(), size, icon.c_str(), 0.0f).x;
        measured.x += icon_width + icon_gap;
    }
    if (max_lines > 0.0f) {
        const float max_height = max_lines * LineHeight() + (max_lines - 1.0f) * line_gap;
        measured.y = Minf(measured.y, max_height);
    }
    return measured;
}

void Label::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (text.empty() && icon.empty()) {
        return;
    }
    ImFont* font = Draw::CurrentFont();
    const float size = ResolvedFontSize();
    const ImU32 color = Theme::Alpha(Theme::U32(text_color), EffectiveOpacity());
    const float line_height = Maxf(LineHeight(), 1.0f);

    const float wrap_width = wrap ? content.Width() : 0.0f;
    std::vector<std::string> line_storage;
    const int line_count = CountLines(wrap_width, &line_storage);
    const float block_height = static_cast<float>(Maxf(line_count, 1)) * line_height +
                               static_cast<float>(Maxf(line_count - 1, 0)) * line_gap;
    const float icon_width = icon.empty() ? 0.0f : Draw::MeasureText(font, size, icon.c_str(), 0.0f).x;
    float block_width = icon_width + (icon.empty() ? 0.0f : icon_gap);
    if (!line_storage.empty()) {
        for (const std::string& line : line_storage) {
            block_width = Maxf(block_width, Draw::MeasureText(font, size, line.c_str(), 0.0f).x +
                                                 (icon.empty() ? 0.0f : icon_width + icon_gap));
        }
    }

    float x = content.min.x;
    if (text_align == TextAlign::Center) {
        x = content.min.x + (content.Width() - block_width) * 0.5f;
    } else if (text_align == TextAlign::Right) {
        x = content.max.x - block_width;
    }
    float y = content.min.y;
    if (vertical_align == VerticalAlign::Middle) {
        y = content.min.y + (content.Height() - block_height) * 0.5f;
    } else if (vertical_align == VerticalAlign::Bottom) {
        y = content.max.y - block_height;
    }

    if (wrap) {
        dl->PushClipRect(content.min, content.max, true);
    }

    float text_x = x;
    if (!icon.empty()) {
        Draw::Text(dl, font, size, ImVec2(x, y), color, icon.c_str(), 0.0f);
        text_x = x + icon_width + icon_gap;
    }

    if (wrap) {
        int drawn = 0;
        for (const std::string& line : line_storage) {
            if (max_lines > 0.0f && static_cast<float>(drawn) >= max_lines) {
                break;
            }
            Draw::Text(dl, font, size, ImVec2(text_x, y), color, line.c_str(), 0.0f);
            y += line_height + line_gap;
            ++drawn;
        }
        dl->PopClipRect();
        return;
    }

    // 单行：跑马灯优先于省略号
    const float available_width = Maxf(content.max.x - text_x, 1.0f);
    const float text_width = Draw::MeasureText(font, size, text.c_str(), 0.0f).x;
    if (marquee && text_width > available_width) {
        Draw::MarqueeText(dl, font, size,
                          Rect{ImVec2(text_x, y), ImVec2(content.max.x, y + line_height)}, color, text.c_str(),
                          Global::time, marquee_speed);
        return;
    }
    if (ellipsize && text_width > available_width) {
        dl->PushClipRect(content.min, content.max, true);
        Draw::Text(dl, font, size, ImVec2(text_x, y), color,
                   Draw::Ellipsize(font, size, text.c_str(), available_width), 0.0f);
        dl->PopClipRect();
        return;
    }
    Draw::Text(dl, font, size, ImVec2(text_x, y), color, text.c_str(), 0.0f);
}

void Label::OnThemeChanged() {
    if (text_color_follows_theme) {
        text_color = Theme::kTextPrimary;
    }
}

// ============================================================= Separator =====

Separator::Separator() : Widget("separator") {}

Separator::Separator(Orientation value) : Separator() {
    orientation = value;
}

Separator& Separator::setThickness(float value) {
    thickness = value;
    return *this;
}

Separator& Separator::setLength(float value) {
    length = value;
    return *this;
}

Separator& Separator::setInset(float start, float end) {
    inset_start = start;
    inset_end = end < 0.0f ? start : end;
    return *this;
}

Separator& Separator::setColor(ImVec4 value) {
    color = value;
    color_follows_theme = false;
    return *this;
}

ImVec2 Separator::MeasureContent(const ImVec2& available) {
    if (orientation == Orientation::Horizontal) {
        return ImVec2(length > 0.0f ? length : Maxf(available.x, 1.0f), Maxf(thickness, 1.0f));
    }
    return ImVec2(Maxf(thickness, 1.0f), length > 0.0f ? length : Maxf(available.y, 1.0f));
}

void Separator::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const ImVec4 base = (color_follows_theme || color.w <= 0.0f) ? Theme::kBorder : color;
    const ImU32 line_color = Theme::Alpha(Theme::U32(base), EffectiveOpacity());
    if (orientation == Orientation::Horizontal) {
        const float y = (content.min.y + content.max.y) * 0.5f;
        dl->AddLine(ImVec2(content.min.x + inset_start, y), ImVec2(content.max.x - inset_end, y), line_color,
                    Maxf(thickness, 1.0f));
    } else {
        const float x = (content.min.x + content.max.x) * 0.5f;
        dl->AddLine(ImVec2(x, content.min.y + inset_start), ImVec2(x, content.max.y - inset_end), line_color,
                    Maxf(thickness, 1.0f));
    }
}

void Separator::OnThemeChanged() {
    if (color_follows_theme) {
        color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// =========================================================== ProgressBar =====

ProgressBar::ProgressBar() : Widget("progress_bar") {
    text_color = Theme::kTextMuted;
    radius = Global::component_style.corner_radius; // 圆角跟全局 Box/Button 一致
}

ProgressBar& ProgressBar::setValue(float next) {
    value = Clampf(next, 0.0f, 1.0f);
    return *this;
}

ProgressBar& ProgressBar::setLabel(std::string next) {
    label = std::move(next);
    show_label = !label.empty();
    return *this;
}

ProgressBar& ProgressBar::setIndeterminate(bool enabled) {
    indeterminate = enabled;
    return *this;
}

ProgressBar& ProgressBar::setBarHeight(float next) {
    bar_height = next;
    return *this;
}

ProgressBar& ProgressBar::setColors(ImVec4 fill, ImVec4 track) {
    fill_color = fill;
    track_color = track;
    return *this;
}

ImVec2 ProgressBar::MeasureContent(const ImVec2& available) {
    const float font_px = font_size > 0.0f ? font_size : Theme::kFontSmall;
    const float text_height =
        (show_label || show_percent) ? Draw::MeasureText(Draw::CurrentFont(), font_px, "0", 0.0f).y : 0.0f;
    const float width = size.x > 0.0f ? size.x : Maxf(available.x, 220.0f);
    const float height = text_height + (text_height > 0.0f ? label_gap : 0.0f) + Maxf(bar_height, 2.0f);
    return ImVec2(width, height);
}

void ProgressBar::OnUpdate(float dt) {
    shown_ = smooth_speed > 0.0f ? Anim::SmoothTo(shown_, value, smooth_speed, dt) : value;
    if (indeterminate) {
        phase_ += dt * 0.55f;
        while (phase_ > 1.0f) {
            phase_ -= 1.0f;
        }
    }
}

void ProgressBar::OnDrawContent(ImDrawList* dl, const Rect& content) {
    ImFont* font = Draw::CurrentFont();
    const float size = font_size > 0.0f ? font_size : Theme::kFontSmall;
    const ImVec4 fill = fill_color.w <= 0.0f ? Theme::kAccent : fill_color;
    const ImVec4 track = track_color.w <= 0.0f ? Theme::kTrack : track_color;
    const float opacity = EffectiveOpacity();

    float bar_top = content.min.y;
    if (show_label || show_percent) {
        const float text_height = Draw::MeasureText(font, size, "0", 0.0f).y;
        const ImU32 text_color_u32 = Theme::Alpha(Theme::U32(text_color), opacity);
        if (show_label && !label.empty()) {
            const char* shown = Draw::Ellipsize(font, size, label.c_str(), content.Width() * 0.72f);
            Draw::Text(dl, font, size, ImVec2(content.min.x, content.min.y), text_color_u32, shown, 0.0f);
        }
        if (show_percent && !indeterminate) {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(shown_ * 100.0f + 0.5f));
            const float width = Draw::MeasureText(font, size, buffer, 0.0f).x;
            Draw::Text(dl, font, size, ImVec2(content.max.x - width, content.min.y), text_color_u32, buffer, 0.0f);
        }
        bar_top = content.min.y + text_height + label_gap;
    }

    const Rect bar{ImVec2(content.min.x, bar_top), ImVec2(content.max.x, bar_top + Maxf(bar_height, 2.0f))};
    if (!bar.Valid()) {
        return;
    }
    const float rounding = Clampf(radius, 0.0f, bar.Height() * 0.5f);
    Draw::RoundedRectFilled(dl, bar, Theme::Alpha(Theme::U32(track), opacity), rounding, rounding, rounding, rounding);

    if (indeterminate) {
        const float band = bar.Width() * 0.38f;
        const float travel = bar.Width() + band;
        const float x = bar.min.x - band + travel * phase_;
        const Rect slice{ImVec2(Maxf(x, bar.min.x), bar.min.y), ImVec2(Minf(x + band, bar.max.x), bar.max.y)};
        if (slice.Valid()) {
            Draw::RoundedRectFilled(dl, slice, Theme::Alpha(Theme::U32(fill), opacity), rounding, rounding, rounding,
                                    rounding);
        }
        return;
    }

    const float fill_width = bar.Width() * Clampf(shown_, 0.0f, 1.0f);
    if (fill_width <= 0.5f) {
        return;
    }
    const Rect filled{bar.min, ImVec2(bar.min.x + fill_width, bar.max.y)};
    Draw::RoundedRectFilled(dl, filled, Theme::Alpha(Theme::U32(fill), opacity), rounding, rounding, rounding, rounding);
}

void ProgressBar::OnThemeChanged() {
    if (text_color_follows_theme) {
        text_color = Theme::kTextMuted;
    }
    radius = Global::component_style.corner_radius;
}

// ================================================================ Image =====

Image::Image() : Widget("image") {
    tint = Theme::kWhite;
    radius = Global::component_style.corner_radius; // 与 Box / Button 同一套圆角
}

Image& Image::setTexture(ImTextureRef ref, float width, float height) {
    texture = ref;
    source_width = width;
    source_height = height;
    return *this;
}

Image& Image::setFit(Fit value) {
    fit = value;
    return *this;
}

Image& Image::setRadius(float value) {
    radius = value;
    return *this;
}

Image& Image::setTint(ImVec4 value) {
    tint = value;
    tint_follows_theme = false;
    return *this;
}

Rect Image::FitRect(const Rect& content) const {
    if (source_width <= 0.0f || source_height <= 0.0f) {
        return content;
    }
    const float aspect = source_width / source_height;
    if (fit == Fit::Stretch) {
        return content;
    }
    if (fit == Fit::None) {
        const ImVec2 center = content.Center();
        return Rect{ImVec2(center.x - source_width * 0.5f, center.y - source_height * 0.5f),
                    ImVec2(center.x + source_width * 0.5f, center.y + source_height * 0.5f)};
    }
    const float content_aspect = content.Height() > 0.0f ? content.Width() / content.Height() : aspect;
    float width = content.Width();
    float height = content.Height();
    const bool use_width = (fit == Fit::Contain) ? (aspect > content_aspect) : (aspect < content_aspect);
    if (use_width) {
        height = width / aspect;
    } else {
        width = height * aspect;
    }
    const ImVec2 center = content.Center();
    return Rect{ImVec2(center.x - width * 0.5f, center.y - height * 0.5f),
                ImVec2(center.x + width * 0.5f, center.y + height * 0.5f)};
}

ImVec2 Image::MeasureContent(const ImVec2& available) {
    if (source_width <= 0.0f || source_height <= 0.0f) {
        return ImVec2(Minf(Maxf(available.x, 1.0f), 240.0f), 160.0f);
    }
    // 上限 = min(可用空间, max_size)：max_size 不参与的话，图片会一直撑到 available，
    // 而 available 又来自父级自适应高度 → 弹窗高度每帧变化（图片与按钮上下抖）。
    float max_width = Maxf(available.x, 1.0f);
    float max_height = Maxf(available.y > 0.0f ? available.y : source_height, 1.0f);
    if (max_size.x > 0.0f) {
        max_width = Minf(max_width, max_size.x);
    }
    if (max_size.y > 0.0f) {
        max_height = Minf(max_height, max_size.y);
    }
    const float scale = Minf(max_width / source_width, max_height / source_height);
    const float scale_use = fit == Fit::None ? 1.0f : Maxf(scale, 0.0001f);
    return ImVec2(source_width * scale_use, source_height * scale_use);
}

void Image::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float opacity = EffectiveOpacity();
    if (texture.GetTexID() == ImTextureID_Invalid || source_width <= 0.0f || source_height <= 0.0f) {
        if (!show_placeholder) {
            return;
        }
        Draw::RoundedRectFilled(dl, content, Theme::Alpha(Theme::U32(Theme::kBgWidget), opacity * 0.6f), radius, radius,
                                radius, radius);
        Draw::RoundedRectOutline(dl, content, Theme::Alpha(Theme::U32(Theme::kBorder), opacity), 1.0f, radius, radius,
                                 radius, radius);
        const char* glyph = Icons::Glyph(Icons::Material::ImagePlaceholder);
        const float size = Minf(28.0f, Maxf(content.Height() * 0.4f, 12.0f));
        const ImVec2 measured = Draw::MeasureText(Draw::CurrentFont(), size, glyph, 0.0f);
        Draw::Text(dl, Draw::CurrentFont(), size,
                   ImVec2(content.Center().x - measured.x * 0.5f, content.Center().y - measured.y * 0.5f),
                   Theme::Alpha(Theme::U32(Theme::kTextMuted), opacity), glyph, 0.0f);
        return;
    }
    const Rect target = FitRect(content);
    if (!target.Valid()) {
        return;
    }
    // 基本裁剪：Cover / 原尺寸时贴图会超出内容区，超出的部分裁掉（不外溢到别的控件上）
    const bool needs_clip = target.min.x < content.min.x - 0.5f || target.min.y < content.min.y - 0.5f ||
                            target.max.x > content.max.x + 0.5f || target.max.y > content.max.y + 0.5f;
    if (needs_clip) {
        dl->PushClipRect(content.min, content.max, true);
    }
    dl->AddImageRounded(texture, target.min, target.max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                        Theme::Alpha(Theme::U32(tint), opacity), radius, ImDrawFlags_None);
    if (needs_clip) {
        dl->PopClipRect();
    }
}

void Image::OnThemeChanged() {
    if (tint_follows_theme) {
        tint = Theme::kWhite;
    }
    radius = Global::component_style.corner_radius;
}

} // namespace gui_dev::cv
