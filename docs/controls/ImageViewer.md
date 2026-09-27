# ImageViewer

来源：`component_view/components/ImageViewer.h`（实现 `component_view/components/ImageViewer.cpp`）
一句话：通用图片浏览器控件（Empty / Loading / Loaded / Failed 四态），手柄、触屏、鼠标共用同一套状态与视图方法。

## 最小示例

```cpp
#include "component_view/components/ImageViewer.h"

using namespace gui_dev::cv;

// 独立使用：自己是一块面板（透明背景 PNG 保持 alpha）
ImageViewer* viewer = page.Root().Emplace<ImageViewer>();
viewer->SetImagePath("/roms/covers/zelda.png")
      ->SetSupportedExtensions({".png", ".jpg", ".jpeg"})
      ->SetToolbarVisible(true)
      ->SetToolbarAutoHide(true, 4.0f)   // 空闲 4s 后工具条淡出
      ->SetShowInfo(true)
      ->SetStateIndicators(true)
      ->SetConfirmEnabled(true)
      ->SetConfirmCallback([](const std::string& path) { /* 选中这张图 */ });

connect(viewer, &ImageViewer::closeRequested, &page, [&] { /* 关闭容器 / 退回上一页 */ });
connect(viewer, &ImageViewer::stateChanged, &page, [](ImageViewer::State state) { /* 状态变化 */ });

// 弹窗里使用（Popup 会把它铺满内容区，标题=文件名、副标题=尺寸·大小·缩放）
Popup* popup = Popups().ShowImageViewer("封面", "/roms/covers/zelda.png");
```

图片加载由宿主注入：`Global::image_source.load` / `release`（`ImageHandle` 带 texture / width / height / file_size / error）。
当前后端只有 libpng（PNG）；未注册 `image_source` 或不支持的扩展名直接进 `Failed`。

## API

### 状态与内容

| 成员 / 方法 | 说明 |
|---|---|
| `ImageViewer()` / `explicit ImageViewer(std::string image_path)` | 构造 |
| `enum class State { Empty, Loading, Loaded, Failed }` | 四态 |
| `SetImagePath(std::string)` | 路径变化才重新加载；空串 = `Empty` |
| `imagePath()` / `fileName()` | 完整路径 / 路径最后一段 |
| `Reload()` | 重试（Failed）或强制重新加载 |
| `state()` / `IsLoaded()` / `errorText()` | 当前状态 / 是否 Loaded / 失败原因 |
| `imageWidth()` / `imageHeight()` | 原始像素尺寸 |
| `InfoText()` | 「宽 × 高 · 文件大小 · 缩放%」，Loading 时是「正在加载…」 |

### 视图（手柄 / 触摸 / 鼠标统一入口）

| 方法 | 说明 |
|---|---|
| `SetZoom(float)` | 绝对缩放，以视口中心为基准；1.0 = 100% |
| `SetZoomAt(float, const ImVec2& focal)` | focal 相对视口中心（滚轮 / 双指用） |
| `ZoomIn()` / `ZoomOut()` | 按 `zoom_steps_` 上下取一档（非 Loaded 时不动） |
| `FitToWindow()` | 适应窗口 + 居中，`fit_mode_ = true` |
| `ActualSize()` | 100%，保留当前 pan |
| `ResetView()` | 100% + 居中 |
| `PanBy(const ImVec2&)` | 屏幕像素增量平移，自动 Clamp 到图片边界 |
| `SetZoomSteps(std::vector<float>)` | 覆盖缩放档位；会先排序，空列表忽略 |
| `zoom()` / `fitZoom()` / `pan()` / `IsFit()` | 只读结果 |

`SetZoomAt` 会把缩放夹在 `zoom_steps_.front() * 0.5 .. zoom_steps_.back() * 2.0`；默认档位
`{0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0}`。缩放值没变化时直接返回，不 emit `zoomChanged`。

### 交互 / 外观配置

| 方法 | 说明 |
|---|---|
| `SetCloseCallback(CloseCallback)` | B / `[B 关闭]` 的回调；`closeRequested` 同时 emit |
| `SetConfirmEnabled(bool)` | 默认 `false`；打开后在工具条显示 `[A 确认]` |
| `ConfirmEnabled()` | 当前值 |
| `SetConfirmCallback(ConfirmCallback)` | 确认回调，参数是图片路径 |
| `ConfirmCurrentImage()` | A 键与 `[A 确认]` 按钮的唯一入口 |
| `RequestClose()` | B 键与 `[B 关闭]` 按钮的唯一入口 |
| `SetToolbarVisible(bool)` | 默认 `true` |
| `SetToolbarAutoHide(bool, idle_seconds = 4.0f)` | 默认关闭；内部分辨率最小 0.5s |
| `SetShowInfo(bool)` | 默认 `true`；Popup 会设 false 改用 Header 副标题 |
| `SetStateIndicators(bool)` | Loading 进度条开关 |
| `SetSupportedExtensions(std::vector<std::string>)` | 默认 `{".png", ".jpg", ".jpeg"}`；自动转小写并补 `.` 前缀 |
| `ToolbarVisible()` | 工具条是否可见且不透明度 > 0.05 |

