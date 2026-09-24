// Qt 风格的信号槽（最小可用实现，不需要 moc）。
//
// 用法与 Qt 一致：
//
//   class Button : public Widget {
//   signals:
//       Signal<bool> toggled;
//   public slots:
//       void setChecked(bool value);
//   };
//
//   // 发射（emit 是空宏，只做标记；等价于 signalObj(args)）
//   emit toggled(checked);
//
//   // 连接：成员函数 / 带 context 的 lambda / 纯 lambda
//   connect(button, &Button::clicked, this, &MyPage::OnConfirm);
//   connect(button, &Button::toggled, this, [this](bool on) { status_ = on; });
//
// 与 Qt 的差别（故意做小）：
//   * 没有 moc / 元对象：信号就是 `Signal<Args...>` 成员，`emit sig(args)` 展开成 `sig(args)`。
//   * 自动断开靠 `Object` 基类：接收者析构时，所有以它为 context 的连接立即失效
//     （对应 Qt 的「接收者生命周期」规则）。
//   * 槽参数可以少于信号；参数类型必须能隐式转换。
#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// Qt 里这三个是宏：signals 展开成 public，slots 展开成空，emit 展开成空。
// 只在包含本头文件的翻译单元里生效（framework/ 下的代码不包含它）。
#ifndef signals
#define signals public
#endif
#ifndef slots
#define slots
#endif
#ifndef emit
#define emit
#endif

namespace gui_dev::cv {

namespace detail {

// 连接句柄：接收者析构时置为失效，发射时跳过并顺手清理。
struct SlotState {
    bool alive = true;
};

// 每个 Object 持有的「以我为 context 的全部连接」，析构时统一失效。
struct ConnectionRegistry {
    std::vector<std::weak_ptr<SlotState>> states;
    void disconnectAll() {
        for (auto& weak : states) {
            if (auto state = weak.lock()) {
                state->alive = false;
            }
        }
        states.clear();
    }
};

} // namespace detail

// 连接句柄（对应 QMetaObject::Connection）
class Connection {
public:
    Connection() = default;
    explicit Connection(std::shared_ptr<detail::SlotState> state) : state_(std::move(state)) {}

    void disconnect() {
        if (auto state = state_.lock()) {
            state->alive = false;
        }
        state_.reset();
    }
    bool connected() const {
        const auto state = state_.lock();
        return state != nullptr && state->alive;
    }
    explicit operator bool() const { return connected(); }

private:
    std::weak_ptr<detail::SlotState> state_;
};

// 所有能当「接收者 / context」的类都继承它（等价于 QObject）。
class Object {
public:
    Object() = default;
    virtual ~Object() { registry_.disconnectAll(); }
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    // 断开以本对象为 context 的所有连接（一般不用手动调）
    void disconnectAll() { registry_.disconnectAll(); }

protected:
    template <typename... Args>
    friend class Signal;
    detail::ConnectionRegistry registry_;
};

// 信号：`Signal<Args...>` 成员；`emit sig(args)` 发射；`connect(...)` 连接。
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    ~Signal() = default;

    // ---- 连接到成员函数 ----------------------------------------------------
    template <typename Receiver, typename Return>
    Connection connect(Receiver* receiver, Return (Receiver::*method)(Args...)) {
        static_assert(std::is_base_of<Object, Receiver>::value,
                      "接收者要继承 cv::Object，才能在析构时自动断开");
        if (receiver == nullptr) {
            return Connection();
        }
        return Add(receiver, [receiver, method](Args... args) { (receiver->*method)(args...); });
    }

    // 槽参数少于信号（无参槽）
    template <typename Receiver, typename Return, std::size_t N = sizeof...(Args),
              typename = std::enable_if_t<(N > 0)>>
    Connection connect(Receiver* receiver, Return (Receiver::*method)()) {
        static_assert(std::is_base_of<Object, Receiver>::value,
                      "接收者要继承 cv::Object，才能在析构时自动断开");
        if (receiver == nullptr) {
            return Connection();
        }
        return Add(receiver, [receiver, method](Args...) { (receiver->*method)(); });
    }

    // ---- 带 context 的 lambda / 可调用对象（Qt5 风格） --------------------
    template <typename Context, typename Callable>
    Connection connect(Context* context, Callable callable) {
        static_assert(std::is_base_of<Object, Context>::value,
                      "context 要继承 cv::Object，才能在析构时自动断开");
        return Add(context, Slot(std::move(callable)));
    }

