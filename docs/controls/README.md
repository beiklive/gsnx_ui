# 控件 API 参考（component_view）

每个控件一页，内容全部来自仓库代码：公开字段、链式方法、信号、尺寸/主题约定、代码注释里点明的坑。
**不是**设计说明——想了解「为什么长这样、从 GBAStation 的哪段搬来」请看
[`component_view/components/*.md`](../../component_view/components/)（CardCarousel / FunctionBar / ImageViewer / RichText）
和框架总览 [`docs/component-view.md`](../component-view.md)。

## 阅读顺序建议

`Widget`（基类：树 / 布局 / 焦点 / 事件 / 绘制）→ `Box`（容器）→ 基础内容件（Label / Separator /
ProgressBar / Image）→ `Button`（8 种形态）→ 组合件（Header / TabColumn / CapsuleTabs / CardCarousel /
FunctionBar / Badge）→ 富文本与图片（RichText / ImageViewer）→ 弹窗与提示（Popup / Toast）→
宿主层（Page / FocusRing / UILayer / WidgetTree）。

## 基类与结构

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Widget` | `component_view/Widget.h` | 所有组件的基类：几何 / 内边距 / 圆角 / 边框 / 阴影 / 溢出滚动 / 焦点 / 命中 / 绘制 / 事件 | [Widget.md](Widget.md) |
| 组件树与时序 | `component_view/Widget.h`、`component_view/Object.h` | 父子树、每帧 `UpdateTree` 顺序、信号槽（`connect` / `emit`） | [WidgetTree.md](WidgetTree.md) |

## 容器与基础内容

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Box` | `component_view/components/Box.h` | 最基础的矩形容器：底色 / 圆角 / 边框 / 阴影，可选变成可聚焦控件 | [Box.md](Box.md) |
| `Label` | `component_view/components/Content.h` | 单行 / 自动换行 / 多行 / 图标 + 文本的静态文本 | [Label.md](Label.md) |
| `Separator` | `component_view/components/Content.h` | 水平 / 垂直分隔线 | [Separator.md](Separator.md) |
| `ProgressBar` | `component_view/components/Content.h` | 确定进度 / 不确定（跑马）进度条 | [ProgressBar.md](ProgressBar.md) |
| `Image` | `component_view/components/Content.h` | 图片：等比缩放 / 居中 / 限高 / 裁剪（Cover）/ 占位框 | [Image.md](Image.md) |
| `RichText` | `component_view/components/RichText.h` | 单文件 Markdown 子集富文本（标题 / 列表 / 引用 / 代码 / 图片 / 粗体） | [RichText.md](RichText.md) |

## 按钮

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Button` + 8 种形态 | `component_view/components/Button.h` | 纯文字 / 图标+文字 / 纯图标 / 开关 / 自定义右侧文字 / LR 选项 / LR 数值 / 文件列表行 | [Button.md](Button.md) |
| `Badge` | `component_view/components/Badge.h` | 机种徽标（标签） | [Badge.md](Badge.md) |

## 导航与组合件

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Header` | `component_view/components/Header.h` | 区块标题：竖条 + 标题 + 右侧补充文字 + 分隔线 | [Header.md](Header.md) |
| `TabColumn` | `component_view/components/TabColumn.h` | 左侧纵向 Tab 列：单选 + 选中底 + 焦点分区 + 滚动 | [TabColumn.md](TabColumn.md) |
| `CapsuleTabs` | `component_view/components/CapsuleTabs.h` | 横向胶囊标签条，选中项停在中间并放大 | [CapsuleTabs.md](CapsuleTabs.md) |
| `CardCarousel` | `component_view/components/CardCarousel.h` | 游戏卡牌行：选中卡居中 / 焦点缩放 / 流光框 / 触摸拖动 / 长按连发 | [CardCarousel.md](CardCarousel.md) |
| `FunctionBar` | `component_view/components/FunctionBar.h` | 胶囊容器里的功能按钮行，名字只在聚焦时显示 | [FunctionBar.md](FunctionBar.md) |

## 媒体

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `ImageViewer` | `component_view/components/ImageViewer.h` | 图片浏览器：缩放 / 平移 / 适应窗口 / 实际尺寸 / 关闭 | [ImageViewer.md](ImageViewer.md) |

## 弹窗与提示

| 控件 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Popup` + `PopupManager` | `component_view/popup/Popup.h`、`component_view/popup/PopupManager.h` | 统一弹窗系统：Info / Confirm / Selection / Progress / 富文本 / 图片 / 自定义页 + 弹窗栈 + Focus Trap | [Popup.md](Popup.md) |
| `Toast` + `ToastManager` | `component_view/Toast.h` | 通知条：成功 / 信息 / 警告 / 错误，自动消失、去重、堆叠 | [Toast.md](Toast.md) |

## 页面与宿主层

| 名称 | 头文件 | 一句话 | 文档 |
|---|---|---|---|
| `Page` | `component_view/pages/Page.h` | 页面基类：一棵组件树 + 页级输入 / 更新 / 绘制 + 弹窗 / Toast / 焦点作用域 | [Page.md](Page.md) |
| `FocusRing` | `component_view/FocusRing.h` | 焦点框图层：统一画流光框 / 角标框，画在弹窗之上、Toast 之下 | [FocusRing.md](FocusRing.md) |
| `UILayer` | `component_view/UILayer.h` | 层级常量与绘制顺序 | [UILayer.md](UILayer.md) |

## 框架层（不是控件，按需查）

这些直接看 [docs/component-view.md](../component-view.md)（分层、主题、输入、后端接口、接入别的项目清单）：

| 文件 | 作用 |
|---|---|
| `component_view/Theme.h` | 调色板（浅 / 深）+ 全部尺寸与字号常量 |
| `component_view/Global.h` | 画布尺寸、鼠标 / 手柄、焦点、输入消费、`BeginFrame` / `EndFrame` |
| `component_view/Draw.h` | 绘制原语：`ComponentBox`、文本、跑马灯、渐变流光框、阴影、裁剪 |
| `component_view/Anim.h` | 时长 / 缓动 / `MoveTowards` / 错开入场 |
| `component_view/Types.h` | `Rect` / `EdgeInsets` / 边框 / 阴影 / 变换 / 枚举 |
| `component_view/Format.h` | `FormatFileSize` 等格式化 |
| `component_view/FocusManager.h` | 焦点作用域（Focus Trap / 焦点恢复） |
| `framework/` | `Backend` 接口、SDL2 后端、`UiContext`、字体 / 图标、`App` / 场景栈 |

## 三条通用约定

1. **设计空间 720p**：组件里写的一切数字（56 行高、18 字号…）都是 1280×720 设计空间的值，
   后端按平台策略换算成实际分辨率（见 [docs/component-view.md](../component-view.md) 的「定标策略」）。
2. **颜色一律读 `Theme::` 角色色、尺寸读 `Theme::` 常量**：切主题时整棵树自动跟随；
   显式设色（`setTextColors` / `fillWith`）会关掉跟随开关，切主题需要宿主自己再调一次。
3. **组件只做三件事**：`MeasureContent()` 量内容、`OnDrawContent()` 画内容、`OnUpdate()` 每帧状态；
   布局 / 焦点 / 溢出滚动 / 命中测试 / 事件分发都由 `Widget` 基类处理。
