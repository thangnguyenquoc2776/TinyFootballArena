#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <cmath>

static const float CONTROL_CONE_ANGLE_COS = std::cos(35.0f * M_PI / 180.0f);

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
        facing  = moveDir;
        tf.vel.x += moveDir.x * accel * dt;
        tf.vel.y += moveDir.y * accel * dt;
    }

    tf.vel.x *= std::exp(-drag * dt);
    tf.vel.y *= std::exp(-drag * dt);

    float speed2 = tf.vel.x*tf.vel.x + tf.vel.y*tf.vel.y;
    if (speed2 > vmax*vmax) {
        float speed = std::sqrt(speed2);
        tf.vel.x = tf.vel.x / speed * vmax;
        tf.vel.y = tf.vel.y / speed * vmax;
    }
}

bool Player::tryShoot(Ball& ball) {
    if (shootCooldown > 0) return false;

    float reach = radius + ball.radius + 6.0f;
    Vec2  toBall = ball.tf.pos - tf.pos;
    bool  closeEnough = (toBall.length2() <= reach*reach);

    if (ball.owner == this || closeEnough) {
        Vec2 dir = facing.normalized();

        // ⚽ Lực sút mạnh hơn, có cộng thêm tốc độ đang chạy
        //  ~7.5 m/s trên mỗi 0.43 kg → ~17-18 m/s (≈700 px/s) + bonus theo tốc độ chạy
        float base = (7.5f / ball.mass) * 40.0f;            // px/s
        float runBonus = std::min(tf.vel.length() * 0.30f, 120.0f); // bonus tối đa 120 px/s
        ball.owner = nullptr;                                // nhả bóng
        ball.tf.vel = dir * (base + runBonus);
        shootCooldown = 0.25f;
        return true;
    }
    return false;
}

void Player::trySlide(Ball& ball, float /*dt*/) {
    if (slideCooldown > 0 || tackling) return;

    tackling = true;
    tackleTimer = 0.25f;
    slideCooldown = 1.0f;

    tf.vel = facing * (8.0f * 40.0f);   // dash

    // 🦵 Xoạc trúng → bóng VĂNG RA và mất quyền sở hữu (không cướp ngay)
    float reach = radius + ball.radius + 10.0f;
    Vec2 toBall = ball.tf.pos - tf.pos;
    if (toBall.length2() <= reach*reach) {
        Vec2 dir = facing.normalized();
        float ejection = 7.0f * 40.0f;          // ~7 m/s văng ra
        ball.owner = nullptr;                   // mất quyền kiểm soát
        ball.tf.vel = dir * ejection;           // đẩy bóng bay ra
    }
}


// Giữ bóng mềm: chỉ dùng nếu bạn vẫn gọi, còn cơ chế chính nay là owner
void Player::assistDribble(Ball& ball, float dt) {
    if (ball.owner && ball.owner != this) return; // người khác đang giữ
    const float PPM = 40.0f;
    const float EXTRA_CONTROL = 0.7f * PPM;
    const float MAX_BALL_SPEED = 3.0f * PPM;
    const float LEAD_DIST = radius + ball.radius + 6.0f;
    const float SPRING_K = 18.0f; // mạnh hơn bản cũ

    Vec2 toBall = ball.tf.pos - tf.pos;
    float dist = toBall.length();
    if (dist <= 1e-3f) return;

    Vec2 fwd = facing.normalized();
    Vec2 dirToBall = toBall * (1.0f / dist);
    float cosAngle = Vec2::dot(dirToBall, fwd);

    if (cosAngle > CONTROL_CONE_ANGLE_COS && dist < (radius + ball.radius + EXTRA_CONTROL)) {
        float vBall = ball.tf.vel.length();
        if (vBall < MAX_BALL_SPEED) {
            Vec2 desired = tf.pos + fwd * LEAD_DIST;
            Vec2 correction = desired - ball.tf.pos;
            ball.tf.vel += correction * (SPRING_K * dt);
            ball.tf.vel += fwd * (tf.vel.length() * 0.35f * dt);
        }
    }
}
