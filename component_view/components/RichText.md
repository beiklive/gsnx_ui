# RichText

单文件富文本显示组件：`component_view/components/RichText.h`

---

## 1. 简介

**RichText 是什么**：一个轻量、零第三方依赖的富文本 **显示** 控件，输入是 Markdown 的一个子集
（外加一套 `[color=#RRGGBB]` 染色语法），输出是画在现有 ImGui DrawList 上的一行行 glyph。

**为什么实现**：弹窗内容、游戏说明、更新日志、Core 信息、错误说明、关于页都需要「带样式的文字 + 图片」，
但完整 Markdown 库（CommonMark / imgui_markdown / MD4C）会带来渲染模型或依赖上的额外约束；
本组件只保留框架真正需要的那一小部分语法，并把解析 / 布局 / 渲染分成三层，便于在 Switch 上控制成本。

**设计目标**

| 目标 | 做法 |
|---|---|
| 融入现有体系 | 继承 `Widget`，只用 `Draw::` / `Theme::` / `Global::component_style`，不新造视觉语言 |
| 零依赖 | 只用 C++ 标准库 + 现有框架 + Dear ImGui |
| 单文件 | 全部实现在 `RichText.h` 里（`inline`），没有第二个文件 |
| 显示而非编辑 | 无光标 / 选择 / 剪贴板 / 撤销 |
| 适合主机 | 解析一次、布局按需、渲染每帧只走已排好的列表 |
| 不越界 | 不做滚动容器、不做焦点系统、不直接开浏览器、不读磁盘 |

---

## 2. 特性

* Markdown：标题、粗体、斜体、粗斜体、删除线、行内代码、无序/有序列表、引用、分隔线、链接、图片
* 染色：`[color=#RRGGBB]` / `[color=#RRGGBBAA]`，可嵌套，内部仍可带样式
* 自动换行：按真实字体测量，中文逐字断行、英文整词断行（UTF-8 安全）
* Inline Image：图片可以出现在文字中间，并参与这一行的排版
* 图片等比缩放 / 居中 / 限宽限高 / 缺图占位
* 解析缓存 + 布局缓存 + 图片解析缓存
* 链接回调（鼠标 / 触摸点击）+ 可选信号 `linkActivated`
* 完全使用现有 Theme（正文色 / 强调色 / 链接色 / 边框色 / 代码底色 / 间距），无硬编码色值

---

## 3. Markdown 支持表

| Syntax | Support | 说明 |
|---|---|---|
| 普通文本 | ✅ | 中文 / English / 数字 / 混排 |
| 段落 / 换行 | ✅ | 一行即一段（单换行也换行，避免中英被拼在一起） |
| `#` ~ `######` 标题 | ✅ | 字号来自 Theme；`####` 以上按三级样式 |
| `**粗体**` / `__粗体__` | ✅ | 当前只有一套字重 → 半像素偏移重绘模拟 |
| `*斜体*` / `_斜体_` | ✅ | 顶点绕基线剪切模拟斜体 |
| `***粗斜体***` | ✅ | 粗体 + 斜体叠加 |
| `~~删除线~~` | ✅ | 文字上加一条横线 |
| `` `行内代码` `` | ✅ | 加底色带（项目无等宽字体，用正文字体） |
| ```` ```代码块``` ```` | ❌ | 已按反馈移除（项目没有等宽字体，显示效果不理想） |
| `-` / `*` / `+` 无序列表 | ✅ | 最多两级缩进（每 2 空格一级） |
| `1.` 有序列表 | ✅ | 保留原编号 |
| `>` 引用 | ✅ | 左侧竖线 |
| `---` / `***` / `___` 分隔线 | ✅ | 用 Theme 的边框色 |
| `[文字](url)` 链接 | ✅ | 只显示 + 回调，不直接开浏览器 |
| `![alt](path)` 图片 | ✅ | 独占时居中；也可行内 |
| `[color=#RRGGBB]…[/color]` | ✅ | 支持 6/8 位十六进制，可嵌套其它样式 |
| 嵌套列表 | ✅ | 两级（文档明确限制） |
| Table | ❌ | 不做 |
| HTML | ❌ | 不做 |
| Mermaid / LaTeX | ❌ | 不做 |
| 任务列表 / 脚注 / 自动链接 | ❌ | 不做 |

