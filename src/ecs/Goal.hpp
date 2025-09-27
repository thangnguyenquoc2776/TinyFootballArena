#pragma once
#include "util/Math.hpp"
#include "ecs/Ball.hpp"

// Cấu trúc trụ cầu môn (post)
struct Post {
    Vec2 pos;
    float radius;
};

// Lớp Goals quản lý 2 khung thành (trái và phải) và điểm số
class Goals {
public:
    Post leftPosts[2];
    Post rightPosts[2];
    int scoreLeft = 0;
    int scoreRight = 0;
    float goalY1; // toạ độ Y mép trên cầu môn
    float goalY2; // toạ độ Y mép dưới cầu môn

    // Khởi tạo vị trí cột gôn dựa trên kích thước sân và chiều cao cầu môn
    void init(int fieldWidth, int fieldHeight, float goalHalfHeight, float postRadius);
    // Kiểm tra bóng qua vạch cầu môn chưa (trả về 1 nếu đội nhà ghi bàn, 2 nếu đội khách, 0 nếu chưa)
    int checkGoal(const Ball& ball);
};
