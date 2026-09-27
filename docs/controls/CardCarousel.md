# CardCarousel

来源：`component_view/components/CardCarousel.h`（实现 `component_view/components/CardCarousel.cpp`）
一句话：游戏卡牌行 —— 横排卡片（封面 + 标题 + 平台徽标 + 副行）、选中卡居中放大并带流光焦点框，只负责显示与导航，点「启动」只发信号。

## 最小示例

```cpp
#include "component_view/components/CardCarousel.h"

CardCarousel* row = panel.Emplace<CardCarousel>();
row->SetCards({
    {EmuPlatform::GBA, "冒险者物语", "12.5 小时 · 昨天", "img/a.png"},
    {EmuPlatform::GBC, "星海远征", "48 分钟 · 3 天前", "img/b.png", /*favourite=*/true},
});
row->SetEmptySlots(2); // 一行末尾留两个占位卡
connect(row, &CardCarousel::cardActivated, this, [](int i) { LaunchByIndex(i); });
```

## API

### 数据结构 Card

| 字段 | 类型 / 默认 | 说明 |
|---|---|---|
| `platform` | `EmuPlatform = EmuPlatform::Unknown` | 徽标来源，文字与底色查 `PlatformBadgeInfoOf()`；`Unknown` 不画徽标 |
| `title` | `std::string` | 主标题；选中且本行有焦点时跑马灯，否则省略号截断 |
| `meta` | `std::string` | 副行文字（组件不解析内容） |
| `cover_path` | `std::string` | 封面路径，走 `Global::image_source` |
| `favourite` | `bool = false` | 右上角红心 |
| `empty` | `bool = false` | 占位卡：只占位，不参与选中与导航 |

### 公开方法

| 方法 | 说明 |
|---|---|
| `CardCarousel()` / `~CardCarousel() override` | 析构时把取过的封面纹理引用还回 `Global::image_source.release` |
| `CardCarousel& SetCards(std::vector<Card> cards, int start_index = 0)` | 换整行数据；重算数据卡数，把 `empty` 卡 `stable_partition` 到尾部，重置滚动与长按，重建封面子节点并 `PlayEntrance()` |
| `CardCarousel& AddCard(Card card)` | 追加一张；第一张数据卡会把 `index_` 设为 0 |
| `CardCarousel& SetEmptySlots(int count)` | 先删掉现有占位卡，再在末尾补 `count` 张（负数按 0） |
| `CardCarousel& SetStyle(const Style& value)` | 换尺寸与动效参数，并同步更新已有封面的圆角 |
| `CardCarousel& PlayEntrance()` | 重播入场（把 `enter_` / `focus_` 归零） |
| `CardCarousel& SetIndex(int index, bool notify = true)` | 选中第 index 张（夹到 `[0, dataCount()-1]`）；值没变直接返回；会 `SnapScrollToIndex()` 滚到中心 |
| `int index()` / `int count()` / `int dataCount()` | 当前下标 / 卡片总数（含占位）/ 数据卡数（不含占位） |
| `const Card* cardAt(int index)` / `const Card* current()` | 取卡片数据（越界返回 `nullptr`） |
| `Rect currentCardRect()` | 当前卡片的绝对屏幕矩形 |
| `CardCarousel& ActivateCurrent()` | 等价于按 A：当前卡非占位时发 `cardActivated` |
| `bool IsCardVisible(int index)` | 该卡是否落在可视区内 |

### Style

`style` 成员是**私有**的，只能通过 `SetStyle()` 整体替换。全部字段如下：

| 字段 | 默认值 | 说明 |
|---|---|---|
| `card_width` | `148.0f` | 卡宽（封面宽） |
| `cover_height` | `106.0f` | 封面高 |
| `title_height` | `22.0f` | 标题行高 |
| `meta_height` | `20.0f` | 副行行高 |
| `card_gap` | `16.0f` | 卡间距 |
| `cover_radius` | `8.0f` | 封面圆角 |
| `focus_scale` | `1.055f` | 选中卡放大比例 |
| `title_size` | `Theme::kFontBody`（16） | 标题字号 |
| `meta_size` | `Theme::kFontSmall`（14） | 副行字号 |
| `enter_speed` | `3.2f` | 入场速度（1/s） |
| `enter_stagger` | `0.05f` | 每张卡错开多少秒 |
| `scroll_speed` | `11.0f` | 滚动插值速度（1/s） |
| `hold_delay` | `0.30f` | 长按连发延迟 |
| `hold_repeat` | `0.085f` | 连发间隔 |
| `fling_threshold` | `320.0f` | 松手速度低于它就直接吸附（px/s） |
| `fling_max` | `4200.0f` | 甩动速度上限（px/s） |
| `friction` | `5.0f` | 甩动衰减（1/s） |

### 子类扩展点

