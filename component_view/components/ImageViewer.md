# ImageViewer

通用图片浏览器 Widget：`component_view/components/ImageViewer.{h,cpp}`

---

## 1. 功能

`Empty / Loading / Loaded / Failed` 四态；
Fit（等比适应）、100%、25%~400% 分档缩放、Pan（带边界）、Reset；
底部工具条（适应屏幕 / 100% / − / ＋ / 重置 / 关闭）+ 顶部信息（文件名 · 尺寸 · 缩放）；
手柄 / 触屏 / 鼠标 **同一套状态与同一组 API**；可作为 Popup 内容使用。

## 2. 支持格式

| 扩展名 | 状态 |
|---|---|
| `.png`（含大写 `.PNG`） | ✅ 当前后端可用（libpng） |
| `.jpg` / `.jpeg`（含大写） | ⚠️ ImageViewer 接受该路径，但**当前后端没有 JPG 解码器** → 进 Failed 并显示明确原因 |

扩展名大小写不敏感（`SetSupportedExtensions` 可配置）。不支持的扩展名在加载前就被拒绝，不会走到解码器。

## 3. 基本 API

```cpp
ImageViewer* viewer = parent.Emplace<ImageViewer>("img/cover.png");
viewer->size = ImVec2(800, 520);              // 或交给容器（默认铺满可用区）
viewer->SetImagePath("img/cover.png");        // 路径变化才重新加载
viewer->Reload();                             // Failed 重试
viewer->SetZoom(2.0f);                        // 绝对缩放（1.0 = 100%）
viewer->ZoomIn(); viewer->ZoomOut();          // 分档缩放
viewer->FitToWindow(); viewer->ActualSize();  // 适应屏幕 / 100%
viewer->ResetView();                          // 100% + 居中
viewer->PanBy(ImVec2(dx, dy));                // 平移（屏幕像素）
viewer->SetCloseCallback([] { /* 关闭 */ });  // 或监听 closeRequested 信号
```

只读：`state()` / `errorText()` / `imageWidth()` / `imageHeight()` / `fileName()` / `zoom()` / `pan()` / `IsFit()`。

## 4. 图片加载

组件层不碰平台接口，宿主注册一次即可（demo 里就是这么做的）：

```cpp
Global::image_source.load = [&](const char* path) {
    Global::ImageHandle handle;
    // 用 Backend::LoadTexture + 自己的缓存保活；失败时填 handle.error
    return handle;
};
Global::image_source.release = [](Global::ImageHandle&) {};  // 缓存由宿主持有可留空
```

## 5. Fit

`fit = min(视口宽 / 图宽, 视口高 / 图高)`，图片居中；开启 Fit 后窗口尺寸变化会自动重新适应。
`fitZoom()` 可读到当前 Fit 比例。

## 6. Zoom

分档默认 `25 / 50 / 75 / 100 / 125 / 150 / 200 / 300 / 400%`（`SetZoomSteps()` 可改）。
按钮 / L·R·ZL·ZR / 滚轮 / 触屏按钮都走同一个 `SetZoom()`；滚轮以指针位置为焦点（`SetZoomAt`）。

## 7. Pan

图片大于视口才允许平移，并夹在边界内（`max_pan = max(0, (显示尺寸 − 视口)/2)`），不会失控跑飞。
手柄长按方向键按 dt 连续平移；触屏 / 鼠标拖动按指针增量平移。

## 8. Reset

`ResetView()` = 100% + 居中（与 Fit 不同）；Fit 用 `FitToWindow()`，100% 用 `ActualSize()`。

## 9. Gamepad 操作（走现有 Action，无平台按键硬编码）

| 输入 | Action | 行为 |
|---|---|---|
| 方向键 / 左摇杆 | Up/Down/Left/Right | 图片超出视口时平移；否则移动焦点到工具条按钮 |
| L / ZL | PageLeft / TriggerLeft | 缩小 |
| R / ZR | PageRight / TriggerRight | 放大 |
| X | ActionX | 重置 |
| Y | ActionY | 适应屏幕 |
| A | Confirm | 适应屏幕 / 100% 切换 |
| B | Cancel | 返回（Popup 里由 PopupManager 关弹窗） |

## 10. Touch 操作

单指拖动 = 平移；点工具条按钮 = 缩放 / 重置 / 适应 / 关闭；点图片 = 让工具条重新出现。
按钮本身就是 `Button` 控件（视觉尺寸与焦点框一致，命中区 ≥ 逻辑 56px），不做"隐形的放大命中区"以免重叠。
**双指捏合缩放未实现**：当前输入层只上报单指（`InputFrame::touch` 是一个点），需要输入层支持多点触控后再接，
届时只需把两点中心喂给 `SetZoomAt`，ImageViewer 的其余逻辑不变。

## 11. Mouse 操作

左键拖动 = 平移；滚轮 / 触控板 = 缩放（以指针为焦点）；点按钮 = 操作；Esc/B = 返回。

## 12. Focus

工具条按钮是普通 `Button`（焦点、禁用、流光框、触摸点击全部复用现有系统）。
ImageViewer 自己是"图片区"的焦点停靠点：图片超出视口的那根轴会被它吃掉（`capture_horizontal/vertical`），
用来平移；没超出时方向键照常把焦点送去按钮。Focus Frame 是绘制层，不拦截命中测试。

## 13. Popup 集成

```cpp
Popups().ShowImageViewer("查看图片", "img/cover.png");
// Popup 打开 → 焦点进 ImageViewer；关闭（关闭按钮 / B / 点遮罩）→ 恢复之前的焦点
```

底层就是 `Popup::setImageViewer()`：ImageViewer 按弹窗可用高度铺满，关闭按钮直接 `Popup::Close()`；
没有第二套弹窗系统。

## 14. Texture Cache

复用宿主自己的 `TextureRef` 缓存（demo 里 ImageViewer 与 RichText 图片共用同一份 `std::map<std::string, TextureRef>`）。
组件不做缓存、不做引用计数，也不清空宿主的缓存。

## 15. 生命周期

`SetImagePath` → 标记 pending → `OnUpdate` 里加载（**绝不在渲染阶段加载**）→ Loaded / Failed。
路径变化或 `Reload()` 时先 `release` 旧纹理再加载新的；析构/换图都通过 `Global::image_source.release` 交还。

## 16. 错误处理

Failed 状态显示：错误图标 + 「图片加载失败」+ 具体原因（来自 `Global::ImageHandle::error`）+ [重试] [关闭]。
不支持的扩展名、宿主导入的加载器缺失、文件不存在 / 解码失败都会走到这里，不会崩溃。

## 17. 性能注意

`SetImagePath()` 只在路径变化时触发加载；渲染阶段只读已加载纹理；
缩放 / 平移只改数值（不重新解码、不重建纹理）；信息行只在布局/缩放变化时重新格式化。

## 18. 当前限制

* **JPG / JPEG 未实现解码**（后端只有 libpng）；GIF / WebP / BMP / SVG / TIFF 更不支持（不为它新加解码器）；
* **双指捏合缩放未实现**（输入层单指）；触屏用手势 + 按钮组合（拖动平移、按钮缩放）；
* **Loading 只占一帧**：`Backend::LoadTexture` 是同步解码；真正异步需要后端提供"后台解码 + UI 线程上传"，
  状态机已经按异步设计好，接上即可；
* 无旋转 / 翻转；无幻灯片 / 缩略图列表；
* 工具条自动淡出已实现但**默认关闭**（`SetToolbarAutoHide(true)` 打开），先保证基础浏览稳定。
