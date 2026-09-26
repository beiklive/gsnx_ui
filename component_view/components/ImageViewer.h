// ImageViewer：通用图片浏览器 Widget（手柄 / 触屏 / 鼠标共用一套状态）。
//
// 设计要点（对应需求文档）：
//   * 只负责「显示」：传入图片路径 → Empty / Loading / Loaded / Failed 四种状态；
//   * 一套状态、一套交互：手柄、触屏、鼠标最终都调同一组方法
//     （SetZoom / ZoomIn / ZoomOut / FitToWindow / ActualSize / ResetView / PanBy / Close）；
//   * 复用现有体系：Widget 基类、Button、Box、Label、ProgressBar、Draw、Theme、
//     Global::component_style、FocusManager（Toolbar 就是普通 Button）、
//     PopupManager（作为 Popup 内容使用）、Texture（通过 Global::image_source 由宿主注入）；
//   * 不新增视觉语言：圆角 / 边框 / 阴影 / 内边距 / 字号全部取 Theme 与 Global::component_style；
//   * 不新增解码器：图片怎么解码由宿主的 image_source 决定（当前后端只有 libpng → PNG）。
//
// 交互映射（都走现有 Action，无平台按键硬编码）：
//   手柄：方向键/摇杆 = 平移（图片超出可视区时），L / R = 缩小 / 放大，
//         X = 重置，Y = 适应屏幕，A = 适应屏幕/100% 切换，B = 关闭（由 Popup 或 closeRequested）
//   触屏：单指拖动 = 平移，点按钮 = 缩放/重置/适应/关闭（双指缩放需要输入层支持多点触控，见文档）
//   鼠标：左键拖动 = 平移，滚轮 = 缩放，点按钮 = 操作，Esc/B = 关闭
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Box;
class Button;
class Label;
class ProgressBar;

class ImageViewer : public Widget {
public:
    // 图片状态
    enum class State {
        Empty,   // 没有设置路径
        Loading, // 正在加载（宿主 image_source 是同步解码，这个状态占一帧）
        Loaded,  // 加载成功
        Failed,  // 加载失败（errorText() 有原因）
    };

    using CloseCallback = std::function<void()>;

    // 已加载的图片数据（纹理 + 原始尺寸）
    struct ImageData {
        ImTextureRef texture{};
        int width = 0;
        int height = 0;
    };

    ImageViewer();
    explicit ImageViewer(std::string image_path);

    // ---- 内容 / 状态 ------------------------------------------------------
    ImageViewer& SetImagePath(std::string path); // 路径变化才重新加载；空串 = Empty
    const std::string& imagePath() const { return path_; }
    ImageViewer& Reload();                       // 重试（Failed）或强制重新加载
    State state() const { return state_; }
    const std::string& errorText() const { return error_text_; }
    bool IsLoaded() const { return state_ == State::Loaded; }
    int imageWidth() const { return image_.width; }
    int imageHeight() const { return image_.height; }
    const std::string& fileName() const { return file_name_; }

    // ---- 视图（Gamepad / Touch / Mouse 统一入口）--------------------------
    ImageViewer& SetZoom(float scale);           // 绝对缩放，1.0 = 100%
    ImageViewer& SetZoomAt(float scale, const ImVec2& focal); // focal 相对视口中心（滚轮/双指用）
    ImageViewer& ZoomIn();
    ImageViewer& ZoomOut();
    ImageViewer& FitToWindow();
    ImageViewer& ActualSize();
    ImageViewer& ResetView();                    // 100% + 居中
    ImageViewer& PanBy(const ImVec2& delta);     // 屏幕像素增量
    ImageViewer& SetZoomSteps(std::vector<float> steps);
    float zoom() const { return scale_; }
    float fitZoom() const { return fit_scale_; }
    ImVec2 pan() const { return pan_; }
    bool IsFit() const { return fit_mode_; }

    // ---- 交互 / 外观配置 --------------------------------------------------
    ImageViewer& SetCloseCallback(CloseCallback callback);
    ImageViewer& SetToolbarVisible(bool visible);
    ImageViewer& SetToolbarAutoHide(bool enabled, float idle_seconds = 4.0f);
    ImageViewer& SetShowInfo(bool visible);
    ImageViewer& SetStateIndicators(bool visible); // Loading 状态条
    ImageViewer& SetSupportedExtensions(std::vector<std::string> extensions); // 默认 png/jpg/jpeg
    bool ToolbarVisible() const;

signals:
    Signal<> closeRequested;    // 关闭请求（Popup 里由 PopupManager 接；独立用时宿主自己接）
    Signal<State> stateChanged; // 状态变化
    Signal<float> zoomChanged;  // 缩放变化

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override;

private:
    void BuildUi();
    void ApplyLabels();
    void RequestLoad();       // 标记需要重新加载（不在这里加载）
    void LoadIfPending();     // OnUpdate 里执行；绝不在渲染阶段加载
    void Unload();
    void LayoutChildren(const ImVec2& inner); // 在 MeasureContent 里算好子控件位置（同一帧生效）
    void RecomputeFit(const ImVec2& viewport);
    void ClampPan();
    Rect ImageRect(const Rect& viewport) const; // 图片绘制矩形（含缩放与平移）
    void RevealToolbar();
    void SetState(State next, std::string error = {});

    // 状态
    ImageData image_;
    std::string path_;
    std::string file_name_;
    std::string error_text_;
    State state_ = State::Empty;
    bool load_pending_ = false;
    bool loaded_once_ = false;

    // 视图
    std::vector<float> zoom_steps_{0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 3.0f, 4.0f};
    float scale_ = 1.0f;
    float fit_scale_ = 1.0f;
    bool fit_mode_ = true;
    ImVec2 pan_{0.0f, 0.0f};
    ImVec2 viewport_size_{1.0f, 1.0f};
    ImVec2 canvas_size_{1.0f, 1.0f};

    // 指针拖动
    bool pointer_dragging_ = false;
    ImVec2 drag_origin_{0.0f, 0.0f};

    // 子控件（Toolbar 全是普通 Button，视觉与交互都复用 Button）
    Box* canvas_ = nullptr;
    Label* info_ = nullptr;
    Label* status_ = nullptr;       // Empty / Loading / Failed 的主文案
    Label* status_detail_ = nullptr;
    ProgressBar* status_bar_ = nullptr;
    Box* error_actions_ = nullptr;  // Failed 时的 [重试] [关闭]
    Button* retry_button_ = nullptr;
    Button* error_close_button_ = nullptr;
    Box* toolbar_ = nullptr;
    Button* fit_button_ = nullptr;
    Button* actual_button_ = nullptr;
    Button* zoom_out_button_ = nullptr;
    Button* zoom_in_button_ = nullptr;
    Button* reset_button_ = nullptr;
    Button* close_button_ = nullptr;

    // 配置
    CloseCallback close_callback_;
    std::vector<std::string> supported_extensions_{".png", ".jpg", ".jpeg"};
    bool toolbar_visible_ = true;
    bool toolbar_autohide_ = false;
    float toolbar_idle_ = 4.0f;
    float toolbar_opacity_ = 1.0f;
    float last_interaction_ = 0.0f;
    bool show_info_ = true;
    bool show_state_indicators_ = true;
};

} // namespace gui_dev::cv
