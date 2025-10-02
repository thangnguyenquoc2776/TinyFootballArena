#include "scene/systems/PossessionSystem.hpp"
#include <algorithm>
#include <cmath>

static inline float clampf(float v, float lo, float hi){
    return (v < lo) ? lo : (v > hi ? hi : v);
}

static bool inKeeperBox(const Ball& ball, const Player* gk, float fieldW, float boxDepth){
    bool leftSide = (gk->id == 3);
    float minX = leftSide ? 0.0f : (fieldW - boxDepth);
    float maxX = leftSide ? boxDepth : fieldW;
    return (ball.tf.pos.x >= minX && ball.tf.pos.x <= maxX);
}

// Keeper bắt bóng nếu trong box + bóng chậm + facing đúng hướng
static void tryKeeperCatch(Ball& ball, Player* gk, float fieldW, float boxDepth) {
    if (ball.owner) return;
    if (!inKeeperBox(ball, gk, fieldW, boxDepth)) return;

    Vec2 toBall = ball.tf.pos - gk->tf.pos;
    float dist  = toBall.length();
    if (dist < 1e-4f) return;

    Vec2 fwd = gk->facing.normalized();
    float cosA = Vec2::dot(toBall * (1.0f / dist), fwd);

    float captureRange = gk->radius + ball.radius + 22.0f;
    float maxBallSpeed = 3.5f * 40.0f;

    if (cosA > std::cos(70.0f * 3.14159265f / 180.0f) &&
        dist < captureRange &&
        ball.tf.vel.length() < maxBallSpeed)
    {
        // GK bắt bóng thành công
        ball.owner = gk;
        ball.tf.vel = Vec2(0,0);
    }
}

// Cho cầu thủ thường tranh bóng
static void tryTakeOne(Ball& ball, Player* p, float fieldW, float boxDepth,
                       float pickupCooldown)
{
    if (ball.owner) return;
    if (ball.justKicked > 0.0f && p->id == ball.lastKickerId) return;
    if (pickupCooldown > 0.0f) return;

    bool isKeeper = (p->id == 3 || p->id == 4);
    if (isKeeper) {
        tryKeeperCatch(ball, p, fieldW, boxDepth);
        return;
    }

    Vec2 toBall = ball.tf.pos - p->tf.pos;
    float d = toBall.length(); if (d < 1e-4f) return;

    Vec2 fwd = p->facing.normalized();
    float cosA = Vec2::dot(toBall * (1.0f / d), fwd);

    float captureRange = p->radius + ball.radius + 16.0f;
    float maxBallSpeed = 6.0f * 40.0f;

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
    pickupCooldown = std::max(0.0f, pickupCooldown - dt);
    ball.justKicked = std::max(0.0f, ball.justKicked - dt);

    tryTakeOne(ball, &p1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &p2, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk2, fieldW, boxDepth, pickupCooldown);
}

// --- GK giữ bóng: bấm shoot = phất, sau 6s auto phất ---
void updateKeeperBallLogic(Ball& ball, Player& gk,
                           float& keeperHoldTimer, float dt)
{
    if (ball.owner != &gk) { keeperHoldTimer = 0; return; }

    keeperHoldTimer += dt;

    // Nếu GK bấm shoot hoặc quá 6s -> phất bóng
    if (gk.in.shoot || keeperHoldTimer > 6.0f) {
        Vec2 dir = gk.facing.normalized();
        if (dir.length() < 1e-6f) dir = Vec2(1,0);

        float kickSpeed = 18.0f * 40.0f;
        ball.owner = nullptr;
        ball.tf.vel = dir * kickSpeed;
        ball.tf.pos = gk.tf.pos + dir * (gk.radius + ball.radius + 4.0f);

        keeperHoldTimer = 0;
        ball.lastKickerId = gk.id;
        ball.justKicked = 0.4f;
    }
}

} // namespace PossessionSystem
