# FocusRing（焦点框图层）

来源：`component_view/FocusRing.h`（实现 `component_view/FocusRing.cpp`）
一句话：页面级焦点框图层，每帧读取当前焦点控件的 `Widget::BuildFocusVisual()` 描述，在页面内容/弹窗之上、Toast 之下画唯一一个焦点框。

## 典型用法

控件不再自己画焦点框，只描述要什么；由 `Page` 的图层统一画：

```cpp
// 页面级：Page::Render() 里每帧一次（component_view/pages/Page.cpp）
focus_ring_.Draw(dl, Global::focused);

// 控件级：只描述焦点框（component_view/components/Button.cpp，节选）
FocusVisual Button::BuildFocusVisual() const {
    FocusVisual visual;
    if (!flowing_focus || focus_mix <= 0.01f) {
        return visual;
    }
    const float margin = ResolvedFocusMargin();
    const Global::ComponentStyle& style = Global::component_style;
    visual.enabled = true;
    visual.pause_style = Global::pause_focus_frame;
    visual.flowing = !visual.pause_style;
    visual.rect = DrawRect().Expanded(margin);
    visual.radius = corner_radius + margin;
    visual.width = ResolvedFocusWidth();
    visual.alpha = focus_mix * EffectiveOpacity();
    visual.phase = Global::time * style.focus_flow_speed + focus_phase_offset;
    return visual;
}
```

## API

| 成员 / 方法 | 说明 |
|---|---|
| `void Reset()` | 清空状态（`rect_` / `has_rect_` / `alpha_` / `pause_style_`），页面重进时调 |
| `void Draw(ImDrawList* dl, Widget* target)` | 每帧调用一次；`target` 一般是 `Global::focused`，传 `nullptr` 则框淡出 |

`Draw` 读取的 `FocusVisual` 字段（定义在 `component_view/Widget.h`）：

| 字段 | 说明 |
|---|---|
| `enabled` | 不需要画（没焦点 / 组件不做焦点框） |
| `flowing` | true = 流光闭合框，false = 单色圆角框 |
| `rect` | 已经是绘制坐标（外扩后的） |
| `radius` / `width` | 圆角 / 线宽 |
| `alpha` | 0..1（焦点强度 × 控件不透明度） |
| `color` | 单色框颜色 |
| `phase` | 流光相位 |
| `saturation` / `brightness` | 流光颜色参数 |
| `pause_style` | pause_menu 风格：角标 + 强调色外框 |

## 信号 / 事件

| 名称 | 参数 | 触发时机 |
|---|---|---|
| （无） | | 纯绘制图层，没有信号；由 `Page::Render()` 每帧驱动 |

## 时序 / 约束

| 项 | 值 / 行为 |
|---|---|
| 绘制位置 | `Page::Render()` 中：`root_->DrawTree` → `OnOverlay` → `popups_.Draw` → `focus_ring_.Draw` → `toasts_.Draw` |
| 动画输入 | `Global::delta_time` 与 `Global::focused` |
| 跟随速度 | `kFollowSpeed = 26.0f`（`Anim::SmoothTo`） |
| 跳变对齐 | 目标中心位移之和大于 `kSnapDistance = 180.0f` 时直接对齐 |
| 淡入淡出 | `kFadeSpeed = 22.0f`；`alpha_ <= 0.02f` 时不画 |
| 绘制分支 | `flowing` → `Draw::FlowingRing`；`pause_style` → `kError` 外框 + 四角 L 标记；否则 `Draw::RoundedRectOutline` |
| 裁剪 | `target->ScrollHost()` 存在且 `overflow == Overflow::Scroll` 时 `PushClipRect` 到该容器 |
| 样式来源 | 组件是否流光、颜色、粗细由控件自己的 `BuildFocusVisual()` 决定 |

## 注意点

- 一帧只画一个框，`FocusVisual::alpha` 负责淡入淡出。
- 目标跳太远（`> kSnapDistance`）就直接对齐，避免彩虹框横穿整个屏幕。
- 焦点框只绘制、不参与命中测试，所以不会挡鼠标/触摸。
- 没有目标时原地淡出，并清掉基准矩形：下次换目标直接对齐，不从别处滑过来。
- 首次绘制（`has_rect_ == false`）直接对齐，不做平滑。
- `Reset()` 目前没有调用点（`Page` 未调）；页面重进需要自己调一次。