    // ---- 纯 lambda（生命周期自理） ----------------------------------------
    template <typename Callable, typename = std::enable_if_t<!std::is_base_of<Object, Callable>::value>>
    Connection connect(Callable callable) {
        return Add(nullptr, Slot(std::move(callable)));
    }

    // ---- 发射 --------------------------------------------------------------
    void emitSignal(Args... args) {
        ++emitting_;
        // 按索引遍历：槽里再 connect/disconnect 也不会把迭代器弄坏；
        // 每次取出 shared_ptr，保证调用期间这个槽不会被释放掉。
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            std::shared_ptr<Entry> entry = slots_[i];
            if (entry != nullptr && entry->state->alive && entry->invoke) {
                entry->invoke(args...);
            }
        }
        --emitting_;
        if (emitting_ == 0) {
            Prune();
        }
    }

    // `emit sig(args)` 实际调用的就是这个
    void operator()(Args... args) { emitSignal(std::forward<Args>(args)...); }

    void disconnectAll() {
        for (auto& entry : slots_) {
            if (entry != nullptr) {
                entry->state->alive = false;
            }
        }
        slots_.clear();
    }

    std::size_t count() const {
        std::size_t total = 0;
        for (const auto& entry : slots_) {
            if (entry != nullptr && entry->state->alive) {
                ++total;
            }
        }
        return total;
    }
    bool empty() const { return count() == 0; }

private:
    struct Entry {
        std::shared_ptr<detail::SlotState> state;
        Slot invoke;
    };

    template <typename Context>
    Connection Add(Context* context, Slot slot) {
        auto state = std::make_shared<detail::SlotState>();
        auto entry = std::make_shared<Entry>();
        entry->state = state;
        entry->invoke = std::move(slot);
        slots_.push_back(std::move(entry));
        if (context != nullptr) {
            context->registry_.states.push_back(state);
        }
        return Connection(state);
    }

    void Prune() {
        slots_.erase(std::remove_if(slots_.begin(), slots_.end(),
                                    [](const std::shared_ptr<Entry>& entry) {
                                        return entry == nullptr || !entry->state->alive;
                                    }),
                     slots_.end());
    }

    std::vector<std::shared_ptr<Entry>> slots_;
    int emitting_ = 0;
};

// ---- 自由函数 connect（Qt 的 QObject::connect 形态） ----------------------
// 说明：信号成员可能声明在基类（例如 &Button::clicked 实际是 Widget::clicked），
// 所以「信号所属类」单独推导，不能拿它去推导 sender（否则 Button* 推不出 Widget*）。

// connect(sender, &Sender::signal, receiver, &Receiver::slot)
template <typename Sender, typename SignalOwner, typename... Args, typename Receiver, typename Return>
Connection connect(Sender* sender, Signal<Args...> SignalOwner::* signal, Receiver* receiver,
                   Return (Receiver::*slot)(Args...)) {
    return sender != nullptr ? (sender->*signal).connect(receiver, slot) : Connection();
}

// 无参槽
template <typename Sender, typename SignalOwner, typename... Args, typename Receiver, typename Return,
          std::size_t N = sizeof...(Args), typename = std::enable_if_t<(N > 0)>>
Connection connect(Sender* sender, Signal<Args...> SignalOwner::* signal, Receiver* receiver,
                   Return (Receiver::*slot)()) {
    return sender != nullptr ? (sender->*signal).connect(receiver, slot) : Connection();
}

// connect(sender, &Sender::signal, context, lambda)
template <typename Sender, typename SignalOwner, typename... Args, typename Context, typename Callable>
Connection connect(Sender* sender, Signal<Args...> SignalOwner::* signal, Context* context, Callable callable) {
    return sender != nullptr ? (sender->*signal).connect(context, std::move(callable)) : Connection();
}

// connect(sender, &Sender::signal, lambda)
template <typename Sender, typename SignalOwner, typename... Args, typename Callable>
Connection connect(Sender* sender, Signal<Args...> SignalOwner::* signal, Callable callable) {
    return sender != nullptr ? (sender->*signal).connect(std::move(callable)) : Connection();
}

} // namespace gui_dev::cv