---

## 4. API

组件是 `Widget` 子类，遵循框架命名：绘制走 `OnDrawContent`（不是自己造 `Draw()`），
链式 setter 用 `SetXxx`。

```cpp
#include "component_view/components/RichText.h"
using gui_dev::cv::RichText;

RichText* text = parent.Emplace<RichText>();
text->SetMarkdown(md)                       // 设置/替换内容（触发重新解析）
     .SetImageResolver(resolver)            // 图片解析回调
     .SetLinkCallback(callback)             // 链接点击回调
     .SetTextScale(1.0f)                    // 整体字号缩放
     .SetLineGap(6.0f)                      // 行距（默认 Theme::kGapSmall/2）
     .SetParagraphGap(10.0f)                // 段间距（默认 Theme::kGap）
     .SetMaxImageSize(0.0f, 220.0f)         // 0 = 不限
     .SetImagePlaceholder(true)             // 缺图画占位
     .SetAutoHeight(true)                   // 自撑高（滚动容器需要内容比视口高）
     .SetScrollKeys(false);                 // 上下键是否滚外部容器（默认 false）

text->size.x = width;                       // 宽度由布局给
// 只读结果
float h = text->ContentHeight();            // 内容高度（排版后）
```

| API | 作用 |
|---|---|
| `SetMarkdown(std::string)` / `SetText(std::string)` | 设置 Markdown，解析一次并标记布局失效 |
| `SetImageResolver(ImageResolver)` | `source → RichTextImage{texture,size,valid}`，解析阶段调用一次 |
| `SetLinkCallback(LinkCallback)` | 链接点击回调；另有 `linkActivated` 信号 |
| `SetTextScale(float)` | 整体缩放（跟 DPI/UIScale 配合） |
| `SetLineGap(float)` / `SetParagraphGap(float)` | 行距 / 段间距 |
| `SetMaxImageSize(w,h)` | 图片最大尺寸（0 = 不限） |
| `SetImagePlaceholder(bool)` | 缺图时画占位还是什么都不画 |
| `SetAutoHeight(bool)` | 是否自撑高（默认开） |
| `SetScrollKeys(bool)` | 上下键滚「外部」滚动容器（默认关，方向键留给焦点导航） |
| `SetIndentWidth(float)` | 列表 / 引用的缩进步长 |
| `ContentHeight()` / `ContentWidth()` / `Links()` / `Document()` | 只读状态（供外部布局 / 自检） |

**焦点**：默认 `focusable = false`（规范要求）。要让手柄滚动外部容器，调用方打开
`focusable = true` 并 `SetScrollKeys(true)`（弹窗里的 Markdown 就是这么做的）。

---

## 5. 图片系统

RichText 不认识 PNG / JPEG / 文件系统 / SD 卡 / HTTP，只认识回调：

```cpp
text->SetImageResolver([&](const std::string& source) {
    RichTextImage image;
    auto found = cache.find(source);
    if (found == cache.end()) { found = cache.emplace(source, Load(source)).first; } // 宿主自己的加载
    if (!Valid(found->second)) return image;                // valid=false → 占位
    image.valid = true;
    image.texture = Texture(found->second);
    image.size = Size(found->second);
    return image;
});
```

* 解析阶段为每张图调用 **一次**，结果缓存在文档节点里，之后每帧只做绘制；
* `valid == false` 时按 `SetImagePlaceholder()` 画占位（圆角框 + 图标）；
* 布局阶段做等比缩放：不超过可用宽度 / `SetMaxImageSize`，绝不拉伸变形；
* 独占一段的图片居中；行内图片跟在文字后面参与换行；
* 图片圆角取 `Global::component_style.corner_radius`（与 Box / Button 同一套）。

---

## 6. Theme 集成

颜色与尺寸全部来自 `Theme::` 与 `Global::component_style`，组件内没有硬编码色值：

| 用途 | 取值 |
|---|---|
| 正文 | `Theme::kTextPrimary` |
| 标题 | `Theme::kTextBright` + `Theme::kFontTitle/kFontHeader/kFontBody` |
| 次要（列表符号、引用、占位图标） | `Theme::kTextMuted` / `Theme::kBorderStrong` |
| 链接 | `Theme::kBlue`（hover → `Theme::kAccentHover`） |
| 分隔线 / 图片描边 | `Theme::kBorder` |
| 代码块 / 行内代码底色 | `Theme::kBgInput` |
| 圆角 | `Global::component_style.corner_radius` |
| 行距 / 段距 / 缩进 | `Theme::kGapSmall` / `Theme::kGap` |

