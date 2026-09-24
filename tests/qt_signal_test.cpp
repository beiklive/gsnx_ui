// Qt 风格信号槽的语义测试（不需要 SDL / ImGui，纯头文件即可跑）
//
//   ctest --test-dir build/mac        或直接 ./build/mac/gui_dev_signal_test
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "component_view/Object.h"

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        ++g_failures;
        std::fprintf(stderr, "[FAIL] %s\n", what);
    } else {
        std::fprintf(stderr, "[ ok ] %s\n", what);
    }
}

using gui_dev::cv::Connection;
using gui_dev::cv::connect;
using gui_dev::cv::Object;
using gui_dev::cv::Signal;

// 发送者：一个按钮
class Button : public Object {
public:
    void click() { emit clicked(); }
    void setChecked(bool value) {
        if (checked_ == value) {
            return;
        }
        checked_ = value;
        emit toggled(checked_);
    }
    bool isChecked() const { return checked_; }

signals:
    Signal<> clicked;
    Signal<bool> toggled;

private:
    bool checked_ = false;
};

// 接收者：一个页面（槽是普通成员函数，Qt 风格）
class Page : public Object {
public:
    void onConfirm() { ++confirm_count; }
    void onSwitched(int index) { last_index = index; }
    void onToggledNoArg() { ++toggle_count_no_arg; }
    void onToggled(bool value) {
        ++toggle_count;
        last_state = value;
    }

    int confirm_count = 0;
    int toggle_count = 0;
    int toggle_count_no_arg = 0;
    int last_index = -1;
    bool last_state = false;
};

} // namespace

int main() {
    // 1) 成员函数槽 + 无参信号
    {
        Button button;
        Page page;
        connect(&button, &Button::clicked, &page, &Page::onConfirm);
        button.click();
        button.click();
        Check(page.confirm_count == 2, "成员函数槽：clicked 触发两次");
    }

    // 2) 带参数信号 + 成员函数槽
    {
        Button button;
        Page page;
        connect(&button, &Button::toggled, &page, &Page::onToggled);
        button.setChecked(true);
        button.setChecked(false);
        Check(page.toggle_count == 2 && page.last_state == false, "带参数信号：toggled(bool) 透传");
        Check(!button.isChecked(), "按钮状态与信号一致");
    }

    // 3) 槽参数少于信号（Qt 允许）
    {
        Button button;
        Page page;
        connect(&button, &Button::toggled, &page, &Page::onToggledNoArg);
        button.setChecked(true);
        Check(page.toggle_count_no_arg == 1, "无参槽可以接带参信号");
    }

    // 4) context + lambda（Qt5 风格）
    {
        Button button;
        Page page;
        connect(&button, &Button::clicked, &page, [&page] { ++page.confirm_count; });
        button.click();
        Check(page.confirm_count == 1, "context + lambda 连接");
    }

    // 5) 手动断开
    {
        Button button;
        Page page;
        Connection handle = connect(&button, &Button::clicked, &page, &Page::onConfirm);
        Check(handle.connected(), "连接句柄初始为已连接");
        handle.disconnect();
        button.click();
        Check(page.confirm_count == 0 && !handle.connected(), "disconnect() 之后不再触发");
    }

    // 6) 接收者析构后自动断开（接收者生命周期）
    {
        Button button;
        Page* page = new Page();
        connect(&button, &Button::clicked, page, &Page::onConfirm);
        const int before = page->confirm_count;
        delete page; // 析构时必须把连接置为失效
        button.click();
        Check(before == 0, "接收者析构：连接自动失效，不会野指针调用");
    }

    // 7) 信号析构后不会留下悬空槽（发送者先死）
    {
        Page page;
        {
            Button button;
            connect(&button, &Button::toggled, &page, &Page::onToggled);
            button.setChecked(true);
        }
        Check(page.toggle_count == 1, "发送者先析构：此前已正常触发");
    }

    // 8) 同一信号多接收者 / 槽里再发射
    {
        Button button;
        Page a;
        Page b;
        connect(&button, &Button::clicked, &a, &Page::onConfirm);
        connect(&button, &Button::clicked, &b, &Page::onConfirm);
        connect(&button, &Button::clicked, &a, [&b] { ++b.confirm_count; });
        button.click();
        Check(a.confirm_count == 1 && b.confirm_count == 2, "一个信号可以接多个接收者");
    }

    std::fprintf(stderr, g_failures == 0 ? "全部通过\n" : "有 %d 项失败\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
