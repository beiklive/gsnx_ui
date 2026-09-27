# Popup / PopupManager

来源：`component_view/popup/Popup.h`、`component_view/popup/PopupManager.h`（实现 `component_view/popup/Popup.cpp`、`component_view/popup/PopupManager.cpp`）
一句话：统一弹窗基类 + 弹窗栈管理器；遮罩 + 窗口 Box + 类型条 + 标题 + 内容容器 + 按钮组只有一套结构，Info / Confirm / Selection / Progress / 自定义页都只是不同配置。

## 最小示例

```cpp
using namespace gui_dev::cv;

// 确认框：按钮顺序固定 [取消, 确认]，确认按钮 primary，默认焦点 0 = 取消
Popup* p = Popups().ShowConfirm("删除游戏", "删除后存档也会一起清掉，确定吗？",
                                [this] { DeleteGame(); }, "删除", "取消");
p->setDefaultFocus(0);
// 结果回调：下标 0 = 取消 / 1 = 删除（先 emit buttonClicked，再调按钮自己的 on_click）
connect(p, &Popup::buttonClicked, this, [](int index) { if (index == 1) { /* 已确认 */ } });

// 进度弹窗：默认没有按钮，生命周期由任务控制（UI 线程调用）
Popup* prog = Popups().ShowProgress("正在扫描 ROM", "已找到 0 个游戏");
prog->setProgress(0.42f)->setMessage("已找到 128 个游戏");
prog->fail("无法读取 SD 卡");   // 错误态 + 自动补一个「关闭」

// 自定义页：builder 拿到内容容器 Box，塞现有控件
Popups().ShowCustom("高级选项", [](Widget& c) { c.Emplace<Label>("放什么控件都行"); }, PopupKind::Warning);
```

## API

### `PopupKind`（只决定类型条颜色 + 默认 Header 图标，不决定结构）

| 取值 | 语义色 |
|---|---|
| `Custom` / `Info` | `Theme::kPopupNeutral`（灰）/ `kPopupInfo`（蓝） |
| `Success` / `Warning` | `Theme::kPopupSuccess`（青绿）/ `kPopupWarning`（黄） |
| `Error` / `Confirm` | `Theme::kPopupError`（红）/ `kPopupConfirm`（蓝） |
| `Selection` / `Progress` | `Theme::kPopupSelection`（紫）/ `kPopupProgress`（蓝） |

`PopupAccentColor(PopupKind)` / `PopupAccentU32(PopupKind)` 是自由函数，供外部取同一套语义色。

`PopupState`：`Opening`、`Visible`、`Closing`、`Closed`。`PopupScope`：`Page`（被 `ClosePageScoped()` 关）、`Global`（常驻）。`PopupButtonLayout`：`Horizontal`（默认）、`Vertical`（选项列表）。

### 配置（全部返回 `*this`，可链式）

| 方法 | 说明 |
|---|---|
| `setKind` / `setHeaderIcon` | 换类型（刷新 Header 描边与图标）/ 覆盖按 kind 自动给的图标 |
| `setTitle` / `setHeaderText` / `setHeaderSubtitle` | Header 主文字（空串自动隐藏）/ 同义 / 副文字（小号浅色） |
| `setModal` / `setBackdrop` / `setDismissOnBackdrop` / `setDismissOnCancel` / `setAutoCloseOnButton` | 模态默认 true；遮罩 alpha `-1` = 不改；点遮罩默认 false；B/Esc 默认 true；按钮自动关默认 true |
| `setScope` / `setSize` / `setMinSize` / `setMaxSize` | 默认 `Page`；`setSize`：`0` = 自适应、(0,1] = 画布比例、>1 = 像素；`setMinSize` 直接给像素下限；`setMaxSize` 的 height >1 视为像素 |
| `setButtonLayout` / `setDefaultFocus` / `setAnimated` | 横排（默认）/ 竖排；默认焦点按钮下标；关动画后 `Open`/`Close` 直接切状态 |
| `setStyle` / `style()` / `setFrameless` | 覆盖整份样式；无框模式（不画底/边框/阴影、内边距归零，ImageViewer 用） |

### 内容

| 方法 | 说明 |
|---|---|
| `setText(std::string)` | 自动换行的正文 Label |
| `setMarkdown(md, RichText::ImageResolver = {}, view_height = 0.0f)` | RichText 渲染；外层 Box 是滚动容器，焦点给正文并打开上下键滚动 |
| `setImageViewer(std::string path)` | ImageViewer 铺满内容区；标题 = 文件名、副标题 = 图片信息，关闭直接关弹窗 |
| `setImage(ImTextureRef, w, h)` | 图片，圆角走 `component_style.corner_radius`，限高 320 |
| `setProgressContent(message, indeterminate = false)` | 说明行 + 进度条 |
| `setContentBuilder(std::function<void(Widget&)>)` / `content()` | 自定义页；参数 / 返回值就是内容容器 `Box` |