| 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 宽高未显式给时取可用空间；内部调用 `LayoutCards()` 当帧算好卡片矩形 |
| `void OnDrawOverlay(ImDrawList* dl, const Rect& content) override` | 画徽标 / 红心 / 标题 / 副行 / 焦点框（必须盖在封面 `Image` 子节点之上） |
| `void OnUpdate(float dt) override` | 入场、滚动插值、长按连发、触摸拖动、甩动惯性、滚轮 |
| `void Activate() override` | 指针路径：点哪张选哪张，点已选中那张才启动 |
| `bool OnPadAction(InputAction action) override` | `Left` / `Right` / `PageLeft` / `PageRight` / `ActionX` / `Confirm` |
| `void OnThemeChanged() override` | 重新把底色置 0 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `selectionChanged` | `int` | 选中项变化（含手柄切卡、拖动改选中、`SetIndex` 通知） |
| `cardActivated` | `int` | A / 点击已选中卡：启动这张卡 |
| `favouriteToggled` | `int` | X（`ActionX`）把当前卡的 `favourite` 翻转；**持久化由宿主自己做** |

## 交互与键盘 / 手柄

本行是一个复合焦点停靠点：`focusable = true`、`focus_only_self = true`、`capture_horizontal = true`、`focus_frame = false`、`overflow = Overflow::Hidden`。

| 输入 | 行为 |
|---|---|
| ← / → | 上一张 / 下一张，首尾相接（`Step(direction, wrap = true)`）；长按连发 |
| L / R（`PageLeft` / `PageRight`） | 上一屏 / 下一屏，一屏 = `floor(可视宽 / pitch)` 张（至少 1） |
| X（`ActionX`） | 当前卡非占位时翻转 `favourite` 并发 `favouriteToggled` |
| A（`Confirm`） | `ActivateCurrent()` |
| ↑ / ↓ / B | 不在 `OnPadAction` 里处理，交给页面的焦点导航 |

长按连发只在 `focused` 时生效，且只管左右（上下不连发）：先等 `hold_delay`，之后每 `hold_repeat` 走一张；方向归零就清计时。

| 手势 | 行为 |
|---|---|
| 点未选中的卡 | 选中它并滚到行中心 |
| 点已选中的卡 | 启动（= A） |
| 横向拖动（阈值 8px） | 滚卡，选中项跟着手指走：选中 = 按下时下标 + 位移换算出的卡数 |
| 松手 | 速度够大进入甩动惯性（`exp(-friction*dt)` 衰减），快停或到边界时吸附；速度低于 `fling_threshold` 直接吸附 |
| 滚轮 | 横向滚卡（`mouse_wheel * 60`） |

## 尺寸 / 主题约定

| 项 | 值 / 来源 |
|---|---|
| 行高 | `cover_height + title_height + meta_height + 10.0f` |
| 卡片步长 pitch | `card_width + card_gap` |
| 平台徽标高 / 左右内边距 | 固定常量 20 / 7（`CardCarousel.cpp:17-18`） |
| 收藏红心字号 | 固定常量 16（`CardCarousel.cpp:16`） |
| 标题 / 副行 | `Theme::kTextPrimary` / `Theme::kTextMuted`（选中标题用 1.0、未选中 0.78 透明度） |
| 徽标文字 | `Theme::kWhite`，底色查 `PlatformBadgeInfoOf(platform).background` |
| 红心 | `Theme::kError` |
| 占位卡 | 底 `Theme::kBgWidget`、描边 `Theme::kBorderStrong`、图标 / 「空位」用 `Theme::kTextMuted` |
| 焦点框 | 粗细 / 颜色 / 饱和度 / 是否 `pause_focus_frame` 全取 `Global::component_style`；本行无焦点时透明度降到 45% |

## 注意点

1. 封面需要宿主注册 `Global::image_source.load/release`；未注册不会报错，封面退化成 `Image` 自己的占位框（`CardCarousel.cpp:183-185`）。
2. 卡片视觉（徽标 / 红心 / 标题 / 焦点框）必须画在 `OnDrawOverlay`，放 `OnDrawContent` 会被封面 `Image` 子节点盖住。
3. 拖动时置 `Global::pointer_dragging = true`，基类据此取消这次点击，避免「一拖既滚动又启动」。
4. 拖动期间选中**不按「谁在行中心」算**（行中心那张往往不是当前选中项，一按就会跳），而是「按下时下标 + 位移 / pitch 四舍五入」（`CardCarousel.cpp:465-474`）。
5. 完全落在可视区外的卡直接不画（横向 ±4px 判据），但 `rects_local_` 仍然全部保留。
6. 占位卡（`empty = true`）不参与选中与 `dataCount()`；`SetIndex` 的上界是 `dataCount()-1`，不是 `count()-1`。
7. 收藏只是改内存里的 `Card::favourite`，落盘由宿主接 `favouriteToggled` 自己实现。

设计说明见 `component_view/components/CardCarousel.md`。
