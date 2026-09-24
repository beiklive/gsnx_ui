// ============================================================================
// 课时 8 · 动画系统
// ============================================================================
//
// 【目标】
//   让动画在 30/60/120 FPS 下表现一致，并做出"弹性/回弹"这类高级手感。
//
// ---------------------------------------------------------------------------
// 一、唯一原则：动画吃 dt
// ---------------------------------------------------------------------------
//   错误：v += 0.1f;                     每个**帧**加固定量 -> 帧率越高越快
//   正确：v += (target - v) * (1 - exp(-speed * dt));   指数趋近，帧率无关
//   正确定时长：v = MoveTowards(v, target, duration, dt); 在 duration 秒内走完
//
//   为什么指数形式对：它解的是 dv/dt = speed*(target-v)，
//   换句话说"每秒缩小 (target-v) 的 speed 倍"，与一帧被调用几次无关。
//
// ---------------------------------------------------------------------------
// 二、两种进度的分工
// ---------------------------------------------------------------------------
//   SmoothTo（指数）：跟随类量。焦点强度、悬浮亮度、焦点框追位置。
//                     特点：没有明确"结束时刻"，越接近越慢（观感柔和）。
//   MoveTowards（定时长）：必须有明确时长的量。焦点切换 160ms、按压 100ms、
//                     入场 280ms。特点：时间可预期，配缓动函数用。
//   本项目两者都有，见 src/gamemenu/MenuAnimation.h。
//
// ---------------------------------------------------------------------------
// 三、缓动函数：给"线性进度"加性格
// ---------------------------------------------------------------------------
//   EaseOutCubic   起步快、收尾慢   —— 最常见的"进入"观感
//   EaseInOutCubic 两头慢、中间快   —— 页面切换
//   EaseOutBack    收尾小幅过冲     —— 焦点获得的"轻微弹性"
//   用法：先算线性进度 t（0..1），再 t2 = EaseOutBack(t)，用 t2 去插值几何量。
//
// ---------------------------------------------------------------------------
// 四、本课时的实验：同一目标，四种做法
// ---------------------------------------------------------------------------
//   A 车道：+= 0.1f            （错误示范）
//   B 车道：SmoothTo           （指数）
//   C 车道：MoveTowards 1.2s   （定时长线性）
//   D 车道：MoveTowards + EaseOutBack（定时长 + 弹性）
//   每个车道有自己的"模拟帧率"：车道 B 每 4 帧才更新一次（等于 15 FPS），
//   其它车道每帧更新（60 FPS）。按"开始"看结果：
//     · A 车道因为按帧计数 + 更新次数少 -> 明显落后（帧率一变速度就变）
//     · B/C/D 因为吃 dt -> 与 60 FPS 的车道同时到达
//   （为了让差异可见，B 车道用的是"累计 dt"而不是真实 dt 传递，见代码注释。）
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把 B 车道的更新间隔从 4 帧改成 12 帧（5 FPS），观察它是否仍然同时到达。
//   2. 把 C 车道的 duration 从 1.2s 改成 0.4s，再改成 3.0s。
//   3. 给 D 车道换成 EaseOutCubic，感受"有弹性"和"没弹性"的区别。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 为什么 += 0.1f 是错的？用一句话解释 dv/dt。
//   2. SmoothTo 与 MoveTowards 各自适合什么？
//   3. EaseOutBack 做了什么？为什么会"过冲"？
// ============================================================================

#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

float SmoothTo(float current, float target, float speed, float dt) {
    return current + (target - current) * (1.0f - std::exp(-speed * dt));
}

float MoveTowards(float current, float target, float duration, float dt) {
    if (duration <= 0.0f) {
        return target;
    }
    const float step = dt / duration;
    if (current < target) {
        return current + step > target ? target : current + step;
    }
    return current - step < target ? target : current - step;
}

float EaseOutCubic(float t) {
    const float x = 1.0f - (t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t));
    return 1.0f - x * x * x;
}

float EaseOutBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float x = (t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t)) - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

// 一条车道：自己记进度 + 自己的"模拟更新间隔"
struct Lane {
    const char* name;
    float value;
    bool naive;      // 是否用错误的 +=0.1f
    int skip;        // 每 N 帧才更新一次（模拟低帧率）
    float ease;      // 1 = 无缓动（用 EaseOutCubic），2 = EaseOutBack
};

} // namespace

