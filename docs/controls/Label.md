# Label

来源：`component_view/components/Content.h`（实现 `component_view/components/Content.cpp`）
一句话：静态文本内容件，支持单行 / 按宽度自动换行 / `\n` 显式断行 / 图标 + 文字 / 居中 / 省略号 / 跑马灯。

## 最小示例

```cpp
Label* title = page.Root().Emplace<Label>("基础控件预览");
title->setFontSize(Theme::kFontTitle);

Label* body = page.Root().Emplace<Label>(
    "这是一段会自动换行的正文：把宽度交给布局后，Label 会按可用宽度断行。");
body->setWrap(true, 6.0f);      // 第二参 = 行距
body->font_size = Theme::kFontSmall;
body->setColor(Theme::kTextMuted);

Label* with_icon = page.Root().Emplace<Label>("带图标的标签");
with_icon->setIcon(Icons::Glyph(Icons::Material::FavoriteBorder));
with_icon->setAlign(TextAlign::Center);
```

## API

### 构造与公开字段

| 成员 / 方法 | 说明 |
|---|---|
| `Label()` | 默认名 `"label"`，`text_color = Theme::kTextPrimary` |
| `explicit Label(std::string value)` | 先按默认构造再把 `text` 设为传入值 |
| `std::string text` | 文本内容 |
| `std::string icon` | 可选的 Material 字形，画在文字左边 |
| `float icon_gap` | 图标与文字间距，默认 8.0 |
| `float font_size` | 0 = `Theme::kFontBody` |
| `ImVec4 text_color` | 文字色，默认 `Theme::kTextPrimary` |
| `bool text_color_follows_theme` | 文字色是否跟随主题，默认 true；`setColor` 会置 false |
| `TextAlign text_align` | 默认 `TextAlign::Left`，可 `Center` / `Right` |
| `VerticalAlign vertical_align` | 默认 `VerticalAlign::Middle`，可 `Top` / `Bottom` |
| `bool wrap` | true = 按内容区可用宽度换行，默认 false |
| `float line_gap` | 多行行距，默认 4.0；`setWrap` 会一起改 |
| `float max_lines` | >0 = 最多画这么多行，超出省略 |
| `bool ellipsize` | 单行放不下时用 `…`，默认 true |
| `bool marquee` | 单行放不下时横向滚动，默认 false，且优先于 `ellipsize` |
| `float marquee_speed` | 跑马灯速度（px/s），默认 26.0 |

### 链式设置（全部返回 `Label&`）

| 成员 / 方法 | 说明 |
|---|---|
| `setText(std::string value)` | 设 `text` |
| `setIcon(std::string glyph)` | 设 `icon` |
| `setFontSize(float value)` | 设 `font_size` |
| `setColor(ImVec4 color)` | 设 `text_color` 并关闭主题跟随 |
| `setAlign(TextAlign value)` | 设水平对齐 |
| `setVerticalAlign(VerticalAlign value)` | 设垂直对齐 |
| `setWrap(bool value, float gap = 4.0f)` | 设 `wrap` 并把 `line_gap` 设为 `gap` |

### 测量与扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `ImVec2 TextSize(float wrap_width) const` | 按当前换行设置算出的文字块尺寸（不含 icon）；`wrap = false` 或 `wrap_width <= 0` 时按单行量 |
| `float LineHeight() const` | 单行行高（按当前字号量 `"Ag"`） |
| `ImVec2 MeasureContent(const ImVec2& available) override`（protected） | 文字尺寸 + icon 宽度 + `icon_gap`；`max_lines > 0` 时高度夹到 `max_lines * LineHeight() + (max_lines - 1) * line_gap` |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override`（protected） | 按 `text_align` / `vertical_align` 定位后绘制图标与各行文字 |
| `void OnThemeChanged() override`（protected） | `text_color_follows_theme` 为 true 时重取 `Theme::kTextPrimary` |

Label 不声明自己的信号，事件全部继承自 [Widget](Widget.md)。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `clicked` 等继承信号 | 无 | Label 默认 `focusable = false`、`interactive = true`；需要点击时自己 `SetFocusable(true)` 并连接基类信号，见 [Widget.md](Widget.md) |

## 尺寸 / 主题约定

- `font_size = 0` 时取 `Theme::kFontBody`；行距默认 4.0；图标与文字间距按像素给的 `icon_gap`，不读主题常量。
- 文字色默认 `Theme::kTextPrimary` 且跟随主题；`setColor` 会关掉跟随。
- 绘制时颜色按 `EffectiveOpacity()` 乘一次（`enabled = false` 会叠加 `disabled_opacity`）。
- `wrap = true` 时换行宽度取内容区宽度；外框宽度已设定时按 `size.x` 减去 padding 与边框算。

## 注意点

- 跑马灯优先于省略号：`marquee = true` 且单行放不下时走 `Draw::MarqueeText`，`ellipsize` 不再生效。
- `wrap = true` 时绘制会 `PushClipRect` 到内容区，超出的行被裁掉；这一分支不处理 `ellipsize` / `marquee`。
- `text` 与 `icon` 都为空时 `OnDrawContent` 直接返回，不绘制。
- `max_lines` 只限制绘制行数与测量高度，不会改变换行结果。
