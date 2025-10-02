#pragma once
#include "ecs/Entity.hpp"
#include "ui/Animation.hpp"
#include <SDL_mixer.h>

class Ball;

struct InputIntent {
    float x = 0.f, y = 0.f;
    bool shoot = false;
    bool slide = false;
    bool switchGK = false;     // <== NÚT ĐỔI GK
};

class Player : public Entity {
public:
    // Input & control flags
    InputIntent in;
    bool isControlled = false;   // đang do người chơi điều khiển?
    bool isGoalkeeper = false;   // đây là GK?

    // Movement/physics
    float accel = 0.0f;
    float vmax  = 0.0f;
    float shootCooldown = 0.0f;
    float slideCooldown = 0.0f;
    float tackleTimer = 0.0f;
    bool  tackling = false;
    Vec2  facing;

    // Animation
    Animation idle[4];
    Animation run[4];
    int dir = 0;

    // SFX
    Mix_Chunk* kickSfx = nullptr;

    Player();
    void applyInput(float dt);
    bool tryShoot(Ball& ball);
    void trySlide(Ball& ball, float dt);
    void assistDribble(Ball& ball, float dt);
    void updateAnim(float dt);
};