### 子结构 `ImageData`

| 字段 | 说明 |
|---|---|
| `ImTextureRef texture` | 纹理 |
| `int width` / `int height` | 原始像素尺寸 |
| `long long size_bytes` | 文件字节数（宿主 `ImageHandle::file_size`；0 = 未知，信息行不显示） |

`ConfirmCurrentImage()` 在确认已启用且 `path_` 非空时，通过 `Global::popup_manager` 弹确认框（标题「确认选择图片」，确认后回调 → `confirmRequested` → `RequestClose()`）；`popup_manager` 为空时直接回调。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `closeRequested` | — | `RequestClose()`：B 键、`[B 关闭]`、Failed 界面的 `[关闭]`、确认完成后 |
| `stateChanged` | `ImageViewer::State` | `SetState()` 里状态真正变化时（Failed 重复设置也会进） |
| `zoomChanged` | `float` | `SetZoomAt` / `FitToWindow` / `ActualSize` / `ResetView` 改变缩放时 |
| `confirmRequested` | `std::string path` | 用户在确认弹窗里点了确认之后 |
| `infoChanged` | `std::string info` | 信息行文字变化（加载完成 / 缩放变化 / 卸载清空） |

## 交互 / 输入

| 输入 | 行为 |
|---|---|
| `PageLeft`(L) | `FitToWindow()`；仅 Loaded 时消费 |
| `PageRight`(R) | `ActualSize()`；仅 Loaded 时消费 |
| `TriggerLeft`(ZL) | `ZoomOut()`；仅 Loaded 时消费 |
| `TriggerRight`(ZR) | `ZoomIn()`；仅 Loaded 时消费 |
| `ActionX`(X) | `ResetView()`；仅 Loaded 时消费 |
| `Cancel`(B) | `RequestClose()`，总是消费（确认弹窗打开时由 Popup 自己处理 B） |
| `Confirm`(A) | `ConfirmCurrentImage()`，总是消费；未启用确认时什么都不做 |
| 方向键 / 摇杆按住 | 图片超出可视区的那根轴吃掉方向键用来平移（`capture_horizontal` / `capture_vertical` 在 `RecomputeFit` 里按显示尺寸算）；未超出时方向键照常把焦点送走 |
| 指针拖动 | 画布内按下开始平移，同时置 `Global::pointer_drag_host = nullptr` 屏蔽页面级拖动滚动 |
| 鼠标滚轮 / 触控板 | 以指针位置为焦点缩放，每格 ×1.15 / ÷1.15 |
| 点工具条按钮 | 走普通 Button 的 `clicked`（触屏 / 鼠标与手柄三条路都进同一组方法） |

工具条按钮是内部 `ToolbarButton`（继承 `TextButton`）：**忽略手柄 Confirm（A）**，A 不会顺着焦点触发工具条上的按钮；按钮上显示 L / R / ZL / ZR / X / B 图标提示。双指缩放需要输入层支持多点触控。

## 尺寸 / 主题约定

| 项 | 取值 |
|---|---|
| 自身底色 / 边框 / 圆角 / 内边距 | `Theme::kBgPanel`、`Global::component_style`（`ApplyComponentBoxStyle()`） |
| 信息行高 / 字号 | 24，`Theme::kFontSmall`，色 `Theme::kTextMuted` |
| 工具条按钮高 / 图标格 | 40 / 22（比 `Theme::kControlHeight`(56) 矮一截） |
| 工具条按钮字号 | `Theme::kFontHeader`(20) |
| 状态主文案 / 明细 | `Theme::kFontHeader`(20) `kTextPrimary` / `Theme::kFontSmall`(14) `kTextMuted` |
| 状态块高 / 进度条 | 96 高；进度条宽 280、高 8、不确定态 |
| 工具条分格 | 一屏宽度内按 7 个位置等分，每格夹在 52 ~ 150 |
| 图片贴边描边 | `Theme::kBorder`（仅图片贴到画布边时画一圈淡边） |

## 注意点

- 加载是宿主 `Global::image_source` 的**同步**解码，所以 `Loading` 状态只占一帧；加载只在 `OnUpdate` 的 `LoadIfPending()` 里做，绝不在渲染阶段加载。
- 未注册 `Global::image_source`、扩展名不在白名单、`ImageHandle::Valid()` 失败都会进 `Failed`，原因放 `errorText()`（显示在状态明细行 + `[重试] [关闭]`）。
- 图片画在画布区，**不填白底**，透明 PNG 保留 alpha。
- `Empty` / `Loading` / `Failed` 时工具条只留 `[关闭]`（Failed 另有 `[重试]`）；缩放类按钮只在 Loaded 时显示，并在到边界时置灰（复用 `Widget::enabled`）。
- 组件默认铺满可用区；`MeasureContent` 里算子控件位置（同一帧生效），因为没有 flex，弹窗里高度由 Popup 按可用区给。
- 指针在画布内按下会接管拖动，避免外层滚动容器同时滚；工具条自动隐藏时 `focus_inert` 一起置位，防止焦点落进不可见按钮。
