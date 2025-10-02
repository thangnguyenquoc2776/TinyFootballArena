#pragma once
#include "ecs/Entity.hpp"
#include "ui/Animation.hpp"
#include <SDL_mixer.h>


class Ball;

struct InputIntent {
    float x = 0.0f;
    float y = 0.0f;
    bool shoot = false;
    bool slide = false;
    bool switchGK = false;
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
    
    // NEW: Animations
    Animation idle[4];  // idle cho 4 hướng
    Animation run[4];   // run cho 4 hướng
    int dir = 0;        // hướng hiện tại (0=down, 1=left, 2=right, 3=up)
    Mix_Chunk* kickSfx    = nullptr;
    
    bool isControlled = false;   // 🟢 thêm dòng này cho tất cả Player/GK
    bool isGoalkeeper = false;   // 🟢 để phân biệt GK với cầu thủ thường


    Player();
    void applyInput(float dt);
    bool tryShoot(Ball& ball);
    void trySlide(Ball& ball, float dt);

    void updateAnim(float dt);   // <<< thêm dòng này


    // ✨ THÊM MỚI: khai báo đúng chữ ký
    void assistDribble(Ball& ball, float dt);
};
