#pragma once
#include "util/Math.hpp"
#include <SDL.h>

// Cấu trúc dùng cho thành phần vị trí, vận tốc (Transform)
struct Transform {
    Vec2 pos;
    Vec2 vel;
};

// Lớp cơ sở Entity (thực thể game chung)
class Entity {
public:
    int id;
    Transform tf;
    float radius;    // bán kính va chạm (px)
    float mass;
    float drag;      // hệ số cản (s^-1)
    float e_wall;    // hệ số đàn hồi khi va chạm tường
    // Hàm ảo update/render (có thể override nếu cần)
    virtual void update(float dt) {}
    virtual void render(SDL_Renderer* renderer) {}
    virtual ~Entity() {}
};
