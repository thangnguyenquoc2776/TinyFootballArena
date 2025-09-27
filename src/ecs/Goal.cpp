#include "ecs/Goal.hpp"

void Goals::init(int fieldWidth, int fieldHeight, float goalHalfHeight, float postRadius) {
    float centerY = fieldHeight / 2.0f;
    goalY1 = centerY - goalHalfHeight;
    goalY2 = centerY + goalHalfHeight;

    leftPosts[0].pos = {0.0f, goalY1};
    leftPosts[1].pos = {0.0f, goalY2};
    leftPosts[0].radius = leftPosts[1].radius = postRadius;

    rightPosts[0].pos = {static_cast<float>(fieldWidth), goalY1};
    rightPosts[1].pos = {static_cast<float>(fieldWidth), goalY2};
    rightPosts[0].radius = rightPosts[1].radius = postRadius;
}

int Goals::checkGoal(const Ball& ball) {
    if (ball.tf.pos.x - ball.radius < 0 &&
        ball.tf.pos.y > goalY1 && ball.tf.pos.y < goalY2) {
        return 2; // đội phải ghi bàn
    }
    if (ball.tf.pos.x + ball.radius > rightPosts[0].pos.x &&
        ball.tf.pos.y > goalY1 && ball.tf.pos.y < goalY2) {
        return 1; // đội trái ghi bàn
    }
    return 0;
}
