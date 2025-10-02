#pragma once
#include "ecs/Entity.hpp"

class Ball;

struct InputIntent {
    float x = 0.0f;
    float y = 0.0f;
    bool shoot = false;
    bool slide = false;
};

class Player : public Entity {
public:
    InputIntent in;
    float accel = 0.0f;
    float vmax  = 0.0f;
    float shootCooldown = 0.0f;
    float slideCooldown = 0.0f;
    Vec2  facing;
    bool  tackling = false;
    float tackleTimer = 0.0f;

    Player();
    void applyInput(float dt);
    bool tryShoot(Ball& ball);
    void trySlide(Ball& ball, float dt);

    // ✨ THÊM MỚI: khai báo đúng chữ ký
    void assistDribble(Ball& ball, float dt);
};
