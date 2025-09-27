#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"

// Lớp Goalkeeper (thủ môn AI)
class Goalkeeper : public Player {
public:
    Goalkeeper();
    void updateAI(const Ball& ball, float fieldCenterY, float dt);
};
