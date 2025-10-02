#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"

namespace PossessionSystem {
    void tryTakeAll(Ball& ball,
                    Player& p1, Player& p2, Player& gk1, Player& gk2,
                    float fieldW, float boxDepth,
                    float& pickupCooldown, float dt);

    // Xử lý riêng logic keeper (chụp bóng, giữ bóng, auto phất)
    void updateKeeperBallLogic(Ball& ball, Player& gk,
                               float& keeperHoldTimer, float dt);
}
