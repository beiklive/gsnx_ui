# Toast

来源：`component_view/Toast.h`（实现 `component_view/Toast.cpp`）
一句话：右上角滑入、停留 3 秒、滑出的通知浮层，多 Toast 自动排队补位；业务代码只调 `ShowSuccess` / `ShowError` / `ShowInfo`，不关心坐标、动画、生命周期与排列。

## 最小示例

```cpp
using namespace gui_dev::cv;

// 在 Page 子类里：Toasts() 就是 Page::Toasts()（页面外写 page.Toasts()）
void MyPage::OnBuild() {
    Toasts().ShowSuccess("存档已保存");
    Toasts().ShowError("无法读取 SD 卡");
    Toasts().ShowInfo("已切换到 2 倍速");

    // 需要更细的控制时：直接 Show，或改这一份全局 ToastStyle
    Toasts().Show(ToastType::Error, "同一类型 + 同一文案");
    Toasts().Style().visible_duration = 2.0f; // 停留时长（入场完成之后才开始算）
}

// 宿主每帧驱动（Page 已经接好；手动用时顺序固定）
// toasts.Update(dt);
// toasts.Draw(dl);   // 画在 UILayer::Toast(9999)
```

## API

### 枚举

| 枚举 | 取值 | 说明 |
|---|---|---|
| `ToastType` | `Success`、`Error`、`Info` | 决定左侧色条与图标 |
| `ToastState` | `Entering`、`Visible`、`Exiting` | 单个 Toast 自己的状态机（Exiting 播完即被移除） |

`ImU32 ToastAccentColor(ToastType)`：`Success → Theme::kSuccess`、`Error → Theme::kError`、`Info → Theme::kAccent`；只给色条和图标用，不改卡片的 Box 配色。

### `ToastManager`

| 成员 / 方法 | 说明 |
|---|---|
| `Show(ToastType, std::string message)` | 追加一条；`message` 为空直接忽略 |
| `ShowSuccess(std::string)` / `ShowError(std::string)` / `ShowInfo(std::string)` | `Show` 的三个便捷包装 |
| `Update(float dt)` | 生命周期推进 + X 轴滑入/滑出 + 每帧重算 `target_y` 并平滑跟随 |
| `Draw(ImDrawList*)` | 画卡片、色条、图标、文本；`dl == nullptr` 或队列为空时不画 |
| `Clear()` / `Empty()` / `Count()` | 清空 / 是否为空 / 当前条数 |
| `Style()` / `const Style()` | 整份 `ToastStyle` 引用，改它即改全局表现 |

### `ToastStyle`（720p 手持基准，全局一份）

| 字段 | 默认 | 说明 |
|---|---|---|
| `min_width` / `max_width` / `min_height` | `200 / 320 / 44` | 卡片尺寸范围；长文本撑到 max 后换行 |
| `bar_width` / `bar_inset` / `bar_radius` / `left_radius` | `4 / 2 / 2 / 3` | 左侧色条宽 / 离边框留白 / 色条圆角 / 卡片左侧两角（右侧固定直角） |
| `icon_size` / `gap` | `22 / 10` | Material 图标字号 / 色条↔图标↔文本间距 |
| `padding_x` / `padding_y` | `16 / 8` | 色条之外的内容留白 |
| `text_size` | `Theme::kFontBody`(16) | 文本字号 |
| `enter_duration` / `exit_duration` / `visible_duration` | `0.25 / 0.25 / 3.0` | 入场 / 退场 / 停留；停留从入场完成之后开始算 |
| `reflow_speed` / `spacing` | `14 / 10` | Y 轴补位的指数趋近速度 / 多条之间的垂直间距 |
| `right_margin` / `top_margin` | `5 / 20` | 离屏幕右边缘 / 顶部的距离 |
| `dedup_window` | `0` | `0` = 不去重；`>0` 时同类型同文案在该窗口内只刷新不新建 |

### `Toast`（单条数据）

| 字段 | 说明 |
|---|---|
| `type` / `message` | 类型 / 文案 |
| `state` / `state_time` / `elapsed` | 当前状态 / 该状态已过时间 / 创建至今（去重用） |
| `width` / `height` | 按文本量出来的目标尺寸 |
| `current_y` / `target_y` | Y 轴当前位置 / 队列槽位目标（前一条消失后自动上移） |
| `slide` | X 轴：`0` = 完全在屏幕右边外，`1` = 停在目标位置 |
| `placed` | 第一帧先对齐 `target_y`，避免从 0 飞下来 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| （无） | — | Toast 是通知浮层，不定义信号，也不参与焦点与输入 |

## 交互 / 输入

| 项 | 行为 |
|---|---|
| 按键 / 指针 | Toast 不消费任何输入，也不参与焦点导航；出栈完全由计时驱动 |
| 层级 | 画在 `UILayer::Toast`(9999)，即焦点框(9900)之上，视觉最顶层 |
| 绘制内容 | 左侧状态色条 + Material 图标（Success `CheckCircle` / Error `ErrorOutline` / Info `Info`）+ 文本 |
| 线程 | `Show*` / `Update` / `Draw` 都必须在 UI 线程调用（项目 UI 单线程） |

## 尺寸 / 主题约定

| 项 | 取值 |
|---|---|
| 卡片框 | `Global::ComponentBoxVisual()` + `Theme::kBgWidget`，左侧两角 `left_radius`(3)、右侧两角直角 |
| 文本色 | `Theme::kTextPrimary` |
| 色条 / 图标色 | `Theme::kSuccess` / `Theme::kError` / `Theme::kAccent` |
| 文本字号 | `style.text_size`，为 0 时回退 `Theme::kFontBody`(16) |
| 目标位置 | 右边缘 = 显示屏宽 - `right_margin`；Y 从 `top_margin` 起按 `height + spacing` 依次排 |

## 注意点

- 停留时间从**入场动画完成**之后才开始算（`state_time` 进 Visible 时归零），不是从 `Show` 起算。
- 新 Toast 第一帧直接落在自己的槽位上，不从 y = 0 飞下来（`placed` 标记）。
- X 轴用定时长 + `EaseOutCubic`，Y 轴用指数趋近（`SmoothTo`），两套动画互不影响。
- `dedup_window` 默认 `0`：每次 `Show` 都新建一条；设成 `>0` 后同类型同文案在该窗口内只刷新停留时间，正在 Exiting 的会被重新拉回 Entering。
- `message` 为空时 `Show` 直接返回，不建 Toast。
- Toast 是 UI 层服务：所有 `Show*` / `Update` / `Draw` 都必须在 UI 线程调用；以后真有后台线程要发通知，再在 `Show*` 前挂线程安全队列。
