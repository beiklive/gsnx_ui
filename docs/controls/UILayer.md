# UILayer（UI 层级 / Z-Order）

来源：`component_view/UILayer.h`（无同名 `.cpp`：枚举、常量与函数全部是头文件内 `inline` / `constexpr`）
一句话：定义渲染层级常量与绘制顺序，`Page::Render()` 按它从下往上画；输入优先级与焦点归属不在这里定义。

## 典型用法

```cpp
// 层级只在 Page::Render() 里集中使用（component_view/pages/Page.cpp 注释）：
//   Background(0) → Content(1000) → Popup(5000~9000) → Focus(9900) → Toast(9999)
// 顺序在这里集中维护：控件内部不要自己决定谁在上面。

// 弹窗栈第 index 层的渲染层级（0 = 最下面那个弹窗）
const int z = PopupLayerZ(index);   // 5000 + index * 10，夹在 Popup..Focus 之间

// 由数值反查所属层级（调试 / 断言用）
const UILayer layer = LayerOf(z);
```

## API

| 成员 / 方法 | 说明 |
|---|---|
| `enum class UILayer` | `Background = 0` / `Content = 1000` / `Popup = 5000` / `Focus = 9900` / `Toast = 9999` |
| `kPopupLayerStep` | 弹窗栈每深一层加多少，`inline constexpr int = 10` |
| `kPopupLayerLimit` | 弹窗层级上限，`inline constexpr int = 9000` |
| `LayerZ(UILayer layer)` | 取层级数值（`static_cast<int>`） |
| `PopupLayerZ(int stack_index)` | `LayerZ(Popup) + stack_index * kPopupLayerStep`，超过 `kPopupLayerLimit` 时夹到上限 |
| `LayerOf(int z)` | 由数值反查所属层级（调试 / 断言用） |

## 信号 / 事件

| 名称 | 参数 | 触发时机 |
|---|---|---|
| （无） | | 纯常量与 `constexpr` 函数，没有运行时行为 |

## 时序 / 约束

| 层级 | 数值范围 | 内容 |
|---|---|---|
| `Background` | 0 ~ 999 | 背景 / 底板（页面底色、壁纸） |
| `Content` | 1000 ~ 4999 | 普通 UI（页面组件树、页面 overlay） |
| `Popup` | 5000 ~ 9000 | 模态弹窗，栈内每层 `+kPopupLayerStep`，越高越靠上 |
| `Focus` | 9900 | 焦点框 Overlay（必须在弹窗之上） |
| `Toast` | 9999 | 全局通知（视觉最顶层） |

- 渲染顺序：`Page::Render()` 严格按 `Background → Content → Popup → Focus → Toast` 从下往上画。
- 输入优先级与渲染顺序不同：`Popup → Content`（Toast 默认不拦输入），见 `PopupManager::HitTest()` / `Page::Update()`。
- 焦点归属由 `FocusManager` + `Widget::CollectFocusables` 决定，与渲染层级分开维护。

## 注意点

- 数值即渲染顺序，越小越先画、越靠下；不要用数值以外的约定决定谁在上面。
- 焦点框画在弹窗之上：弹窗里的按钮有焦点时框不会被弹窗压住；但焦点框只绘制，不参与命中测试——视觉最上层 ≠ 拦输入。
- `PopupLayerZ()` 超过 `kPopupLayerLimit` 会夹到 9000，不会盖过 `Focus` / `Toast`。
- `LayerOf()` 的判定从高到低，`Popup` 区间与具体层级数值相关，仅用于调试/断言。
