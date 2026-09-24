#include "ui/Scene.h"

#include "ui/UiContext.h"

namespace gui_dev {

void Scene::OnInput(UiContext& ui) {
    // 默认实现：把抽象动作翻译成 ImGui 导航事件，这样任何 Scene 天然支持
    // 手柄/键盘选择，无需各自处理。需要自定义时覆写本方法。
    ImGuiIO& io = ImGui::GetIO();
    const PadState& pad = ui.Pad();

    if (pad.Pressed(InputAction::Up)) {
        io.AddKeyEvent(ImGuiKey_UpArrow, true);
        io.AddKeyEvent(ImGuiKey_UpArrow, false);
    }
    if (pad.Pressed(InputAction::Down)) {
        io.AddKeyEvent(ImGuiKey_DownArrow, true);
        io.AddKeyEvent(ImGuiKey_DownArrow, false);
    }
    if (pad.Pressed(InputAction::Left)) {
        io.AddKeyEvent(ImGuiKey_LeftArrow, true);
        io.AddKeyEvent(ImGuiKey_LeftArrow, false);
    }
    if (pad.Pressed(InputAction::Right)) {
        io.AddKeyEvent(ImGuiKey_RightArrow, true);
        io.AddKeyEvent(ImGuiKey_RightArrow, false);
    }
    if (pad.Pressed(InputAction::Confirm)) {
        io.AddKeyEvent(ImGuiKey_Enter, true);
        io.AddKeyEvent(ImGuiKey_Enter, false);
    }
    if (pad.Pressed(InputAction::Cancel)) {
        io.AddKeyEvent(ImGuiKey_Escape, true);
        io.AddKeyEvent(ImGuiKey_Escape, false);
    }
}

void SceneStack::Push(std::unique_ptr<Scene> scene) {
    if (scene == nullptr) {
        return;
    }
    scene->OnEnter(*ui_);
    scenes_.push_back(std::move(scene));
}

std::unique_ptr<Scene> SceneStack::Pop() {
    if (scenes_.empty()) {
        return nullptr;
    }
    scenes_.back()->OnLeave(*ui_);
    std::unique_ptr<Scene> top = std::move(scenes_.back());
    scenes_.pop_back();
    return top;
}

void SceneStack::Reset(std::unique_ptr<Scene> scene) {
    Clear();
    Push(std::move(scene));
}

void SceneStack::Clear() {
    // 逆序 OnLeave，再统一析构（与"越晚创建越早销毁"一致）
    for (auto it = scenes_.rbegin(); it != scenes_.rend(); ++it) {
        (*it)->OnLeave(*ui_);
    }
    scenes_.clear();
}

Scene* SceneStack::Top() { return scenes_.empty() ? nullptr : scenes_.back().get(); }

const Scene* SceneStack::Top() const { return scenes_.empty() ? nullptr : scenes_.back().get(); }

void SceneStack::RenderAll(UiContext& ui) {
    for (auto& scene : scenes_) {
        scene->OnRender(ui);
    }
}

void SceneStack::UpdateAll(UiContext& ui, float dt) {
    for (auto& scene : scenes_) {
        scene->OnUpdate(ui, dt);
    }
}

void SceneStack::DispatchInput(UiContext& ui) {
    if (Scene* top = Top()) {
        top->OnInput(ui);
    }
}

bool SceneStack::ApplyClosures() {
    while (!scenes_.empty() && scenes_.back()->WantsClose()) {
        scenes_.pop_back();
    }
    return scenes_.empty();
}

} // namespace gui_dev
