#include "component_view/popup/PopupManager.h"

#include <utility>

#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 弹窗名字唯一化：同名时加序号（Find/Close 用名字定位）
std::string UniqueName(const std::vector<std::unique_ptr<Popup>>& popups, const std::string& base) {
    auto taken = [&popups](const std::string& candidate) {
        for (const auto& popup : popups) {
            if (popup->name() == candidate) {
                return true;
            }
        }
        return false;
    };
    if (!taken(base)) {
        return base;
    }
    for (int index = 2; index < 1000; ++index) {
        const std::string candidate = base + "_" + std::to_string(index);
        if (!taken(candidate)) {
            return candidate;
        }
    }
    return base;
}

} // namespace

PopupManager::PopupManager(FocusManager* focus) : focus_(focus) {}

PopupManager::~PopupManager() = default;

// =============================================================== 展示 =====

Popup* PopupManager::Show(std::unique_ptr<Popup> popup) {
    if (popup == nullptr) {
        return nullptr;
    }
    Popup* raw = popup.get();
    // 名字要唯一：作用域、Find/Close 都按名字定位
    raw->setName(UniqueName(popups_, raw->name()));
    // 第一个弹窗打开时记住背景焦点：整栈关完后要还回去（需求 §9）
    if (popups_.empty()) {
        background_focus_ = Global::focused;
    }
    raw->SetLayerZ(PopupLayerZ(static_cast<int>(popups_.size())));
    popups_.push_back(std::move(popup));

    PushScopeFor(*raw);
    raw->Open();
    FocusPopup(*raw);
    return raw;
}

void PopupManager::PushScopeFor(Popup& popup) {
    if (focus_ == nullptr) {
        return;
    }
    focus_->PushScope("popup:" + popup.name(), &popup.Root());
}

void PopupManager::FocusPopup(Popup& popup) {
    // 弹窗打开后焦点从背景 UI 转移到弹窗内部（需求 §9）
    if (Widget* target = popup.DefaultFocusTarget()) {
        Global::SetFocus(target);
    }
    // 方向键重复状态清一下：避免把「打开弹窗那一按」当成连按
    Global::nav_repeat.Reset();
}

void PopupManager::RebuildScopes() {
    if (focus_ == nullptr) {
        return;
    }
    // 非栈顶弹窗被关掉时（少见路径）：直接按当前存活弹窗重建作用域栈，保证一致
    focus_->Reset();
    for (std::size_t i = 0; i < popups_.size(); ++i) {
        FocusManager::Scope scope;
        scope.name = "popup:" + popups_[i]->name();
        scope.root = &popups_[i]->Root();
        scope.saved_focus = (i == 0) ? background_focus_ : nullptr;
        focus_->PushScope(scope.name, scope.root);
    }
    if (Popup* top = Top()) {
        if (Widget* target = top->DefaultFocusTarget()) {
            Global::SetFocus(target);
        }
    }
}

// =========================================================== 常用形态 =====

Popup* PopupManager::ShowInfo(std::string title, std::string message, std::string button_text) {
    auto popup = std::make_unique<Popup>("info", PopupKind::Info);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setText(std::move(message));
    popup->setDismissOnBackdrop(true); // 纯信息提示：点遮罩关掉不算误操作
    popup->addButton(std::move(button_text));
    return Show(std::move(popup));
}

Popup* PopupManager::ShowConfirm(std::string title, std::string message, std::function<void()> on_confirm,
                                 std::string confirm_text, std::string cancel_text) {
    auto popup = std::make_unique<Popup>("confirm", PopupKind::Confirm);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setText(std::move(message));
    popup->setDismissOnBackdrop(false); // 关键操作：不允许点遮罩关掉（需求 §24）
    popup->setDismissOnCancel(true);
    Popup::ButtonSpec cancel;
    cancel.text = std::move(cancel_text);
    popup->addButton(std::move(cancel));
    Popup::ButtonSpec confirm;
    confirm.text = std::move(confirm_text);
    confirm.primary = true;
    confirm.on_click = std::move(on_confirm);
    popup->addButton(std::move(confirm));
    popup->setDefaultFocus(0); // 默认落在「取消」上，避免误操作（需求 §19，可再改）
    return Show(std::move(popup));
}

Popup* PopupManager::ShowSelection(std::string title, std::string message, std::vector<Popup::ButtonSpec> options,
                                   PopupButtonLayout layout) {
    auto popup = std::make_unique<Popup>("selection", PopupKind::Selection);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    if (!message.empty()) {
        popup->setText(std::move(message));
    }
    popup->setButtonLayout(layout);
    for (Popup::ButtonSpec& spec : options) {
        popup->addButton(std::move(spec));
    }
    popup->setDismissOnBackdrop(true);
    popup->setDefaultFocus(0);
    return Show(std::move(popup));
}

Popup* PopupManager::ShowProgress(std::string title, std::string message, bool indeterminate) {
    auto popup = std::make_unique<Popup>("progress", PopupKind::Progress);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setProgressContent(std::move(message), indeterminate);
    // 进度弹窗默认没有按钮：生命周期由任务控制（Open/Close），不靠用户点关闭（需求 §17）
    popup->setDismissOnBackdrop(false);
    popup->setDismissOnCancel(false);
    return Show(std::move(popup));
}

Popup* PopupManager::ShowRichText(std::string title, std::vector<RichText::Run> runs, float view_height, PopupKind kind) {
    auto popup = std::make_unique<Popup>("richtext", kind);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setRichText(std::move(runs), view_height);
    popup->addButton("关闭");
    return Show(std::move(popup));
}

