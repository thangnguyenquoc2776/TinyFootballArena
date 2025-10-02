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

// Critically-damped smoothing (không overshoot, ổn định số)
Vec2 DribbleSystem::smoothDamp(const Vec2& current, const Vec2& target,
                               Vec2& vel, float smoothTime, float dt)
{
    smoothTime = std::max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * dt;
    float expf = 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);

    Vec2 change = current - target;
    Vec2 temp   = (vel + change * omega) * dt;
    vel = (vel - temp * omega) * expf;
    return target + (change + temp) * expf;
}

void DribbleSystem::update(Ball& ball, Player& h, float dt) {
    Vec2& fv = filtVel[h.id]; // vận tốc nội bộ của bộ lọc

    // Hướng mặt & các tham số cơ bản
    Vec2 dir = h.facing.normalized(); if (dir.length() < 1e-6f) dir = Vec2(1,0);
    float pSpd = h.tf.vel.length();
    bool moving = (pSpd > 0.6f * 40.0f);

    // Lead động theo tốc độ + lateral bias để bóng lệch 1 bên
    float lead = h.radius + ball.radius + P.extraLead + P.leadSpeedK * pSpd;
    Vec2 perp(-dir.y, dir.x);
    float side = (dir.x >= 0.0f) ? 1.0f : -1.0f;    // đơn giản: quay mặt sang phải dùng chân phải
    Vec2 lateral = perp * (side * P.lateralBias);

    Vec2 target = h.tf.pos + dir * lead + lateral;

    // mất bóng nếu quá xa người
    if ( (ball.tf.pos - h.tf.pos).length() > (lead + P.loseDistance) ) {
        ball.owner = nullptr;
        return;
    }

    // Smooth theo trạng thái (đi/đứng)
    float st = moving ? P.smoothTimeMove : P.smoothTimeStop;

    // SmoothDamp vị trí → target (không teleport)
    Vec2 prev = ball.tf.pos;
    ball.tf.pos = smoothDamp(prev, target, fv, st, dt);

    // Deadzone: rất gần mục tiêu khi chậm → snap để diệt rung vặt
    Vec2 toT = target - ball.tf.pos;
    if (!moving && toT.length() < P.targetDeadRad) {
        ball.tf.pos = target;
    }

    // Ước lượng vận tốc rồi căn hướng mượt về mặt cầu thủ
    Vec2 v = (ball.tf.pos - prev) * (1.0f / std::max(1e-4f, dt));
    float vlen = v.length();

    if (vlen > 1.0f) {
        Vec2 vdir = v * (1.0f / vlen);

        // Trộn hướng với mặt cầu thủ để tránh "loạn hướng"
        float align = 0.35f + 0.50f * std::min(1.0f, pSpd / (h.vmax + 1.0f)); // 0.35..0.85
        Vec2 blend = (vdir * (1.0f - align) + dir * align);
        float bl = blend.length();
        if (bl > 1e-6f) blend = blend * (1.0f / bl);
        else            blend = dir;

        // Giới hạn tốc độ xoay để mượt
        Vec2 ndir = rotateTowards(vdir, blend, P.turnRate * dt);
        v = ndir * vlen;
    }

    // Đặt magnitude theo mong muốn (đi chậm khi chậm, nhanh khi nhanh)
    float want = std::clamp(P.minSpeed + pSpd * P.carryFactor, P.minSpeed, P.maxSpeed);
    float mag = std::clamp(v.length(), P.minSpeed, want);
    if (mag > 1.0f) v = v * (mag / std::max(1.0f, v.length()));

    // Khi đứng yên → dập vận tốc nhanh để tránh “đập đập”
    if (!moving) v *= std::exp(-P.idleDamping * dt);

    ball.tf.vel = v;
}
