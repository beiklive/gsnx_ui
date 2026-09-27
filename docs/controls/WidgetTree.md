# WidgetTree（父子树与事件）

来源：`component_view/Widget.h`（实现 `component_view/Widget.cpp`）+ `component_view/Object.h`
一句话：`Widget` 通过 `parent` / `children` 组成树，树负责布局、更新、绘制、命中；跨对象通信走 `Object.h` 的 Qt 风格信号槽。框架总览见 [component-view.md](../component-view.md)。

## 最小示例

```cpp
// 建树：Emplace 返回裸指针，树持有 unique_ptr（examples/min_demo/main.cpp）
void OnBuild() override {
    tab_ = Root().Emplace<TabColumn>();
    toasts_ = Root().Emplace<TextButton>("弹一个 Toast");

    // 信号槽：sender、信号、context、槽（带 context 的连接在 context 析构时自动失效）
    connect(toasts_, &Widget::clicked, this, [this] { Toasts().ShowSuccess("按钮被按下"); });
}

// 发射（emit 是空宏，等价于 signalObj(args)）
emit clicked();
```

## API

**树（`component_view/Widget.h`）：**

| 成员 / 方法 | 说明 |
|---|---|
| `Widget* parent` | 父节点，`Add()` 时自动设置 |
| `std::vector<std::unique_ptr<Widget>> children` | 子节点（所有权在这里） |
| `Widget& Add(std::unique_ptr<Widget> child)` | 挂一个子节点，返回 `*this` |
| `template <typename T, typename... Args> T* Emplace(Args&&...)` | 构造并挂入子节点，返回裸指针 |
| `Widget* Find(const std::string& name)` | 按 `name` 深度优先查找（含自己） |
| `template <typename T> T* FindAs(const std::string& name)` | `Find` + `dynamic_cast` |
| `void Remove(Widget* child)` | 从 `children` 中移除该节点（按指针） |
| `void Clear()` | 清空全部子节点 |
| `bool ContainsDescendant(const Widget* target) const` | target 是否在本节点子树内（不含自己） |
| `void LayoutTree(const ImVec2& parent_content_pos, const ImVec2& parent_content_size)` | 测量 + 定位整棵子树 |
| `void UpdateTree(float dt)` | 每帧更新整棵子树 |
| `void DrawTree(ImDrawList* dl)` | 绘制整棵子树 |
| `void RefreshThemeTree()` | 递归 `OnThemeChanged()`，切主题后宿主调一次 |
| `void CollectFocusables(std::vector<Widget*>& out)` | 收集可聚焦节点（先自己后子节点） |
| `Widget* HitTest(const ImVec2& p)` | 命中测试，子节点优先 + `z_order` 高者优先 |
| `void Move(const ImVec2& delta)` | 平移整棵子树 |
| `bool EnsureVisible(Widget* target)` | 把本子树内的 target 滚进可见区 |
| `void EnsureRectVisible(const Rect& target_rect)` | 把任意矩形滚进内容区 |
| `void ScrollPage(int direction, float scale = 1.0f)` | 翻页，`direction=-1` 上一页 / `+1` 下一页 |
| `Widget* ScrollHost()` | 最近的滚动容器祖先（自己也算） |

**对象与信号槽（`component_view/Object.h`）：**

| 成员 / 方法 | 说明 |
|---|---|
| `class Object` | 能当接收者 / context 的基类（等价 `QObject`） |
| `Signal<Args...>` | 信号成员；不可拷贝 |
| `connect(Receiver*, Return (Receiver::*)(Args...))` | 连成员函数（要求接收者继承 `Object`） |
| `connect(Receiver*, Return (Receiver::*)())` | 连无参槽（信号有参数时） |
| `connect(Context*, Callable)` | 带 context 的 lambda，context 析构自动失效 |
| `connect(Callable)` | 纯 lambda，生命周期自理 |
| `emitSignal(Args...)` / `operator()(Args...)` | 发射；`emit sig(args)` 展开为 `sig(args)` |
| `disconnectAll()` / `count()` / `empty()` | 信号侧：断开 / 有效连接数 |
| `Connection::disconnect()` / `connected()` | 连接句柄（`QMetaObject::Connection`） |
| `connect(sender, &S::signal, receiver, slot)` 等自由函数 | `QObject::connect` 形态，信号可来自基类 |

## 信号 / 事件

`Widget` 声明的信号（全部是 `Signal<>`，无参数）：

| 名称 | 参数 | 触发时机 |
|---|---|---|
| `clicked` | 无 | 指针：悬停且 enabled 时松开（拖动中取消）；手柄：`Confirm` 未被 `OnPadAction` 消费时（`Activate()` 之后） |
| `pressed` | 无 | 悬停在控件上且本帧按下鼠标左键 |
| `released` | 无 | 本帧松开鼠标左键（无论是否命中） |
| `hoverEntered` | 无 | `Global::hovered` 变为自己 |
| `hoverLeft` | 无 | 自己失去 hover |
| `focusIn` | 无 | `Global::focused` 变为自己 |
| `focusOut` | 无 | 自己失去焦点 |
| `enabledChanged` | 无 | `enabled` 发生跳变（不是每帧发） |

## 时序 / 约束

`UpdateTree(dt)` 顺序（每个节点递归）：

| 顺序 | 动作 |
|---|---|
| 1 | `UpdateScroll(dt)` |
| 2 | `UpdateInteraction(dt)`（hover / focus / down、`clicked` 等信号） |
| 3 | 遍历 `children`，只对 `visible` 的子节点递归 `UpdateTree(dt)` |
| 4 | 自己 `visible` 时 `OnUpdate(dt)` |

绘制顺序（`DrawTree`）：自身背景 → `OnDrawContent` → `DrawChildren`（按 `z_order` 升序稳定排序）→ `OnDrawOverlay` → 滚动条。

- `LayoutTree()` = `Measure()` + `Place()`；`position` / `anchor` / `pivot` / `offset` 相对父节点内容区，绝对矩形每帧算好放在 `rect`。
- `CollectFocusables()`：`!visible || !enabled || focus_inert` 直接跳过；`focus_only_self` 时收集完自己就返回。
- `HitTest()`：`!visible || !enabled` 返回 nullptr；子节点按 `z_order` 从高到低测试。
- `z_order` 只影响绘制与命中顺序，不改变 `UpdateTree` 的遍历顺序。

## 注意点

- 不借用 ImGui item 机制（不调 `InvisibleButton`）：命中测试自己做，才能按 `z_order` / 子节点优先决定谁被点中。
- `size` 是外框尺寸（含 padding/border），0 表示按内容自适应。
- `Emplace` 返回的裸指针只在节点仍挂在树上时有效；`Remove` / `Clear` 之后立即失效。
- 接收者 / context 必须继承 `Object`（`connect` 里有 `static_assert`），否则析构时不会自动断开。
- 槽参数可以少于信号，参数类型必须能隐式转换。
- `Signal` 不可拷贝；`emit` / `signals` / `slots` 是只在包含 `Object.h` 的翻译单元里生效的宏。
