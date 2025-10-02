#include "scene/systems/PossessionSystem.hpp"
#include <cmath>
#include <algorithm>

static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi ? hi : v);
}

bool PossessionSystem::inKeeperBox(const Player* p, const Ball& ball, float fieldW, float boxDepth){
    bool isGK1 = (p->id == 3);
    float minX = isGK1 ? 0.0f : (fieldW - boxDepth);
    float maxX = isGK1 ? boxDepth : fieldW;
    return (ball.tf.pos.x >= minX && ball.tf.pos.x <= maxX);
}

void PossessionSystem::tryTakeOne(Ball& ball, Player* p, float fieldW, float boxDepth, float& pickupCooldown){
    if (ball.owner || pickupCooldown > 0.0f) return;

    Vec2 toBall = ball.tf.pos - p->tf.pos;
    float d = toBall.length(); if (d < 1e-3f) return;
    Vec2 fwd = p->facing.normalized(); if (fwd.length()<1e-6f) fwd = Vec2(1,0);
    float cosA = Vec2::dot(toBall * (1.0f / d), fwd);

    bool isKeeper = (p->id==3 || p->id==4);
    if (isKeeper && !inKeeperBox(p, ball, fieldW, boxDepth)) return;

    float captureRange = p->radius + ball.radius + (isKeeper ? 10.0f : 16.0f);
    float maxBallSpeed = isKeeper ? (3.0f * 40.0f) : (6.0f * 40.0f);

    if (cosA > std::cos(60.0f * 3.14159265f/180.0f) && d < captureRange &&
        ball.tf.vel.length() < maxBallSpeed) {
        ball.owner = p;
    }
}

void PossessionSystem::tryTakeAll(Ball& ball,
                                  Player& p1, Player& p2, Player& gk1, Player& gk2,
                                  float fieldW, float boxDepth, float& pickupCooldown)
{
    tryTakeOne(ball, &p1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &p2, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk1, fieldW, boxDepth, pickupCooldown);
    tryTakeOne(ball, &gk2, fieldW, boxDepth, pickupCooldown);
}
