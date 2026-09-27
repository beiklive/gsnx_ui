# Image

来源：`component_view/components/Content.h`（实现 `component_view/components/Content.cpp`）
一句话：显示一张由宿主加载好的纹理，支持等比缩放 / 填满裁剪 / 拉伸 / 原始尺寸，纹理缺失时画占位框。

## 最小示例

```cpp
// 纹理由宿主（Page / App）通过后端加载好再交给它，组件本身不碰平台接口
gui_dev::TextureRef tex(ui.GetBackend(), "img/test.png");
Image* image = page.Root().Emplace<Image>();
image->setTexture(tex.ImGuiRef(), static_cast<float>(tex.Width()), static_cast<float>(tex.Height()));
image->setFit(Image::Fit::Contain);
image->max_size = ImVec2(0.0f, 190.0f);   // 限高：高度被限制住，宽度等比跟随
```

## API

### 构造与公开字段

| 成员 / 方法 | 说明 |
|---|---|
| `enum class Fit { Contain, Cover, Stretch, None }` | `Contain` 等比装进内容区（默认）/ `Cover` 等比填满并裁剪 / `Stretch` 拉伸不保比例 / `None` 原始尺寸居中 |
| `Image()` | 默认名 `"image"`，`tint = Theme::kWhite`，`radius = Global::component_style.corner_radius` |
| `ImTextureRef texture` | 纹理；无效（`ImTextureID_Invalid`）= 画占位 |
| `float source_width` / `float source_height` | 源图尺寸，>0 才参与缩放计算 |
| `Fit fit` | 默认 `Fit::Contain` |
| `ImVec4 tint` | 贴图着色，默认 `Theme::kWhite` |
| `bool tint_follows_theme` | 默认 true；`setTint` 会置 false |
| `float radius` | 贴图与占位框的圆角，默认取全局约定圆角 |
| `bool show_placeholder` | 无纹理时是否画占位框，默认 true |

### 链式设置

| 成员 / 方法 | 说明 |
|---|---|
| `Image& setTexture(ImTextureRef ref, float width, float height)` | 同时设 `texture` / `source_width` / `source_height` |
| `Image& setFit(Fit value)` | 设缩放模式 |
| `Image& setRadius(float value)` | 设圆角 |
| `Image& setTint(ImVec4 value)` | 设着色并关闭主题跟随 |

### 计算与扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `Rect FitRect(const Rect& content) const` | 按 `fit` 在内容区里算出的实际贴图矩形（`Stretch` 直接返回 `content`，`None` 以内容区中心放原始尺寸，`Contain`/`Cover` 按宽高比取内接 / 外接矩形） |
| `ImVec2 MeasureContent(const ImVec2& available) override`（protected） | 纹理无效时返回 `(Minf(Maxf(available.x, 1.0f), 240.0f), 160.0f)`；否则取 `min(可用宽/max_size.x, 可用高/max_size.y)` 缩放源尺寸，`Fit::None` 时按原始尺寸 |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override`（protected） | 纹理无效：`show_placeholder` 为真时画 `Theme::kBgWidget` 底色 + `Theme::kBorder` 描边 + `Material::ImagePlaceholder` 字形；否则 `AddImageRounded` 画贴图，超出内容区时 `PushClipRect` 裁掉 |
| `void OnThemeChanged() override`（protected） | `tint_follows_theme` 为 true 时重取 `Theme::kWhite`，并把 `radius` 重取为 `Global::component_style.corner_radius` |

Image 不声明自己的信号，事件接口继承自 [Widget](Widget.md)。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| 继承自 `Widget` 的 `clicked` / `pressed` / … | 无 | 默认不参与焦点；信号表见 [Widget.md](Widget.md) |

## 尺寸 / 主题约定

- 圆角默认取 `Global::component_style.corner_radius`（与 Box / Button 同一套），切主题时重新取一次。
- 着色默认 `Theme::kWhite`；占位框用 `Theme::kBgWidget`（透明度乘 0.6）与 `Theme::kBorder`，图标用 `Theme::kTextMuted`。
- `MeasureContent` 会把 `max_size` 一起纳入上限：上限 = `min(可用空间, max_size)`，按此等比缩放源尺寸。
- 占位框的尺寸是 `(min(可用宽度, 240), 160)`；`Fit::None` 时测量值就是 `source_width × source_height`。
- 贴图与占位框绘制都按 `EffectiveOpacity()` 乘透明度。

## 注意点

- 纹理对象必须活到控件销毁（宿主持有）；组件只保存 `ImTextureRef`，不负责加载与释放。
- `max_size` 不参与测量时，图片会一直撑到 `available`，而 `available` 又来自父级自适应高度 —— 会造成弹窗高度每帧变化（图片与按钮上下抖）。需要限高时显式设 `max_size`。
- `Cover` 与 `None` 模式下贴图会超出内容区，绘制时会临时裁剪，不会外溢到别的控件上；`Contain` 不会触发裁剪。
- 纹理无效（资源缺失）时不会崩，画占位框 + 图标；`show_placeholder = false` 则什么都不画。
