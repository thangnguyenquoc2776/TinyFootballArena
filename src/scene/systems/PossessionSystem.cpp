#include "scene/systems/PossessionSystem.hpp"
#include <algorithm>
#include <cmath>

static inline float clampf(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}

static bool inKeeperBox(const Ball& ball, const Player* gk, float fieldW, float boxDepth){
    bool leftSide = (gk->id == 3);
    float minX = leftSide ? 0.0f : (fieldW - boxDepth);
    float maxX = leftSide ? boxDepth : fieldW;
    return (ball.tf.pos.x >= minX && ball.tf.pos.x <= maxX);
}

static void tryTakeOne(Ball& ball, Player* p, float fieldW, float boxDepth, float pickupCooldown){
    if (ball.owner) return;

    if (ball.justKicked > 0.0f && p->id == ball.lastKickerId) return;
    if (pickupCooldown > 0.0f) return;

    Vec2 toBall = ball.tf.pos - p->tf.pos;
    float d = toBall.length(); if (d < 1e-4f) return;

    Vec2 fwd = p->facing.normalized();
    float cosA = Vec2::dot(toBall * (1.0f/d), fwd);

    bool isKeeper = p->isGoalkeeper;
    if (isKeeper && !inKeeperBox(ball, p, fieldW, boxDepth)) return;

    float captureRange = p->radius + ball.radius + (isKeeper ? 10.0f : 16.0f);
    float maxBallSpeed = isKeeper ? (3.5f * 40.0f) : (6.0f * 40.0f);

    if (cosA > std::cos(60.0f * 3.14159265f/180.0f) &&
        d < captureRange &&
        ball.tf.vel.length() < maxBallSpeed)
    {
        ball.owner = p; // “ôm bóng” (GK) hay “dắt bóng” (cầu thủ) đều là owner
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

void updateKeeperBallLogic(Ball& ball, Player& gk, float& holdTimer, float dt)
{
    if (!gk.isGoalkeeper) return;

    // chỉ khi GK đang giữ bóng
    if (ball.owner == &gk) {
        holdTimer += dt;

        // giữ bóng “trước tay”
        Vec2 aim = gk.facing.normalized(); if (aim.length()<1e-6f) aim=Vec2(1,0);
        float holdDist = gk.radius + ball.radius + 2.0f;
        ball.tf.pos = gk.tf.pos + aim * holdDist;
        ball.tf.vel = Vec2(0,0);

        // phất khi bấm shoot hoặc quá thời gian
        const float AUTO_TIME = 6.0f;
        if (gk.in.shoot || holdTimer > AUTO_TIME) {
            holdTimer = 0.0f;
            ball.owner = nullptr;
            float kick = 18.0f * 40.0f; // ~18 m/s
            ball.tf.vel = aim * kick + gk.tf.vel * 0.3f;
            ball.lastKickerId = gk.id;
            ball.justKicked   = 0.30f;
        }
    } else {
        holdTimer = 0.0f;
    }
}

} // namespace