### 进度 / 按钮 / 生命周期

| 方法 | 说明 |
|---|---|
| `setProgress(0..1)` / `setIndeterminate(bool)` / `setMessage(std::string)` | 进度弹窗专用；`setProgress` 会关掉不确定态 |
| `fail(std::string)` | 切 `Error`、进度条变红、设消息；仅按钮为空时补「关闭」并 `setDefaultFocus(0)` |
| `addButton(ButtonSpec)` / `addButton(text, on_click = {})` / `buttonCount()` / `buttonAt(int)` | 追加按钮；数量；`Button*`（越界返回 nullptr） |
| `Open()` / `Close()` / `CloseNow()` | Opening / Closing / 立即 Closed（无动画时直接跳） |
| `IsOpen` / `IsOpening` / `IsClosing` / `IsClosed` / `name()` / `setName()` / `kind()` / `scope()` / `state()` | 状态与只读属性查询 |
| `IsModal()` / `openProgress()` / `layerZ()` | 模态 / 动画进度 / 渲染层级 |
| `SyncVisualStyleFromGlobal()` / `RequestDismiss()` / `WantsDismiss()` / `dismissOnCancel()` / `Root()` / `window()` / `contentWidget()` | 同步全局视觉参数（构造时 + 切主题时）；请求关闭（先 emit 再 `Close()`）；`WantsDismiss` 由 Manager 消费；后三个是内部节点，不要直接调 |

### `ButtonSpec`

| 字段 | 说明 |
|---|---|
| `text` / `icon` | 文字 / Material 图标字形 |
| `on_click` | 点击回调（晚于 `buttonClicked`，早于自动关闭） |
| `color` / `primary` | 按钮文字色（alpha = 0 用默认配色）/ 主按钮：语义色描边 + 文字 |
| `close_on_click` | 默认 `true`；与 `auto_close_on_button_` 同时为真才自动 `Close()` |

### `PopupStyle`

| 字段 | 默认 |
|---|---|
| `width` / `height` / `min_width` / `max_width` / `min_height` | `0 / 0 / 380 / 640 / 0`；`max_width_ratio` / `max_height_ratio` = `0.74 / 0.82` |
| `padding` / `gap` / `header_size` / `header_gap` | `24 / 16 / 32 / 12`；`corner_radius` / `backdrop_alpha` / `backdrop` = `5 / 0.55 / true` |
| `title_size` / `body_size` / `markdown_height` / `image_max_height` | 20 / 16 / 300 / 300 |
| `button_height` / `button_min_width` / `button_gap` / `button_row_gap` | `kControlHeight`(56) / 132 / 12 / 10 |
| `open_duration` / `close_duration` / `open_scale_from` / `open_translate_y` / `animated` | `0.20 / 0.14 / 0.96 / 16 / true` |

构造时 `SyncVisualStyleFromGlobal()` 会把 `padding / header_size / header_gap / gap / min_width / max_width_ratio / max_height_ratio / backdrop_alpha / button_min_width / corner_radius` 覆盖成 `Global::component_style` 的 `popup_*`（默认 `24 / 32 / 12 / 16 / 300 / 0.74 / 0.82 / 0.55 / 132 / 5`）。

### `PopupManager`（`Page::Popups()`）

