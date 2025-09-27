#include "ecs/Goalkeeper.hpp"
#include <cmath>

Goalkeeper::Goalkeeper() {
    facing = Vec2(-1.0f, 0.0f); // mặc định quay mặt vào sân
}

void Goalkeeper::updateAI(const Ball& ball, float fieldCenterY, float dt) {
    float targetY = ball.tf.pos.y;
    float dy = targetY - tf.pos.y;

    if (std::abs(dy) > 2.0f) {
        tf.vel.y = (dy > 0 ? 1 : -1) * vmax;
    } else {
        tf.vel.y = 0;
    }
}
