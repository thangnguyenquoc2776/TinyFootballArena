#include "scene/systems/PossessionSystem.hpp"
#include <algorithm>
#include <cmath>

static inline float clampf(float v, float lo, float hi){
    return (v < lo) ? lo : (v > hi ? hi : v);
}

static bool inKeeperBox(const Ball& ball, const Player* gk, float fieldW, float boxDepth){
    // giả định id: gk1=3 (bên trái), gk2=4 (bên phải)
    bool leftSide = (gk->id == 3);
    float minX = leftSide ? 0.0f : (fieldW - boxDepth);
    float maxX = leftSide ? boxDepth : fieldW;
    return (ball.tf.pos.x >= minX && ball.tf.pos.x <= maxX);
}

static void tryTakeOne(Ball& ball, Player* p, float fieldW, float boxDepth,
                       float pickupCooldown)
{
    if (ball.owner) return;

    // Không cho người vừa sút nhặt lại trong “justKicked”
    if (ball.justKicked > 0.0f && p->id == ball.lastKickerId) return;

    // cooldown chung tránh nhặt lại trong cùng nhịp
    if (pickupCooldown > 0.0f) return;

    Vec2 toBall = ball.tf.pos - p->tf.pos;
    float d = toBall.length(); if (d < 1e-4f) return;

    Vec2 fwd = p->facing.normalized();
    float cosA = Vec2::dot(toBall * (1.0f / d), fwd);

    bool isKeeper = (p->id == 3 || p->id == 4);
    if (isKeeper && !inKeeperBox(ball, p, fieldW, boxDepth)) return; // GK chỉ bắt trong box

    float captureRange = p->radius + ball.radius + (isKeeper ? 10.0f : 16.0f);
    float maxBallSpeed = isKeeper ? (3.5f * 40.0f) : (6.0f * 40.0f);

    if (cosA > std::cos(60.0f * 3.14159265f/180.0f) &&
        d < captureRange &&
        ball.tf.vel.length() < maxBallSpeed)
    {
        ball.owner = p;
    }
}

namespace PossessionSystem {

void tryTakeAll(Ball& ball,
                Player& p1, Player& p2, Player& gk1, Player& gk2,
                float fieldW, float boxDepth,
                float& pickupCooldown, float dt)
{
    // hạ dần timers
    pickupCooldown = std::max(0.0f, pickupCooldown - dt);
    ball.justKicked = std::max(0.0f, ball.justKicked - dt);

    tryTakeOne(ball, &p1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &p2, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk2, fieldW, boxDepth, pickupCooldown);
}

} // namespace PossessionSystem
