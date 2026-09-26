// Markdown：把一小段 Markdown 解析成 RichText 的 runs（文字 + 图片）。
//
// 范围（按规范）刻意做小：标题、粗体、斜体、列表、链接文本、行内代码、代码块、换行、图片。
// 不做表格 / 图表 / Mermaid / 数学公式 / HTML / WebView —— 这里不是网页渲染引擎。
//
// 用法（两步，因为控件层不碰平台接口，纹理由宿主加载）：
//
//   const std::string md = LoadReadme();
//   for (const std::string& path : Markdown::ImagePaths(md)) {
//       assets[path] = LoadTexture(path);          // 宿主：走 Backend
//   }
//   auto lookup = [&assets](const std::string& path) -> const Markdown::ImageAsset* { ... };
//   popup->setRichText(Markdown::Parse(md, lookup), 260.0f);
//
// 图片以「块」的形式独占一行（等比缩放到可用宽度，可居中），文字自动换行由 RichText 负责。
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/components/Content.h"

namespace gui_dev::cv::Markdown {

// 宿主解析出来的图片资源（纹理 + 原始像素尺寸）
struct ImageAsset {
    std::string path;
    ImTextureRef texture{};
    float width = 0.0f;
    float height = 0.0f;
};

// 按路径查图片：没有就返回 nullptr（会退化成一行 [图片缺失] 的提示文字）
using ImageLookup = std::function<const ImageAsset*(const std::string& path)>;

struct Options {
    float body_size = 0.0f;        // 0 = Theme::kFontBody
    float max_image_width = 0.0f;  // 0 = 用内容宽度
    ImVec4 link_color = Theme::kBlue;      // 链接文本色（跟主题无关的语义色）
    ImVec4 code_color = Theme::kTextPrimary;
    bool show_link_url = true;             // 链接后面用浅色补一个 (url)，方便在主机上看清楚
};

// 第一遍：收集所有 ![alt](path) 里的 path（去重，保持出现顺序）
std::vector<std::string> ImagePaths(const std::string& text);

// 第二遍：解析成 runs（文字 + 图片块）
std::vector<RichText::Run> Parse(const std::string& text, const ImageLookup& lookup = {},
                                 const Options& options = {});

} // namespace gui_dev::cv::Markdown
