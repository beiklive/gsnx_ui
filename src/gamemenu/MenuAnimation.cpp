#include "gamemenu/MenuAnimation.h"

#include <cmath>

namespace gui_dev::gamemenu {

float SmoothTo(float current, float target, float speed, float dt) {
    if (speed <= 0.0f || dt <= 0.0f) {
        return current;
    }
    const float t = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * t;
}

float MoveTowards(float current, float target, float duration, float dt) {
    if (duration <= 0.0f) {
        return target;
    }
    const float step = dt / duration;
    if (current < target) {
        current += step;
        return current > target ? target : current;
    }
    current -= step;
    return current < target ? target : current;
}

float DecayOnce(float current, float duration, float dt) {
    if (current <= 0.0f) {
        return 0.0f;
    }
    return MoveTowards(current, 0.0f, duration, dt);
}

float AdvanceOnce(float current, float duration, float dt) {
    if (current >= 1.0f) {
        return 1.0f;
    }
    return MoveTowards(current, 1.0f, duration, dt);
}

float EaseOutCubic(float t) {
    const float x = 1.0f - Clamp01(t);
    return 1.0f - x * x * x;
}

float EaseOutQuad(float t) {
    const float x = 1.0f - Clamp01(t);
    return 1.0f - x * x;
}

float EaseInOutCubic(float t) {
    const float x = Clamp01(t);
    if (x < 0.5f) {
        return 4.0f * x * x * x;
    }
    const float y = -2.0f * x + 2.0f;
    return 1.0f - y * y * y * 0.5f;
}

float EaseOutBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float x = Clamp01(t) - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

} // namespace gui_dev::gamemenu
