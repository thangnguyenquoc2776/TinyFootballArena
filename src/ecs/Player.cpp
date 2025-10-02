#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <cmath>
#include <algorithm>
#include "ui/Animation.hpp"

// ===== Tunables =====
static const float PPM             = 40.0f;   // px-per-meter
static const float MAX_FACE_TURN   = 4.0f;    // rad/s – xoay mặt người chơi
static const float MAX_BALL_TURN   = 2.0f;    // rad/s – tốc độ đổi hướng bóng khi đang rê
static const float MAX_DRIBBLE_V   = 4.6f*PPM;// tốc độ tối đa của bóng khi rê
static const float CAPTURE_CONE_DEG= 60.0f;   // góc trước mặt để nhận bóng
static const float CAPTURE_SPEED_GK= 3.5f*PPM;// (để đồng bộ luật nhặt bóng nếu có)
static const float CAPTURE_SPEED_PL= 6.0f*PPM;

static inline float clampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }

static inline Vec2 rotateTowards(const Vec2& a, const Vec2& b, float maxRad) {
    Vec2 from = a.normalized(); if (from.length() < 1e-6f) from = Vec2(1,0);
    Vec2 to   = b.normalized(); if (to.length()   < 1e-6f) to   = Vec2(1,0);
    float dot = clampf(Vec2::dot(from,to), -1.0f, 1.0f);
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

    // đang xoạc thì chỉ trôi theo drag
    if (tackling) {
        tackleTimer -= dt;
        if (tackleTimer <= 0) tackling = false;
        float dmp = std::exp(-drag * dt);
        tf.vel *= dmp;
        return;
    }

    // input trục (x,y) đã có sẵn trong struct in
    if (in.x != 0.0f || in.y != 0.0f) {
        Vec2 moveDir(in.x, in.y);
        moveDir = moveDir.normalized();

        // xoay mặt mượt
        facing = rotateTowards(facing, moveDir, MAX_FACE_TURN * dt);

        // tăng tốc về vận tốc mong muốn
        Vec2 desired = moveDir * vmax;
        Vec2 delta   = desired - tf.vel;
        float maxDv  = accel * dt;
        float len    = delta.length();
        if (len > maxDv && len > 1e-6f) delta = delta * (maxDv/len);
        tf.vel += delta;
    } else {
        // không bấm phím → giảm tốc mượt (exponential damping)
        float dmp = std::exp(-drag * dt * 0.5f);
        tf.vel *= dmp;
    }

    // clamp vmax
    float sp2 = tf.vel.length2();
    if (sp2 > vmax*vmax) {
        float sp = std::sqrt(sp2);
        tf.vel = tf.vel * (vmax / sp);
    }
}

// --- Sút: cho phép nếu bóng nằm trong cửa sổ phía trước mũi chân (không cần owner tuyệt đối)
bool Player::tryShoot(Ball& ball) {
    Vec2 dir = facing.normalized();
    if (dir.length() < 1e-6f) dir = Vec2(1,0);

    // cửa sổ phía trước
    float baseLead = radius + ball.radius + 9.0f;
    float minLong  = baseLead - 10.0f;
    float maxLong  = baseLead + 24.0f;
    float maxLat   = 12.0f;

    Vec2 rel    = ball.tf.pos - tf.pos;
    float longi = Vec2::dot(rel, dir);
    float lat   = std::abs(rel.x*dir.y - rel.y*dir.x);

    bool inFrontWindow = (longi >= minLong && longi <= maxLong && lat <= maxLat);
    float nearR = radius + ball.radius + 16.0f;
    bool veryClose = (rel.length2() <= nearR*nearR);

    bool canShoot = (ball.owner == this) || inFrontWindow || veryClose;
    if (!canShoot) return false;

    // lực sút: cơ bản + bonus theo tốc độ chạy
    float baseSpeed = 13.5f * PPM;
    float runBoost  = std::min(tf.vel.length() * 0.45f, 3.0f * PPM);
    float power     = baseSpeed + runBoost;

    ball.owner = nullptr;
    ball.tf.vel = dir * power;

    // chống “nhặt lại ngay”
    ball.lastKickerId = this->id;
    ball.justKicked   = 0.30f;
    return true;
}

