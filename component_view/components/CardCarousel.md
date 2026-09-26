# CardCarousel：游戏卡牌行

横排卡片 + 选中卡居中 + 焦点缩放 / 流光框 + 触摸拖动。**只负责显示与导航**，
点「启动」只发信号，谁来启动由宿主决定。

## 1. 学习来源（GBAStation `SwitchLayout`）

本组件是把 GBAStation `src/ui/view/SwitchLayout.cpp` 的「游戏卡片行」搬到本组件库，
保留它的行为，换成库里的画法与交互约定：

| GBAStation 的做法 | 出处 | 本组件 |
|---|---|---|
| 一行固定 10 个槽位，卡片 220×350，间距 20 | `HOME_CARD_SLOTS` cpp:19-25、cpp:931-934 | `SetEmptySlots(n)` 占位卡，尺寸由 `Style` 给 |
| 卡片 = 封面 + 标题（上方）+ 平台徽章 + 时长 / 上次游玩 | `_drawGameCard` cpp:1082-1207 | 封面 `Image(Cover)` + 标题 / 副行在封面下方 + 徽标 / 红心叠在封面上 |
| 封面按 `max(w/iw,h/ih)` 居中裁剪，解码中画 shimmer | `_drawCover` cpp:1209-1246、cpp:938-967 | `Image::Fit::Cover` + `Image` 自带的占位框；入场用错开淡入 |
| 选中卡往行中心滚（插值，不瞬移） | `_updateTargetScroll` cpp:896-910、cpp:623-635 | `scroll_` 指数平滑 → `scroll_target_` |
| 选中卡放大 + 一圈渐变流光框，标题才跑马灯 | cpp:1082-1205、cpp:1130-1149 | `Style::focus_scale` + `Draw::FlowingRing` + `Draw::MarqueeText` |
| ←/→ 切卡 + 长按连发（0.30s 后每 0.085s 一张），上下不连发 | cpp:26-27、cpp:811-828 | `OnPadAction(Left/Right)` 首帧 + `OnUpdate` 里按 dt 推进连发 |
| 空位画成占位卡 | `_drawEmptyCard` cpp:998-1044 | `Card::empty` + 「空位」占位卡（不可选中） |
| 删除抖动 / 回流动画 | cpp:559-610、cpp:1046-1080 | **未移植**：这是与数据层联动的删除动画，宿主可自己加 |

换成库里的约定后，视觉一致性由全局样式负责：封面圆角、焦点框粗细 / 颜色 / 是否 `pause_focus_frame`
全部读 `Global::component_style`，平台徽标的文字与底色查 `PlatformBadgeInfoOf()`（与 Badge 组件同一张表）。

## 2. 快速开始

```cpp
CardCarousel* row = panel.Emplace<CardCarousel>();
row->SetCards({
    {EmuPlatform::GBA, "冒险者物语", "12.5 小时 · 昨天", "img/photo.jpg"},
    {EmuPlatform::GBC, "星海远征",   "48 分钟 · 3 天前", "img/image.png",  /*favourite=*/true},
});
row->SetEmptySlots(2); // 一行末尾留两个空位

connect(row, &CardCarousel::cardActivated, this, [](int index) { LaunchByIndex(index); });
connect(row, &CardCarousel::selectionChanged, this, [](int index) { /* 更新详情栏 */ });
```

封面走宿主注入的图片钩子（与 ImageViewer 同一套）：宿主注册 `Global::image_source.load/release`，
组件按路径取一次纹理并在析构时还回去；**没注册时不会报错**，封面退化成 `Image` 的占位框。

## 3. 卡片数据

| 字段 | 说明 |
|---|---|
| `EmuPlatform platform` | 徽标来源：文字 + 底色查 `PlatformBadgeInfoOf()`；`Unknown` 不画徽标 |
| `std::string title` | 主标题；选中且本行有焦点时放不下会跑马灯，否则省略号截断 |
| `std::string meta` | 副行（游玩时长 / 上次游玩之类，组件不解析内容） |
| `std::string cover_path` | 封面路径（`Global::image_source`） |
| `bool favourite` | 右上角红心 |
| `bool empty` | 占位卡：只占位、不参与选中与导航 |

## 4. 对外接口

