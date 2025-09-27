#pragma once
#include <vector>
#include "ecs/Entity.hpp"
#include "ecs/Goal.hpp"

// Hệ thống vật lý: xử lý tích hợp chuyển động và va chạm giữa các thực thể
class PhysicsSystem {
public:
    // Hàm cập nhật vật lý cho danh sách thực thể trong dt thời gian
    void step(float dt, std::vector<Entity*>& entities, Goals& goals, int fieldWidth, int fieldHeight);
};
