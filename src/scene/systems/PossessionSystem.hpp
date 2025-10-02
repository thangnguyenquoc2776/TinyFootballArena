#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"

namespace PossessionSystem {
    // pickupCooldown: cooldown chung tránh nhặt lại quá sớm (tham chiếu để giảm dần)
    void tryTakeAll(Ball& ball,
                    Player& p1, Player& p2, Player& gk1, Player& gk2,
                    float fieldW, float boxDepth,
                    float& pickupCooldown, float dt);
}
