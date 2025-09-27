#pragma once
#include <cmath>

// Lớp tiện ích toán học 2D (vector 2 chiều)
struct Vec2 {
    float x;
    float y;
    Vec2(): x(0), y(0) {}
    Vec2(float x_, float y_): x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    float length() const { return std::sqrt(x*x + y*y); }
    float length2() const { return x*x + y*y; }
    Vec2 normalized() const {
        float len = length();
        if (len == 0) return Vec2(0, 0);
        return Vec2(x / len, y / len);
    }
    static float dot(const Vec2& a, const Vec2& b) {
        return a.x * b.x + a.y * b.y;
    }
};