| 方法 | 说明 |
|---|---|
| `SetCards(std::vector<Card>, int start_index = 0)` | 换整行数据（占位卡自动排到最后） |
| `AddCard(Card)` / `SetEmptySlots(int)` | 追加一张 / 设占位卡数量 |
| `SetStyle(const Style&)` | 尺寸与动效参数 |
| `SetIndex(int, bool notify = true)` | 选中第 N 张（会滚到中心） |
| `index()` / `count()` / `dataCount()` / `cardAt(i)` / `current()` | 读取状态 |
| `currentCardRect()` / `IsCardVisible(i)` | 供宿主做详情栏对齐 / 判断 |
| `ActivateCurrent()` | 等价于「按 A」 |
| `PlayEntrance()` | 重播入场 |
| `signal selectionChanged(int)` | 选中项变化 |
| `signal cardActivated(int)` | A / 再点一次已选中的卡 |
| `signal favouriteToggled(int)` | X 切换收藏（宿主自己持久化） |

## 5. Style（默认按 1080×600 逻辑画布）

| 字段 | 默认 | 说明 |
|---|---|---|
| `card_width` / `cover_height` | 148 / 106 | 封面尺寸（GBAStation 是正方形，这里用 4:3 更省纵向空间） |
| `title_height` / `meta_height` | 22 / 20 | 封面下方两行的高度 |
| `card_gap` | 16 | 卡间距（= 滚动步长 pitch 的一部分） |
| `cover_radius` | 8 | 封面圆角 |
| `focus_scale` | 1.055 | 选中卡放大比例 |
| `title_size` / `meta_size` | `kFontBody` / `kFontSmall` | 字号 |
| `enter_speed` / `enter_stagger` | 3.2 / 0.05 | 入场速度与逐卡错开 |
| `scroll_speed` | 11 | 滚动插值速度 |
| `hold_delay` / `hold_repeat` | 0.30 / 0.085 | 长按连发（与 GBAStation 同值） |

## 6. 交互

**手柄**（本行持有焦点时）

| 输入 | 行为 |
|---|---|
| ← / → | 上一张 / 下一张，首尾相接；长按 0.30s 后每 0.085s 一张 |
| L / R（PageLeft / PageRight） | 上一屏 / 下一屏（一屏 = 当前可见张数） |
| X | 收藏 / 取消收藏当前卡（发 `favouriteToggled`） |
| A | 启动当前卡（发 `cardActivated`） |
| ↑ / ↓ | 不消费，交给页面在卡牌行与功能行之间移动焦点 |

`capture_horizontal = true` + `focus_only_self = true`：整行是一个焦点停靠点，左右键被本行吃掉，
所以全局焦点导航不会把焦点挪到别的控件上。

**触摸 / 鼠标**

| 手势 | 行为 |
|---|---|
| 点未选中的卡 | 选中它（并把它滚到中心） |
| 点已选中的卡 | 启动（= A） |
| 横向拖动 | 滚卡；松手吸附到最近一张（拖动期间不会触发点击，也不会带动页面纵向滚） |
| 滚轮 | 横向滚卡 |

## 7. 焦点与绘制

* 焦点指示画在**选中那张卡的封面**上：`Draw::FlowingRing`，粗细 / 颜色 / 饱和度 / 是否
  `pause_focus_frame` 全部来自 `Global::component_style`；本行有焦点时全亮，没有焦点时降到 45% 透明度。
* 卡片视觉（徽标 / 红心 / 焦点框 / 标题）画在 `OnDrawOverlay` —— 必须盖在封面 `Image` 子节点之上。
* 超出一行的卡片用 `Overflow::Hidden` 裁掉；完全在可视区外的卡直接不画。
* 颜色全部读 `Theme`（浅色 / 深色都跟随），没有任何硬编码色值。

## 8. 限制 / 未做

1. **数据层不管**：启动、删除、收藏持久化都由宿主接信号实现；组件不碰文件系统 / 数据库。
2. **删除动画未移植**：GBAStation 的抖动 + 回流 + 占位卡补位是与数据层联动的，需要时由宿主自己做。
3. **不做多行**：一行到底；要多行请放多个 CardCarousel 或等后续的网格组件。
4. **不做分页点**：没有「第 N/M 页」指示，靠选中卡居中表达位置。
5. 封面纹理由宿主缓存保活；组件析构会调 `Global::image_source.release`，宿主自己决定是否真的释放。
