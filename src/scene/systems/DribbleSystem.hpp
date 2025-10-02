#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>

class DribbleSystem {
public:
    struct Params {
        // Smooth follow
        float smoothTimeMove = 0.12f;     // mượt khi đang chạy
        float smoothTimeStop = 0.18f;     // mượt hơn khi dừng (trôi nhẹ rồi dừng)
        float turnRate       = 1.8f;      // rad/s, giới hạn tốc độ xoay hướng vận tốc
        float extraLead      = 9.0f;      // px: khoảng cách bổ sung trước mũi chân (cơ bản)
        float leadSpeedK     = 0.020f;    // px mỗi (px/s): chạy nhanh → lead tăng
        float lateralBias    = 3.5f;      // px: lệch bóng sang bên cho tự nhiên
        float targetDeadRad  = 4.0f;      // px: gần mục tiêu thì snap để diệt jitter

        float maxSpeed       = 5.0f * 40.0f;
        float minSpeed       = 1.0f * 40.0f;
        float carryFactor    = 0.30f;     // % tốc độ người cộng vào bóng
        float loseDistance   = 44.0f;     // px: xa người quá thì mất bóng
        float idleDamping    = 11.0f;     // dập vận tốc khi đứng yên (cao → tắt nhanh)
    };

    DribbleSystem() : P{} {}
    explicit DribbleSystem(const Params& p) : P(p) {}

    void reset() { filtVel.clear(); }
    void setParams(const Params& p) { P = p; }

    // Chỉ gọi khi ball.owner == &carrier
    void update(Ball& ball, Player& carrier, float dt);

private:
    Params P;

    // vận tốc nội bộ của bộ lọc SmoothDamp cho từng player
    std::unordered_map<int, Vec2> filtVel;

    static inline float clampf(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi ? hi : v);
    }
    static Vec2 rotateTowards(const Vec2& a, const Vec2& b, float maxRad);

    // SmoothDamp 2D (phiên bản ổn định số)
    static Vec2 smoothDamp(const Vec2& current, const Vec2& target,
                           Vec2& currentVel, float smoothTime, float dt);
};