// --- Xoạc: hất bóng nếu trong tầm
void Player::trySlide(Ball& ball, float /*dt*/) {
    if (slideCooldown > 0 || tackling) return;

    tackling = true;
    tackleTimer = 0.25f;
    slideCooldown = 1.0f;

    tf.vel = facing.normalized() * (8.0f * PPM);

    float reach = radius + ball.radius + 12.0f;
    Vec2 toBall = ball.tf.pos - tf.pos;
    if (toBall.length2() <= reach*reach) {
        Vec2 n = toBall.normalized();
        if (n.length() < 1e-6f) n = facing.normalized();
        float knock = 9.5f * PPM;
        ball.owner = nullptr;
        ball.tf.vel = n * knock;
        ball.lastKickerId = this->id;
        ball.justKicked   = 0.25f;
    }
}

// --- Dribble: nhận bóng khi đủ điều kiện; nếu đang sở hữu thì follow theo vị trí mượt + kẹp góc
void Player::assistDribble(Ball& ball, float dt) {
    Vec2 fwd = facing.normalized(); if (fwd.length()<1e-6f) fwd = Vec2(1,0);

    // 1) Nếu CHƯA sở hữu → thử nhận bóng (đúng cone + tầm + bóng không quá nhanh)
    if (ball.owner == nullptr) {
        // không cho người vừa sút nhặt lại ngay
        if (!(ball.justKicked > 0.0f && ball.lastKickerId == this->id)) {
            Vec2 toBall = ball.tf.pos - tf.pos;
            float d = toBall.length(); if (d > 1e-6f) {
                Vec2 dirToBall = toBall * (1.0f/d);
                float cosA = Vec2::dot(dirToBall, fwd);
                float coneCos = std::cos(CAPTURE_CONE_DEG * 3.14159265f / 180.0f);
                float capRange = radius + ball.radius + 16.0f;
                float maxSp    = CAPTURE_SPEED_PL;

                if (cosA > coneCos && d < capRange && ball.tf.vel.length() < maxSp) {
                    ball.owner = this; // nhận quyền sở hữu
                }
            }
        }
    }

    // 2) Nếu KHÔNG phải mình sở hữu → không can thiệp
    if (ball.owner != this) return;

    // 3) Follow mượt theo vị trí đích trước mũi chân với giới hạn đổi hướng
    float speed = tf.vel.length();
    float lead  = (radius + ball.radius + 10.0f) + 0.03f * speed; // chạy nhanh → bóng xa hơn chút

    // Hướng hiện tại của vector (cầu thủ -> bóng) để quay dần về facing
    Vec2 rel = ball.tf.pos - tf.pos;
    float rlen = rel.length();
    Vec2 curDir = (rlen < 1e-4f) ? fwd : (rel * (1.0f/rlen));
    Vec2 aimDir = rotateTowards(curDir, fwd, MAX_BALL_TURN * dt);

    Vec2 desiredPos = tf.pos + aimDir * lead;

    // Exponential smoothing vị trí để tránh “nam châm”
    Vec2 prevPos = ball.tf.pos;
    float posAlpha = 1.0f - std::exp(-12.0f * dt);     // nhanh vừa phải
    ball.tf.pos = prevPos + (desiredPos - prevPos) * posAlpha;

    // Tính vận tốc mục tiêu từ dịch chuyển vị trí, rồi lọc mượt
    Vec2 targetVel = (ball.tf.pos - prevPos) * (1.0f / std::max(1e-4f, dt));
    float velAlpha = 1.0f - std::exp(-10.0f * dt);
    ball.tf.vel = ball.tf.vel * (1.0f - velAlpha) + targetVel * velAlpha;

    // Kẹp tốc độ bóng khi rê
    float sp = ball.tf.vel.length();
    if (sp > MAX_DRIBBLE_V) ball.tf.vel = ball.tf.vel * (MAX_DRIBBLE_V / sp);

    // Khi người gần như đứng yên → dập thêm để bóng ngừng rung
    if (speed < 0.25f * vmax) {
        float extra = 1.0f - std::exp(-12.0f * dt);
        ball.tf.vel = ball.tf.vel * (1.0f - extra);
    }
}

void Player::updateAnim(float dt) {
    // Xác định hướng theo vận tốc
    if (fabs(tf.vel.x) > fabs(tf.vel.y)) {
        dir = (tf.vel.x > 0) ? 2 : 1; // right=2, left=1
    } else if (fabs(tf.vel.y) > 0) {
        dir = (tf.vel.y > 0) ? 0 : 3; // down=0, up=3
    }

    // Idle hay chạy?
    bool moving = (fabs(tf.vel.x) > 1 || fabs(tf.vel.y) > 1);
    if (moving) run[dir].update(dt);
    else idle[dir].update(dt);
}