Popup* PopupManager::ShowImage(std::string title, ImTextureRef texture, float width, float height) {
    auto popup = std::make_unique<Popup>("image", PopupKind::Info);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setImage(texture, width, height);
    popup->addButton("关闭");
    return Show(std::move(popup));
}

Popup* PopupManager::ShowCustom(std::string title, std::function<void(Widget& content)> builder, PopupKind kind) {
    auto popup = std::make_unique<Popup>("custom", kind);
    popup->setStyle(defaults_);
    popup->setTitle(std::move(title));
    popup->setContentBuilder(std::move(builder));
    popup->addButton("关闭");
    return Show(std::move(popup));
}

// ============================================================== 每帧 =====

void PopupManager::Layout() {
    for (auto& popup : popups_) {
        popup->Layout();
    }
}

Widget* PopupManager::HitTest(const ImVec2& point) {
    for (std::size_t i = popups_.size(); i > 0; --i) {
        Popup* popup = popups_[i - 1].get();
        if (Widget* hit = popup->HitTest(point)) {
            return hit;
        }
        // 模态弹窗：这一点没落在弹窗内容上也算被它拦住（背景不接受 hover/点击）
        if (popup->IsModal()) {
            return nullptr;
        }
    }
    return nullptr;
}

bool PopupManager::BlocksBackground() const {
    for (const auto& popup : popups_) {
        if (popup->IsModal() && !popup->IsClosed() && !popup->IsClosing()) {
            return true;
        }
    }
    return false;
}

bool PopupManager::AnyModal() const {
    return BlocksBackground();
}

void PopupManager::CollectFocusables(const std::vector<Widget*>& fallback, std::vector<Widget*>& out) const {
    if (focus_ != nullptr) {
        focus_->CollectFocusables(fallback, out);
        return;
    }
    out = fallback;
}

void PopupManager::UpdateTree(float dt) {
    // 输入优先级：只有最上层弹窗收输入，下面的弹窗完全静默（渲染仍在，见 Draw）
    if (Popup* top = Top()) {
        top->UpdateTree(dt);
    }
}

void PopupManager::HandleDismiss() {
    Popup* top = Top();
    if (top == nullptr || !top->IsOpen()) {
        return;
    }
    // B / Esc：控件没有消费掉就当“返回”处理（需求 §31）
    if (top->dismissOnCancel() && Global::pad.Pressed(InputAction::Cancel) &&
        Global::Available(InputAction::Cancel)) {
        Global::MarkConsumed(InputAction::Cancel);
        top->RequestDismiss();
    }
    // 遮罩点击（dismiss_on_backdrop 在自己的信号里已经置了标记）
    if (top->WantsDismiss()) {
        top->ClearDismissRequest();
        top->Close();
    }
}

void PopupManager::Advance(float dt) {
    for (auto& popup : popups_) {
        popup->Update(dt);
    }
    RemoveClosed();
}

void PopupManager::RemoveClosed() {
    bool removed = false;
    for (std::size_t i = 0; i < popups_.size();) {
        if (popups_[i]->IsClosed()) {
            const bool was_top = (i + 1 == popups_.size());
            const std::string name = popups_[i]->name();
            popups_.erase(popups_.begin() + static_cast<std::ptrdiff_t>(i));
            removed = true;
            if (!was_top) {
                // 关掉的不是栈顶：作用域栈直接按现存弹窗重建
                RebuildScopes();
            } else if (focus_ != nullptr && !focus_->PopScopeByName("popup:" + name)) {
                focus_->PopScope();
            }
            continue;
        }
        ++i;
    }
    if (removed && popups_.empty() && focus_ != nullptr && focus_->Empty()) {
        background_focus_ = nullptr;
    }
    // 层级重新编号（每个弹窗的 layer 跟随栈序）
    for (std::size_t i = 0; i < popups_.size(); ++i) {
        popups_[i]->SetLayerZ(PopupLayerZ(static_cast<int>(i)));
    }
}

void PopupManager::Draw(ImDrawList* dl) {
    for (auto& popup : popups_) {
        popup->Draw(dl);
    }
}

bool PopupManager::EnsureVisible(Widget* target) {
    if (target == nullptr) {
        return false;
    }
    for (std::size_t i = popups_.size(); i > 0; --i) {
        if (popups_[i - 1]->EnsureVisible(target)) {
            return true;
        }
    }
    return false;
}

// ========================================================= 查询 / 操作 ====

Popup* PopupManager::Top() const {
    if (popups_.empty()) {
        return nullptr;
    }
    return popups_.back().get();
}

Popup* PopupManager::Find(const std::string& name) const {
    for (const auto& popup : popups_) {
        if (popup->name() == name) {
            return popup.get();
        }
    }
    return nullptr;
}

void PopupManager::CloseTop() {
    if (Popup* top = Top()) {
        top->Close();
    }
}

void PopupManager::Close(const std::string& name) {
    for (auto& popup : popups_) {
        if (popup->name() == name) {
            popup->Close();
            return;
        }
    }
}

void PopupManager::CloseAll() {
    for (auto& popup : popups_) {
        popup->Close();
    }
}

void PopupManager::ClosePageScoped() {
    for (auto& popup : popups_) {
        if (popup->scope() == PopupScope::Page) {
            popup->Close();
        }
    }
}

void PopupManager::RefreshTheme() {
    for (auto& popup : popups_) {
        popup->RefreshTheme();
    }
}

} // namespace gui_dev::cv