| 方法 | 说明 |
|---|---|
| `Show(std::unique_ptr<Popup>)` | 接管所有权；同名自动加 `_2` 后缀 |
| `ShowInfo(title, message, button_text = "确定")` | Info；`setDismissOnBackdrop(true)` |
| `ShowConfirm(title, message, on_confirm, confirm_text = "确认", cancel_text = "取消")` | 按钮顺序 `[取消, 确认]`，确认 `primary`，`setDismissOnBackdrop(false)`，`setDefaultFocus(0)` |
| `ShowSelection(title, message, options, layout = Vertical)` | Selection；逐个 `addButton`，`setDefaultFocus(0)`，message 为空不加正文 |
| `ShowProgress(title, message, indeterminate = false)` | 无按钮、`setDismissOnBackdrop(false)` + `setDismissOnCancel(false)`，由任务 `Close()` |
| `ShowImageViewer(title, image_path, kind = Info)` | 尺寸 0.88×0.94，B / 点遮罩可关 |
| `ShowMarkdown(...)` / `ShowImage(title, texture, w, h)` / `ShowCustom(title, builder, kind = Custom)` | RichText 正文 / 静态图片 / 自定义 builder；三者都追加 `[关闭]` |
| `Top()` / `Find(name)` / `Count()` / `Empty()` / `CloseTop()` / `Close(name)` / `CloseAll()` / `ClosePageScoped()` | 查询与关闭；`ClosePageScoped` 只关 `Scope::Page` |
| `BlocksBackground()` / `AnyModal()` | 是否存在「模态且不在 Closed/Closing」的弹窗；两者等价 |
| `HitTest` / `CollectFocusables` / `UpdateTree` / `HandleDismiss` / `Advance` / `Draw` / `EnsureVisible` | 每帧流程，`Page` 按顺序调用（顺序保证输入优先级） |
| `defaults()` / `RefreshTheme()` | 默认 `PopupStyle&`，所有 `Show*` 先 `setStyle(defaults_)`；切主题转发给栈内弹窗 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `opening` / `opened` | — | `Open()` 开始 / 动画播完（无动画时同帧连续发） |
| `closing` / `closed` | — | `Close()` 开始 / 动画播完（`CloseNow()` 直接发 `closed`） |
| `buttonClicked` | `int index` | 任意按钮点击，先于该按钮的 `on_click` 与自动关闭 |
| `dismissRequested` | — | B / Esc、点遮罩触发 `RequestDismiss()` 时 |

关闭 / 结果语义：`Close()` 只把状态推成 `Closing`，动画播完由 `PopupManager::Advance()` → `RemoveClosed()` 真正删除并弹出焦点作用域；「结果」通过按钮的 `on_click` 与 `buttonClicked(index)` 给出，弹窗本身不保存选择结果。

## 交互 / 输入

| 输入 | 行为 |
|---|---|
| B / Esc、点遮罩 | `PopupManager::HandleDismiss()`：栈顶弹窗 `dismissOnCancel` 为真、且控件没消费掉按键时 `RequestDismiss()` 并 `MarkConsumed`；点遮罩等价于 `dismiss_on_backdrop_ && IsOpen()` → `RequestDismiss()` |
| A | 走普通 Button 焦点激活（弹窗按钮就是普通 `TextButton`） |
| 焦点（Focus Trap） | 每个弹窗入栈时 `FocusManager::PushScope("popup:" + name, &Root())`；只有栈顶弹窗内部控件可聚焦，整栈关完恢复到第一个弹窗打开前的焦点 |
| 输入优先级 | 只更新栈顶弹窗；`Closing` 期间整棵树不收输入；渲染仍按栈序从底到顶；`BlocksBackground()` 为真时背景 UI 不接受 hover / 点击，模态弹窗命中不到控件时 `HitTest` 返回 nullptr |

渲染层级：弹窗 `5000 + 栈序 × 10`（上限 9000）、焦点框 9900、Toast 9999（只画不吃输入），见 `component_view/UILayer.h`。

## 尺寸 / 主题约定

| 项 | 取值 |
|---|---|
| 语义色 | `Theme::kPopupInfo / kPopupSuccess / kPopupWarning / kPopupError / kPopupConfirm / kPopupSelection / kPopupProgress / kPopupNeutral` |
| 窗口底 / 边框 / 阴影 / 圆角 | `Global::component_style`（与 Box / Button 同一套） |
| 遮罩 | `ImVec4(0.03, 0.03, 0.04, backdrop_alpha)`，默认 alpha 0.55 |
| 文字 | 正文 `Theme::kTextPrimary`、副文字 / 进度说明 `Theme::kTextMuted`、进度条失败色 `Theme::kError` |
| Header 图标 Box | 边长 `popup_header_size`(32)，透明底 + 语义色 1.5px 描边，图标按墨迹居中 |

## 注意点

- 生命周期是状态机 `Opening → Visible → Closing → Closed`，动画播完才由 Manager 回收；不能用 `if (open) draw()` 代替。
- 自适应高度有个已知坑：测量阶段窗口高度先钉在高度上限，否则 Image 这类按 available 反向撑满的控件会让弹窗反复变高变矮（图片和底部按钮上下抖）。
- `setStyle()` 单独改过的参数会在下次 `SyncVisualStyleFromGlobal()`（切主题）被覆盖。
- 关闭动画期间不收输入，背景此时也不该被操作；弹窗名字必须唯一（作用域、`Find` / `Close` 都按名字定位），`Show()` 会自动改名。
