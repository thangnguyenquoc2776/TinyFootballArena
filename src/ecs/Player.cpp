#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <cmath>
#include <algorithm>

// Giới hạn góc quay hướng nhìn (rad/s) để input 8 hướng không làm “giật”
static const float MAX_FACE_TURN = 4.0f; // ≈229°/s

static inline Vec2 rotateTowards(const Vec2& a, const Vec2& b, float maxRad) {
    Vec2 from = a.normalized(); if (from.length() < 1e-6f) from = Vec2(1,0);
    Vec2 to   = b.normalized(); if (to.length()   < 1e-6f) to   = Vec2(1,0);
    float dot = std::max(-1.0f, std::min(1.0f, Vec2::dot(from,to)));
    float ang = std::acos(dot);
    if (ang <= maxRad || ang < 1e-4f) return to;
    float cross = from.x*to.y - from.y*to.x;
    float sgn   = (cross >= 0.f) ? 1.f : -1.f;
    float rot   = maxRad * sgn;
    float cs = std::cos(rot), sn = std::sin(rot);
    return Vec2(from.x*cs - from.y*sn, from.x*sn + from.y*cs);
}

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

        // Hướng nhìn xoay dần (không nhảy)
        facing = rotateTowards(facing, moveDir, MAX_FACE_TURN * dt);

        tf.vel.x += moveDir.x * accel * dt;
        tf.vel.y += moveDir.y * accel * dt;
    }

    // Drag + clamp vmax
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
        // Lực sút có bonus theo tốc độ chạy (nhưng không quá “nổ”)
        float base = (7.2f / ball.mass) * 40.0f;             // px/s ~ 16.7 m/s
        float runBonus = std::min(tf.vel.length() * 0.25f, 100.0f);
        ball.owner = nullptr;
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

    tf.vel = facing * (8.0f * 40.0f);

    // Xoạc trúng → bóng văng ra + mất quyền sở hữu
    float reach = radius + ball.radius + 10.0f;
    Vec2 toBall = ball.tf.pos - tf.pos;
    if (toBall.length2() <= reach*reach) {
        Vec2 dir = facing.normalized();
        float ejection = 7.0f * 40.0f; // ~7 m/s
        ball.owner = nullptr;
        ball.tf.vel = dir * ejection;
    }
}

// Assist dribble: chỉ kéo nhẹ khi bóng tự do, đúng góc phía trước
void Player::assistDribble(Ball& ball, float dt) {
    if (ball.owner && ball.owner != this) return; // người khác đang giữ
    const float PPM = 40.0f;
    const float EXTRA_CONTROL = 0.7f * PPM;
    const float MAX_BALL_SPEED = 3.0f * PPM;
    const float LEAD_DIST = radius + ball.radius + 6.0f;
    const float SPRING_K = 14.0f;

    Vec2 toBall = ball.tf.pos - tf.pos;
    float dist = toBall.length();
    if (dist <= 1e-3f) return;

    Vec2 fwd = facing.normalized();
    Vec2 dirToBall = toBall * (1.0f / dist);
    float cosAngle = Vec2::dot(dirToBall, fwd);

    // Cone 70°, trong tầm và bóng chậm → kéo nhẹ về trước mũi chân
    static const float CONE_COS = std::cos(35.0f * M_PI / 180.0f);
    if (cosAngle > CONE_COS && dist < (radius + ball.radius + EXTRA_CONTROL)) {
        float vBall = ball.tf.vel.length();
        if (vBall < MAX_BALL_SPEED) {
            Vec2 desired = tf.pos + fwd * LEAD_DIST;
            Vec2 correction = desired - ball.tf.pos;
            ball.tf.vel += correction * (SPRING_K * dt);
            ball.tf.vel += fwd * (tf.vel.length() * 0.25f * dt);
        }
    }
}
