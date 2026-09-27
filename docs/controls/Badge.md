# Badge：机种徽标

来源：`component_view/components/Badge.h`（实现 `component_view/components/Badge.cpp`）
一句话：运行时绘制的圆角矩形 + 居中短文本，用来标机种（GBA / NDS / PSP …），没有图片、没有九宫格。

## 最小示例

```cpp
Badge* badge = panel->Emplace<Badge>(EmuPlatform::GBA);   // 构造即查表注入文字 + 底色
badge->setStyle(BadgeStyle::GridItem)
     ->setHeight(Theme::kBadgeHeight)
     ->setFixedWidth(true)
     ->setMinWidth(48.0f);
```

## API

### 公开字段

| 字段 | 默认 | 说明 |
|---|---|---|
| `platform` | `EmuPlatform::Unknown` | 机种 id，仅 `setPlatform()` 会依据它注入内容 |
| `text` | 空 | 徽标文字；空文字不画徽标 |
| `background` / `foreground` | `rgba(100,100,100,200)` / `Theme::kTextPrimary` | 底色 / 文字色 |
| `font_size` / `height` | `Theme::kFontSmall`(14) / `Theme::kBadgeHeight`(26) | 字号 / 徽标高度 |
| `min_width` / `pad_x` | `36.0f` / `8.0f` | 最小宽度 / 左右内边距（`pad_x` 两侧都乘 2） |
| `radius` | `4.0f` | 圆角；`<0` = 胶囊（`height / 2`） |
| `fixed_width` | `false` | `true` = 宽度恒为 `min_width` |
| `long_text_min_width` | `0.0f` | `> 0` 且文本超过 3 字符时改用这个最小宽度 |
| `alpha` | `1.0f` | 动画进度，乘在底色 / 文字 alpha 上（IisuCover 用） |

### 链式接口

| 方法 | 说明 |
|---|---|
| `setPlatform(EmuPlatform)` | 查表注入 `text` + `background`（不改几何） |
| `setText` / `setColors(bg, fg)` | 显式文字 / 显式设色（之后不跟主题） |
| `setFontSize` / `setHeight` / `setMinWidth` / `setPadding` | 分别写 `font_size`（`>= 1`）/ `height`（`>= 1`）/ `min_width`（`>= 0`）/ `pad_x`（`>= 0`） |
| `setRadius` / `setFixedWidth` / `setAlpha` | 圆角（`<0` 胶囊）/ 固定宽 / alpha（夹进 `[0,1]`） |
| `setStyle(BadgeStyle)` | 整套套用变体几何与用色（见下） |
| `moveTo(x, y)` | 位置 |
| `float badgeWidth() const` | 当前文字与样式下的实际宽度；文字为空返回 `0`（`MeasureContent` 返回 `(badgeWidth(), height)`，没设过 `size` 时控件尺寸就等于徽标本身） |

### 变体 BadgeStyle

| `BadgeStyle` | 几何（字号 / 高 / 最小宽 / pad / 圆角） | 底色 / 文字色 |
|---|---|---|
| `GridListDetail`（默认） | 14 / 26 / 36 / 8 / 4 | 平台色 / `kTextPrimary`（跟主题） |
| `IisuCover` | 12 / 17 / 30 / 14 / 胶囊 | 平台色 / 白 `alpha 255` |
| `GameDataView` | 14 / 26 / 固定宽 62 / 8 / 5 | 固定蓝 `rgba(79,153,222,205)` / 白 `alpha 245` |
| `GridItem` | 12 / 20 / 36（文本 > 3 字符时 58）/ 8 / 4 | 平台色 / 白 |

`BadgeStyleOf(style)` 直接返回 `BadgeStyleParams`（`font_size` / `height` / `min_width` / `pad_x` / `radius` / `fixed_width` / `long_text_min_width` / `white_text` / `fixed_blue_bg`），宿主可以不建控件直接读参数；`GridListDetail` 取的是结构体默认值（`kFontSmall` 14 / `kBadgeHeight` 26），头文件注释里的「12 / 20」与代码不一致。

### 机种表 `PlatformBadgeInfoOf(EmuPlatform)`

| 机种 | `text` | `background` | 机种 | `text` | `background` |
|---|---|---|---|---|---|
| `GBA` | `GBA` | `rgba(108,77,191,220)` | `MD` | `MD` | `rgba(23,55,139,220)` |
| `GBC` | `GBC` | `rgba(0,112,221,220)` | `Arcade` | `Arcade` | `rgba(236,134,44,220)` |
| `GB` | `GB` | `rgba(0,168,107,220)` | `DC` | `DC` | `rgba(0,142,180,220)` |
| `FC` | `FC` | `rgba(218,41,28,220)` | `PSP` | `PSP` | `rgba(67,118,226,220)` |
| `SFC` | `SFC` | `rgba(160,100,180,220)` | `PS1` | `PS1` | `rgba(74,74,82,220)` |
| `NDS` | `NDS` | `rgba(54,150,190,220)` | `Saturn` | `Saturn` | `rgba(68,82,150,220)` |
| `_3DS` | `3DS` | `rgba(230,79,91,220)` | `Dolphin` | `GC / Wii` | `rgba(54,102,196,220)` |

`EmuPlatform::Unknown`（0）与表外值返回 `{"", rgba(100,100,100,200)}`。

## 信号

Badge 未新增信号；构造时 `focusable = false`，纯展示，不参与手柄焦点导航。

## 尺寸 / 主题约定

| 项 | 值 |
|---|---|
| 默认字号 / 高度 | `Theme::kFontSmall`(14) / `Theme::kBadgeHeight`(26) |
| 宽度公式 | `fixed_width` → `min_width`；否则 `max(min_width, textW + 2 * pad_x)`，再按 `long_text_min_width` 取大 |
| 圆角 | `radius >= 0` 用之，`< 0` → `height / 2` |
| 内边距 | `padding = EdgeInsets{}`，全部走 `pad_x` |
| 最终不透明度 | 底色 / 文字 alpha × `alpha` × `EffectiveOpacity()` |
| 文字跟随主题 | `OnThemeChanged()` 在 `foreground_follows_theme_` 为 `true` 时刷新为 `kTextPrimary` |

## 注意点

- 空 `text` 不绘制：`PlatformBadgeInfoOf(EmuPlatform::Unknown)` 的文字就是空的，demo 里靠 `setText("其它")` 才显示出来。
- `setPlatform()` 只注入文字与底色，不改几何；`setStyle()` 又会用当前 `platform` 的底色覆盖 `background`（`GameDataView` 用固定蓝），所以顺序应是先 `setPlatform()` 再 `setStyle()`。
- `setColors()` 会把文字色固定下来（不再跟主题）；`setStyle()` 会重设 `foreground` 并按变体改写该跟随开关。
- `setStyle()` 用 `long_text_min_width` 判长文本，条件是 `text.size() > 3`（按字节数）。
- `badgeWidth()` 是纯计算，随 `text` / `font_size` / `pad_x` / `fixed_width` 实时变化；注释约定调用方用它 + 10 摆右侧元素。
