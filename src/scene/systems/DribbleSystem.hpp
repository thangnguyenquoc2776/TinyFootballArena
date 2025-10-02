#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>

class DribbleSystem {
public:
    struct Params {
        float tps            = 7.0f;          // touches per second
        float touchSpeed     = 4.6f * 40.0f;  // px/s sau mỗi lần chạm
        float carryFactor    = 0.30f;         // % vận tốc người cộng vào bóng
        float turnRate       = 3.2f;          // rad/s đổi hướng giữa 2 lần chạm
        float maxSpeed       = 5.2f * 40.0f;
        float minSpeed       = 1.2f * 40.0f;
        float extraLead      = 6.0f;          // khoảng cách thêm trước mũi chân
        float loseDistance   = 42.0f;         // văng xa thì mất bóng
    };

    DribbleSystem() : P{} {}                  // ✅ mặc định
    explicit DribbleSystem(const Params& p)   // ✅ nhận params
        : P(p) {}

    void reset() { timers.clear(); }
    void setParams(const Params& p) { P = p; }

    // Chỉ gọi khi ball.owner == &carrier
    void update(Ball& ball, Player& carrier, float dt);

private:
    Params P;
    std::unordered_map<int, float> timers; // player.id -> touchTimer

    static inline float clampf(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi ? hi : v);
    }
    static Vec2 rotateTowards(const Vec2& a, const Vec2& b, float maxRad);
};
