# Header

来源：`component_view/components/Header.h`（实现 `component_view/components/Header.cpp`）
一句话：区块标题行 —— 左侧圆角竖条 + 标题文字 + 可选右侧补充文字 + 底部分隔线，用来把页面按「Header + 一组控件」水平切段。

## 最小示例

```cpp
#include "component_view/components/Header.h"

Header* header = panel.Emplace<Header>("常规设置");
header->setInfo("共 15 项");       // 右侧补充文字，可空
header->size.x = content_width;    // 给成内容区宽度，分隔线就横跨整段
```

## API

### 公开字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `title` | `std::string` | 左侧标题 |
| `info` | `std::string` | 右侧补充文字（例如「共 15 项」），可空 |
| `style` | `Style` | 几何与字号，见下 |

### 构造与链式 setter

| 成员 / 方法 | 说明 |
|---|---|
| `Header()` | 默认构造 |
| `explicit Header(std::string title)` | 带标题构造 |
| `Header& setTitle(std::string value)` | 设置标题 |
| `Header& setInfo(std::string value)` | 设置右侧补充文字 |
| `Header& setBarColor(const ImVec4& color)` | 覆盖竖条颜色 |
| `Header& setTextColor(const ImVec4& color)` | 覆盖标题文字颜色 |
| `Header& setDividerColor(const ImVec4& color)` | 覆盖分隔线颜色 |

### Style

| 字段 | 默认值 | 说明 |
|---|---|---|
| `height` | `58.0f` | 整条高度 |
| `bar_width` | `4.0f` | 左侧竖条宽 |
| `bar_height` | `24.0f` | 左侧竖条高 |
| `bar_radius` | `2.0f` | 竖条圆角 |
| `bar_offset_x` | `2.0f` | 竖条离左边缘 |
| `text_offset_x` | `18.0f` | 标题离左边缘 |
| `text_size` | `Theme::kFontHeader`（20） | 标题字号 |
| `info_size` | `Theme::kFontSmall`（14） | 右侧补充文字字号 |
| `info_gap` | `12.0f` | 标题与右侧补充文字的最小间距 |
| `divider_inset_x` | `18.0f` | 分隔线左缩进 |
| `divider_inset_right` | `6.0f` | 分隔线右缩进 |
| `divider_offset_y` | `6.0f` | 分隔线离底部距离 |
| `divider_width` | `1.0f` | 分隔线粗细 |
| `divider` | `true` | 是否画底部分隔线 |

### 子类扩展点

| 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 高度固定 `style.height`；宽度按标题 + 补充文字测量并夹到 `available.x` |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 画竖条 / 标题 / 补充文字 / 分隔线 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| 无 | - | 构造函数里 `focusable = false`，不接手柄键，也不发信号 |

## 交互与键盘 / 手柄

| 项 | 行为 |
|---|---|
| 焦点 | 不是焦点停靠点（`focusable = false`） |
| 按键 | 不消费任何手柄键，全部留给页面 |
| 视觉底 | `background = 0`、`border.width = 0`、`shadow.enabled = false`，只有竖条 / 文字 / 分隔线 |

## 尺寸 / 主题约定

| 元素 | 默认主题角色 | 说明 |
|---|---|---|
| 竖条 | `Theme::kAccent` | 可用 `setBarColor` 覆盖 |
| 标题 | `Theme::kTextPrimary` | 可用 `setTextColor` 覆盖 |
| 右侧补充文字 | `Theme::kTextMuted` | 无覆盖接口，恒跟主题 |
| 分隔线 | `Theme::kBorder` | 可用 `setDividerColor` 覆盖 |

颜色覆盖的约定：`alpha = 0` 表示跟主题，`alpha > 0` 才用覆盖值（`Pick()`，`Header.cpp:10-12`）。

## 注意点

1. 三个颜色覆盖接口默认值都是 `{0,0,0,0}`，即「跟主题」；切成固定色再想回到主题要把 alpha 设回 0。
2. 右侧补充文字只在空间够时绘制：`info_x > text_x + style.info_gap` 才画，容器太窄时它会直接消失（`Header.cpp:91`）。
3. `MeasureContent` 的高度恒为 `style.height`，改高度只能通过 `style.height`。
4. 分隔线的左右端点按 `divider_inset_x` / `divider_inset_right` 算，只有 `x1 > x0` 才画。
