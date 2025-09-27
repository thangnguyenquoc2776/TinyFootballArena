#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <cmath>

static const float CONTROL_CONE_ANGLE_COS = std::cos(35.0f * M_PI / 180.0f);
static const float CONTROL_RADIUS_FACTOR = 0.7f;

Player::Player() {
    facing = Vec2(1.0f, 0.0f);
}

void Player::applyInput(float dt) {
    if (shootCooldown > 0) shootCooldown -= dt;
    if (slideCooldown > 0) slideCooldown -= dt;

    if (tackling) {
        tackleTimer -= dt;
        if (tackleTimer <= 0) tackling = false;
        tf.vel.x *= std::exp(-drag * dt);
        tf.vel.y *= std::exp(-drag * dt);
        return;
    }

    if (in.x != 0.0f || in.y != 0.0f) {
        Vec2 moveDir(in.x, in.y);
        moveDir = moveDir.normalized();
        facing = moveDir;
        tf.vel.x += moveDir.x * accel * dt;
        tf.vel.y += moveDir.y * accel * dt;
    }

    tf.vel.x *= std::exp(-drag * dt);
    tf.vel.y *= std::exp(-drag * dt);

    float speed2 = tf.vel.x * tf.vel.x + tf.vel.y * tf.vel.y;
    if (speed2 > vmax * vmax) {
        float speed = std::sqrt(speed2);
        tf.vel.x = tf.vel.x / speed * vmax;
        tf.vel.y = tf.vel.y / speed * vmax;
    }
}

bool Player::tryShoot(Ball& ball) {
    if (shootCooldown > 0) return false;

    Vec2 toBall = ball.tf.pos - tf.pos;
    float dist2 = toBall.length2();
    float reach = radius + ball.radius + 4.0f;
    if (dist2 <= reach * reach) {
        Vec2 shootDir = facing.normalized();
        float impulse = 3.2f; 
        float addedSpeed = impulse / ball.mass;
        ball.tf.vel.x = shootDir.x * addedSpeed * 40.0f;
        ball.tf.vel.y = shootDir.y * addedSpeed * 40.0f;
        shootCooldown = 0.25f;
        return true;
    }
    return false;
}

void Player::trySlide(Ball& ball, float dt) {
    if (slideCooldown > 0 || tackling) return;

    tackling = true;
    tackleTimer = 0.25f;
    slideCooldown = 1.2f;
    tf.vel.x = facing.x * 8.0f * 40.0f;
    tf.vel.y = facing.y * 8.0f * 40.0f;

    Vec2 playerToBall = ball.tf.pos - tf.pos;
    float projLen = Vec2::dot(playerToBall, facing);
    Vec2 projVec = facing * projLen;
    Vec2 perpVec = playerToBall - projVec;
    float distSide = perpVec.length();

    if (projLen > 0 && distSide < (ball.radius + 4.0f)) {
        Vec2 slideDir = facing;
        float ballSpeedAlong = Vec2::dot(ball.tf.vel, slideDir);
        float newBallSpeed = 7.0f * 40.0f + 0.5f * ballSpeedAlong;
        ball.tf.vel = slideDir * newBallSpeed;
    }
}
