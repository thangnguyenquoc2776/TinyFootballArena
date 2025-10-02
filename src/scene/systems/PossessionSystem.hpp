#pragma once
#include "ecs/Ball.hpp"
#include "ecs/Player.hpp"

struct PossessionSystem {
    static void tryTakeAll(Ball& ball,
                           Player& p1, Player& p2, Player& gk1, Player& gk2,
                           float fieldW, float boxDepth, float& pickupCooldown);

private:
    static bool inKeeperBox(const Player* p, const Ball& ball, float fieldW, float boxDepth);
    static void tryTakeOne(Ball& ball, Player* p, float fieldW, float boxDepth, float& pickupCooldown);
};
