# RichText

来源：`component_view/components/RichText.h`（实现 `component_view/components/RichText.h`，全部 inline，没有 .cpp）
一句话：单文件、无第三方依赖的 Markdown 子集富文本控件，只做高性能显示（弹窗正文、游戏说明、更新日志、关于页、错误说明）。

## 最小示例

```cpp
#include "component_view/components/RichText.h"

using namespace gui_dev::cv;

// RichText 自己不提供滚动容器：长文要挂在 Box(Overflow::Scroll) / 弹窗的滚动内容里
RichText* text = page.Root().Emplace<RichText>();
text->SetMarkdown("# 更新日志\n\n- 修复 **偶发** 卡顿\n- 新增封面 ![封面](cover.png)")
    ->SetTextScale(1.0f)
    ->SetLineGap(4.0f)
    ->SetParagraphGap(12.0f)
    ->SetIndentWidth(20.0f)
    ->SetMaxImageSize(0.0f, 220.0f)   // 宽 0 = 用内容宽度，高 0 = 不限
    ->SetAutoHeight(true)
    ->SetImagePlaceholder(true)
    // 宿主把 source 解析成纹理；RichText 不认识文件系统 / 解码器 / SD 卡 / HTTP
    ->SetImageResolver([](const std::string& source) {
        return RichTextImage{};       // 无效 = 画占位框
    });

connect(text, &RichText::linkActivated, &page, [](const std::string& url) {
    // RichText 不自己开浏览器，链接交给宿主
});

// 弹窗里读长文：让焦点停在本控件，上下键滚它所在的外部滚动容器
text->focusable = true;
text->SetScrollKeys(true);
```

## API

### 内容

| 成员 / 方法 | 说明 |
|---|---|
| `RichText()` / `explicit RichText(std::string markdown)` | 构造；默认 `focusable = false`、`capture_vertical = false` |
| `SetMarkdown(std::string)` | 设置正文；解析一次 + 标记布局失效 |
| `SetText(std::string)` | `SetMarkdown` 的同义包装 |
| `Markdown()` | 当前 Markdown 原文 |
| `Document()` | 解析后的 `RichTextDocument` |
| `Empty()` | 文档没有任何 block |
| `ContentHeight()` / `ContentWidth()` | 上次布局的内容尺寸（外部滚动容器 / 弹窗布局用） |
| `LastRenderHeight()` | 上次渲染的高度 |
| `Links()` | 已排版的链接命中区 `std::vector<LinkHit>`（矩形是内容区局部坐标） |

### 资源 / 交互（扩展点）

| 成员 / 方法 | 说明 |
|---|---|
| `using ImageResolver = std::function<RichTextImage(const std::string& source)>` | 图片来源；解析阶段调用一次，之后缓存 |
| `SetImageResolver(ImageResolver)` | 换 resolver 会同时让文档与布局失效（图片要重新解析） |
| `using LinkCallback = std::function<void(const std::string& url)>` | 链接点击回调 |
| `SetLinkCallback(LinkCallback)` | 与 `linkActivated` 等价，二选一；两个都设会都触发 |

### 排版参数

| 方法 | 默认值 |
|---|---|
| `SetTextScale(float)` | `1.0`，整体字号缩放 |
| `SetLineGap(float)` | `Theme::kGapSmall * 0.5` = 3 |
| `SetParagraphGap(float)` | `Theme::kGap` = 10 |
| `SetIndentWidth(float)` | `Theme::kGap * 2` = 20 |
| `SetMaxImageSize(max_width, max_height)` | 宽 `0`、高 `220` |
| `SetImagePlaceholder(bool)` | `true`；无有效纹理时画占位框 |
| `SetAutoHeight(bool)` | `true`；`MeasureContent` 直接把 `size.y` 设成布局高度 |
| `SetScrollKeys(bool)` | `false`；打开后同时把 `capture_vertical` 置 true |
| `ScrollKeys()` | 当前值 |

### 文档模型 / 子结构

| 结构 | 字段 |
|---|---|
| `RichTextImage` | `ImTextureRef texture`、`ImVec2 size`、`bool valid`；静态 `Invalid()` |
| `RichTextSpan` | `text`、`bold`、`italic`、`strike`、`code`、`has_color`、`color`、`is_link`、`link` |
| `RichTextInline` | `enum class Kind { Text, Image }`、`span`、`image_source`、`image_alt`、`image` |
| `RichTextBlockKind` | `Paragraph`、`Heading`、`Bullet`、`Ordered`、`Quote`、`Divider` |
| `RichTextBlock` | `kind`、`level`（标题 1..6 / 列表缩进 0..2）、`marker`（`•` 或 `1.`）、`items` |
| `RichTextDocument` | `blocks`、`Empty()`、`Clear()` |
| `RichText::LinkHit` | `Rect rect`、`std::string url` |

