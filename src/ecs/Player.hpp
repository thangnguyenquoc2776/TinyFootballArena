#pragma once
#include "ecs/Entity.hpp"

// Forward declare lớp Ball để dùng trong hàm
class Ball;

// Cấu trúc InputIntent: hướng di chuyển và hành động shoot/slide
struct InputIntent {
    float x = 0.0f;
    float y = 0.0f;
    bool shoot = false;
    bool slide = false;
};

// Lớp Player (cầu thủ do người chơi điều khiển)
class Player : public Entity {
public:
    InputIntent in;      // input trong frame hiện tại
    float accel;         // gia tốc di chuyển (px/s^2)
    float vmax;          // tốc độ tối đa (px/s)
    float shootCooldown = 0.0f;  
    float slideCooldown = 0.0f;  
    Vec2 facing;         // hướng mặt
    bool tackling = false;  
    float tackleTimer = 0.0f; 

    Player();

    void applyInput(float dt);
    bool tryShoot(Ball& ball);
    void trySlide(Ball& ball, float dt);
};
