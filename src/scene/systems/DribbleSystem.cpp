#include "scene/systems/DribbleSystem.hpp"

static const float PI = 3.14159265358979323846f;

Vec2 DribbleSystem::rotateTowards(const Vec2& a, const Vec2& b, float maxRad) {
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

void DribbleSystem::update(Ball& ball, Player& h, float dt) {
    float& touchTimer = timers[h.id]; // mặc định 0 nếu chưa có

    Vec2 dir = h.facing.normalized();
    if (dir.length() < 1e-6f) dir = Vec2(1,0);

    float lead = h.radius + ball.radius + P.extraLead;
    Vec2  contactPos = h.tf.pos + dir * lead;

    // Mất bóng nếu quá xa thân người
    if ( (ball.tf.pos - h.tf.pos).length() > (lead + P.loseDistance) ) {
        ball.owner = nullptr;
        return;
    }

    // ==== IDLE SETTLE: đứng yên thì KHÔNG tap, ghìm bóng về vị trí trước mũi chân ====
    const float PLAYER_IDLE_SPD = 0.6f * 40.0f;  // ~0.6 m/s
    const float SETTLE_K        = 28.0f;         // độ cứng lò xo
    const float SETTLE_D        = 12.0f;         // giảm chấn
    float pspd   = h.tf.vel.length();
    bool  moving = pspd > PLAYER_IDLE_SPD;

    if (!moving) {
        // spring–damper tới contactPos
        Vec2 delta = contactPos - ball.tf.pos;
        Vec2 acc   = delta * SETTLE_K - ball.tf.vel * SETTLE_D;
        ball.tf.vel += acc * dt;

        // diệt jitter rất nhỏ
        if (delta.length() < 1.5f && ball.tf.vel.length() < 8.0f) {
            ball.tf.pos = contactPos;
            ball.tf.vel = Vec2(0,0);
        }
        // reset để khi bắt đầu chạy lại là tap ngay
        touchTimer = 0.0f;
        return;
    }

    // ==== ĐANG DI CHUYỂN → tap theo nhịp + xoay mượt ====
    touchTimer -= dt;
    if (touchTimer <= 0.0f) {
        // Mỗi lần chạm: đặt bóng sát trước mũi chân và đẩy theo hướng mặt
        float carry = pspd * P.carryFactor;
        float sp = std::clamp(P.touchSpeed + carry, P.minSpeed, P.maxSpeed);

        ball.tf.pos = contactPos;
        ball.tf.vel = dir * sp;

        touchTimer = 1.0f / P.tps;
    } else {
        // Xoay hướng vận tốc về hướng mặt – tốc độ xoay tỉ lệ với tốc độ chạy
        float vlen = ball.tf.vel.length();
        if (vlen > 1.0f) {
            float turn = P.turnRate * (0.6f + 0.4f * std::min(1.0f, pspd / (h.vmax + 1.0f)));
            Vec2  vdir = ball.tf.vel * (1.0f / vlen);
            Vec2  ndir = rotateTowards(vdir, dir, turn * dt);
            ball.tf.vel = ndir * vlen;
        }

        // Hút nhẹ để bóng bám quỹ đạo phía trước, không bị lọt ra sau gót
        Vec2 toContact = contactPos - ball.tf.pos;
        float dist = toContact.length();
        if (dist > 0.5f) {
            float pull = std::min(dist * 5.0f * dt, 8.0f);
            ball.tf.pos += toContact * (pull / std::max(dist, 1e-3f));
        }

        // Giới hạn tốc độ
        float sp = ball.tf.vel.length();
        if (sp > P.maxSpeed) ball.tf.vel *= (P.maxSpeed / sp);
        if (sp < P.minSpeed) ball.tf.vel  = dir * P.minSpeed;
    }
}