class Lesson08AnimationScene final : public Scene {
public:
    const char* Name() const override { return "lesson08"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 8 · 动画系统",
                                    "吃 dt 的动画在任意帧率下表现一致")) {
            Components::EndPanel(ui);
            return;
        }

        const float dt = ui.DeltaTime();

        Section("一、唯一原则：动画吃 dt");
        Code("// 错：每帧加固定量 -> 帧率越高越快\n"
             "v += 0.1f;\n"
             "\n"
             "// 对：指数趋近（解 dv/dt = speed*(target-v)）\n"
             "v += (target - v) * (1.0f - exp(-speed * dt));\n"
             "\n"
             "// 对：定时长推进（有明确时长上限的进度）\n"
             "v = MoveTowards(v, target, duration, dt);");

        Section("二、实验：同一目标、四种做法、不同帧率");
        Note("B 车道每 %d 帧才更新（模拟低帧率），其它车道每帧更新。", skip_b_);
        if (ImGui::Button("开始 / 重来")) {
            running_ = true;
            elapsed_ = 0.0f;
            frame_index_ = 0;
            lanes_[0] = {"A  += 0.1f", 0.0f, true, 1, 0.0f};
            lanes_[1] = {"B  SmoothTo", 0.0f, false, skip_b_, 0.0f};
            lanes_[2] = {"C  MoveTowards 1.2s", 0.0f, false, 1, 1.0f};
            lanes_[3] = {"D  + EaseOutBack", 0.0f, false, 1, 2.0f};
        }
        ImGui::SameLine();
        ImGui::SliderInt("B 车道更新间隔（帧）", &skip_b_, 1, 12, "%d 帧");

        if (running_) {
            elapsed_ += dt;
            ++frame_index_;
            for (Lane& lane : lanes_) {
                if (lane.skip > 1 && (frame_index_ % lane.skip) != 0) {
                    continue; // 模拟低帧率：这几帧不更新
                }
                // 低帧率车道要用"累计的 dt"，否则 dt 会少算 -> 那是另一个错误示范
                const float step_dt = dt * static_cast<float>(lane.skip);
                if (lane.naive) {
                    lane.value += 0.1f; // 错误示范：按帧计数
                } else if (lane.ease == 0.0f) {
                    lane.value = SmoothTo(lane.value, 1.0f, 3.0f, step_dt);
                } else {
                    lane.value = MoveTowards(lane.value, 1.0f, 1.2f, step_dt);
                }
                // 弹性只影响绘制阶段（把线性进度过一遍 EaseOutBack），进度本身保持线性
            }
            // 全部到 1 后停表（A 车道很可能一直不到，所以只看 B/C/D）
            if (lanes_[1].value >= 0.999f && lanes_[2].value >= 0.999f && lanes_[3].value >= 0.999f) {
                running_ = false;
            }
        }

        // 绘制四条车道
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < 4; ++i) {
            const Lane& lane = lanes_[i];
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, 30.0f);
            ImGui::Dummy(size); // 占位（顺便复习课时 4：必须提交 item）
            const ImVec2 mx(mn.x + size.x, mn.y + size.y);

            // 进度显示值：D 车道套 EaseOutBack 看弹性
            float display = lane.value;
            if (lane.ease == 2.0f) {
                display = EaseOutBack(lane.value);
            } else if (lane.ease == 1.0f) {
                display = EaseOutCubic(lane.value);
            }
            const float x = mn.x + (size.x - 120.0f) * (display < 0.0f ? 0.0f : display);

            dl->AddRectFilled(mn, mx, IM_COL32(0x1A, 0x1D, 0x22, 0xFF), 5.0f);
            dl->AddCircleFilled(ImVec2(x + 14.0f, mn.y + size.y * 0.5f), 11.0f,
                                lane.naive ? IM_COL32(0x98, 0x98, 0xA1, 0xFF)
                                           : IM_COL32(0xE2, 0x1B, 0x25, 0xFF));
            dl->AddText(ImVec2(mn.x + 34.0f, mn.y + 7.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), lane.name);
            if (lane.skip > 1) {
                dl->AddText(ImVec2(mx.x - 96.0f, mn.y + 7.0f), IM_COL32(0x98, 0x98, 0xA1, 0xFF),
                            "低帧率车道");
            }
        }
        ImGui::TextDisabled("已跑 %.2fs（%d 帧）", static_cast<double>(elapsed_), frame_index_);
        Note("观察：B（低帧率）与 C/D 同时到达；A 因为按帧计数会明显落后。");

        Section("三、两种进度 + 缓动函数");
        Code("SmoothTo      跟随类：焦点强度、悬浮亮度、焦点框追位置（无明确终点）\n"
             "MoveTowards   定长类：焦点 160ms / 按压 100ms / 入场 280ms（时间可预期）\n"
             "\n"
             "EaseOutCubic   起步快收尾慢  <- 进入观感\n"
             "EaseInOutCubic 两头慢中间快  <- 页面切换\n"
             "EaseOutBack    收尾小幅过冲  <- 焦点获得的轻微弹性");
        Note("用法：先算线性进度 t，再 t2 = EaseOutBack(t)，用 t2 插值几何量。");

        Section("四、练习与验收");
        Bullet("练习：把 B 车道间隔调到 12 帧（5 FPS），确认它仍与 C/D 同时到达");
        Bullet("验收：为什么 +=0.1f 错？SmoothTo 与 MoveTowards 各适合什么？");

        EndLesson(ui);
    }

private:
    float elapsed_ = 0.0f;
    int frame_index_ = 0;
    int skip_b_ = 4;
    bool running_ = true;
    Lane lanes_[4] = {{"A  += 0.1f", 0.0f, true, 1, 0.0f},
                      {"B  SmoothTo", 0.0f, false, 4, 0.0f},
                      {"C  MoveTowards 1.2s", 0.0f, false, 1, 1.0f},
                      {"D  + EaseOutBack", 0.0f, false, 1, 2.0f}};
};

std::unique_ptr<Scene> CreateLesson08Animation() { return std::make_unique<Lesson08AnimationScene>(); }

} // namespace gui_dev::course
