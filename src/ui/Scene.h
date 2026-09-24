// 场景：一屏 UI 的最小单元。菜单栈、设置页、游戏库各是一个 Scene。
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace gui_dev {

class UiContext;

class Scene {
public:
    virtual ~Scene() = default;

    // 显示名，用于页头与调试。
    virtual const char* Name() const = 0;

    // 每帧绘制。不要在这里调用 ImGui::NewFrame/EndFrame，由 UiContext 负责。
    virtual void OnRender(UiContext& ui) = 0;

    // 可选的每帧逻辑（与绘制解耦，独立于帧率）。
    virtual void OnUpdate(UiContext& ui, float dt) { (void)ui; (void)dt; }

    // 返回 true 表示本场景要出栈 / 应用退出。
    virtual bool WantsClose() const { return false; }

    // 输入统一走这里，默认把动作交给 ImGui（方向键导航）。
    virtual void OnInput(UiContext& ui);
};

// 场景栈：只有栈顶场景接受绘制与输入；Push 后旧场景保留状态（例如设置页返回游戏库）。
class SceneStack {
public:
    void Push(std::unique_ptr<Scene> scene);
    // 弹出栈顶。栈空时返回 nullptr。
    std::unique_ptr<Scene> Pop();
    // 清空并压入新根场景。
    void Reset(std::unique_ptr<Scene> scene);

    bool Empty() const { return scenes_.empty(); }
    std::size_t Depth() const { return scenes_.size(); }
    Scene* Top();
    const Scene* Top() const;

    // 依次调用所有场景的 OnRender（下层场景只读展示，输入只给栈顶）。
    void RenderAll(UiContext& ui);
    void UpdateAll(UiContext& ui, float dt);
    void DispatchInput(UiContext& ui);

    // 处理 WantsClose 出栈；栈空返回 true（应用应退出）。
    bool ApplyClosures();

private:
    std::vector<std::unique_ptr<Scene>> scenes_;
};

} // namespace gui_dev