切主题时 `OnThemeChanged()` 让布局失效，下一帧用新颜色重排（颜色是排版结果的一部分）。

---

## 7. 性能

```text
SetMarkdown()  ──► document_dirty_ ──► Parse（一次）──► RichTextDocument
                                              │
宽度 / 字号 / 主题变化 ──► layout_dirty_ ──► Layout（按需）──► RichTextLayout
                                              │
                        每帧 ─────────────► Render（只遍历 glyph 列表）
```

* **Parse Cache**：只在 `SetMarkdown` / `SetImageResolver` 时重跑；解析结果（含图片解析）持有在文档里；
* **Layout Cache**：`layout_dirty_` 或 `width`/`scale` 变化才重排；否则直接复用行与 glyph；
* **Image Cache**：图片解析在解析阶段完成，渲染阶段只读 `ImTextureRef`，不加载资源；
* 渲染阶段不分配字符串（glyph 只引用文档里的 span，或持有图片必需的小副本）；
* 目标平台（Switch 720p、60fps）下每帧成本 ≈ 行数 × glyph 数 的绘制调用，无解析、无布局。

---

## 8. 限制（明确不支持）

* ❌ HTML / CSS / DOM / JavaScript / WebView
* ❌ Table（Markdown 表格）、Mermaid、LaTeX / 数学公式
* ❌ 富文本编辑（光标 / 选区 / 剪贴板 / 撤销 / 拼写检查）
* ❌ 自动链接（`<url>`）、任务列表、脚注、定义列表
* ❌ 三级以上嵌套列表（两级，文档即约定）
* ❌ 真正的等宽字体与斜体/粗体字重：项目当前只有一个字体，
  粗体用「半像素重绘」、斜体用「顶点绕基线剪切」模拟（不修改字体系统）
* ❌ 滚动容器：RichText 只算高度 + 绘制。超出屏幕由外部页面/弹窗的滚动系统负责
* ❌ 平台相关代码：组件不认识文件系统 / SD 卡 / 网络，全部通过 resolver 注入

---

## 9. 使用示例

### 9.1 最简

```cpp
RichText rich;
rich.SetMarkdown(R"(
# GBAStation

这是 **RichText** 示例。

[color=#55CC88]运行成功[/color]

- FC
- GBA
- NDS
)");
rich.size.x = 640.0f;           // 宽度由布局给
rich.SetAutoHeight(true);       // 内容多高就多高
```

### 9.2 放进弹窗（真实用法，见 `Popup::setMarkdown`）

```cpp
// 弹窗内容 = 一个 Overflow::Scroll 的 Box + RichText
Box* view = host.Emplace<Box>("markdown_scroll");
view->overflow = Overflow::Scroll;
view->size.y = 300.0f;                                  // 视口高度
RichText* text = view->Emplace<RichText>();
text->SetMarkdown(markdown);
text->SetImageResolver(resolver);
text->focusable = true;
text->SetScrollKeys(true);                              // 上下键滚这段正文
text->SetMaxImageSize(0.0f, 220.0f);
```

### 9.3 页面里并排多段（方向键优先）

```cpp
// 页面里 RichText 之间要能上下移动：不要打开 SetScrollKeys，
// 方向键交给焦点导航，页面整体滚动由 Widget::EnsureVisible 负责。
```

### 9.4 支持的语法速查

```markdown
# 一级标题
## 二级标题
**粗体**  *斜体*  ***粗斜体***  ~~删除线~~  `行内代码`
[color=#FF5555]错误[/color]  [color=#55FF55]成功[/color]  [color=#5599FFAA]半透明[/color]
- 无序
- 列表
  - 二级
1. 有序
2. 列表
> 引用
---
[链接](https://github.com)
![图片](img/cover.png)   行内图片也可以：这是 ![icon](img/icon.png) 一行
```

---

## 10. 文件

| 文件 | 说明 |
|---|---|
| `component_view/components/RichText.h` | 单文件实现（Parser / Document / Layout / Renderer + API） |
| `RichText.md` | 本文档 |
