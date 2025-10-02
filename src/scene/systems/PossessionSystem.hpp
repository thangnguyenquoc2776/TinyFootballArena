#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"

namespace PossessionSystem {
    void tryTakeAll(Ball& ball,
                    Player& p1, Player& p2, Player& gk1, Player& gk2,
                    float fieldW, float boxDepth,
                    float& pickupCooldown, float dt);

    // GK ôm bóng: giữ bóng trước tay, auto phất sau X giây, hoặc phất khi bấm shoot
    void updateKeeperBallLogic(Ball& ball, Player& gk, float& holdTimer, float dt);
}
