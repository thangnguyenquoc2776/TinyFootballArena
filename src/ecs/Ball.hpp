#pragma once
#include "ecs/Entity.hpp"

// Khai báo sớm để dùng con trỏ đến Player mà không cần include vòng tròn
class Player;

class Ball : public Entity {
public:
    Ball() {}
    // Ai đang giữ bóng; nullptr = bóng tự do
    Player* owner = nullptr;
};