### Markdown 子集（只列代码里真实支持的）

| 语法 | 行为 |
|---|---|
| 普通行 | 一行一段（单换行也换行，避免中英混排被拼在一起） |
| `# ` ~ `###### ` | 标题；`#` 数量 = level 1..6，`#` 后必须跟空格 |
| `**x**` / `__x__` / `*x*` / `_x_` | 粗体 / 斜体 |
| `***x***` / `___x___` | 粗斜体 |
| `~~x~~` | 删除线 |
| `` `x` `` | 行内代码（加底色带）；整行都是代码时整行铺底色 |
| `- ` / `* ` / `+ ` | 无序列表；行首每 2 空格一级缩进，最多两级，marker 固定 `•` |
| `1. ` | 有序列表；level 固定 0，marker 保留原文数字（如 `1.`） |
| `> ` | 引用；左侧画竖线 |
| `---` / `***` / `___` | 分隔线（>= 3 个同字符） |
| `[文字](url)` | 链接 |
| `![alt](path)` | 图片；段落里只有这一项时独占一行并居中 |
| `[color=#RRGGBB]…[/color]` | 染色，支持 `#RRGGBBAA`，可嵌套 |

不做：``` 代码块（已按反馈移除）、HTML / CSS / DOM / Table / Mermaid / LaTeX / WebView / 编辑器。
解析与渲染严格分离：`SetMarkdown` 解析 → 布局（宽度 / 字号 / 主题变化时重排）→ 每帧只走已排好的 glyph。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `linkActivated` | `std::string url` | 指针在链接命中区内按下，再在该链接上松开；先调 `SetLinkCallback` 的回调，再 emit |

## 交互 / 输入

| 输入 | 条件 | 行为 |
|---|---|---|
| 鼠标 hover | `Global::mouse_available` 且命中链接矩形 | 链接文字变 `Theme::kAccentHover` |
| 鼠标左键按下 / 松开 | 同一链接内按下并松开 | 触发回调 + `linkActivated`；松开时清 pressed 状态 |
| `Up` / `Down` | `focusable` + `SetScrollKeys(true)` | 滚外部滚动容器 1 行（行高 = 正文 + 行距） |
| `PageLeft`(L) / `TriggerLeft`(ZL) | 同上 | 向上滚 4 行 |
| `PageRight`(R) / `TriggerRight`(ZR) | 同上 | 向下滚 4 行 |
| 其它按键 | — | 不消费，交回焦点导航 |

上下键滚动只作用于 `ScrollHost()`（RichText 自己不做滚动容器）；没有可滚的外部容器时返回 false，按键继续走导航。

## 尺寸 / 主题约定

| 项 | 取值 |
|---|---|
| 正文字号 | `Theme::kFontBody`(16) × `text_scale_` |
| H1 / H2 / H3~H6 | `Theme::kFontTitle`+4 = 26 / `Theme::kFontHeader`(20) / `Theme::kFontBody`(16) |
| 正文色 / 标题色 | `Theme::kTextPrimary` / `Theme::kTextBright` |
| 列表 marker | `Theme::kTextMuted` |
| 分隔线 / 引用竖线 | `Theme::kBorder` / `Theme::kBorderStrong` |
| 行内代码底色 | `Theme::kBgInput` × 0.85 |
| 图片占位框 | `Theme::kBgWidget` + `Theme::kBorder` + `Icons::Material::ImagePlaceholder` |
| 圆角 | `Global::component_style.corner_radius`；行内代码固定 3，代码行 `min(radius, 4)` |
| 主题切换 | `OnThemeChanged` 直接标布局失效（颜色进了排版结果） |

## 注意点

- 只有一套字重：粗体靠同一位置半像素偏移重画一遍模拟；没有斜体字重，斜体是对刚写入的顶点做「绕基线剪切」（`ImDrawList` 顶点缓冲是公开 API）。
- ``` 代码块已按反馈移除：项目没有等宽字体，显示效果不理想。
- 图片在**解析阶段**由 resolver 解析并缓存，绝不每帧加载；换 resolver 必须重新解析。
- 布局带缓存：宽度、`text_scale_`、主题没变就复用；`SetLineGap` / `SetParagraphGap` / `SetMaxImageSize` 会直接标脏。
- `SetAutoHeight` 默认 true，`MeasureContent` 会把 `size.y` 改成内容高度；要固定高度必须 `SetAutoHeight(false)`，否则显式高度会被覆盖。
- 默认不参与焦点（`focusable = false`）；要用手柄滚外部容器得同时 `focusable = true` + `SetScrollKeys(true)`，后者会把 `capture_vertical` 也置 true。焦点框只框「可见的文本区」：内容可能远高于视口，整块框住会被裁剪掉。
- `Links()` 里的矩形是内容区局部坐标，命中测试要再加 `content_rect.min`（控件内部已这么做）。
