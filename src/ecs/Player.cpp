#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"
#include <cmath>
#include <algorithm>
#include "ui/Animation.hpp"
#include <unordered_map>
#include <SDL_mixer.h>

// ===== Tham số cảm giác (bạn có thể tinh chỉnh nhanh) =====
static const float PPM               = 40.0f;     // px per meter
static const float MAX_FACE_TURN     = 4.2f;      // rad/s — xoay mặt cầu thủ
static const float CAPTURE_CONE_DEG  = 65.0f;     // góc nhận bóng
static const float PI                = 3.14159265358979323846f;

// Rê theo nhịp (tap) — chốt bóng LUÔN Ở TRƯỚC người
struct DrbState {
    float clock   = 0.0f;           // đếm lùi tới lần chạm tiếp theo
    Vec2  aim     = Vec2(1,0);      // hướng rê đã làm mượt
    // tuning nhịp:
    float tps     = 6.6f;           // touches per second (6.2–7.0 tuỳ gu)
    float touchSp = 4.9f * PPM;     // vận tốc “đẩy” mỗi nhịp
    float carryK  = 0.35f;          // % vận tốc người cộng vào bóng khi tap
    float turnR   = 3.0f;           // rad/s — làm mượt hướng rê
    float maxSp   = 5.2f * PPM;     // kẹp tốc độ bóng lúc rê
    float minSp   = 1.0f * PPM;     // dưới ngưỡng này sẽ tap lại
    float extra   = 6.0f;           // bóng vượt qua lead 1 chút khi tap (px)
    float tapBlend= 0.58f;          // blend vị trí khi tap để tránh teleport
};
static std::unordered_map<const Player*, DrbState> g_drb;

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

// Hướng aim từ input; nếu không bấm thì dùng facing
static inline Vec2 currentAimDir(const Player& p){
    if (std::abs(p.in.x) > 1e-4f || std::abs(p.in.y) > 1e-4f) {
        Vec2 d(p.in.x, p.in.y); return d.normalized();
    }
    return p.facing.normalized();
}

Player::Player(){
    facing = Vec2(1.0f, 0.0f);
    kickSfx = Mix_LoadWAV("assets/audio/kick.wav");
}

void Player::applyInput(float dt){
    if (shootCooldown > 0) shootCooldown -= dt;
    if (slideCooldown > 0) slideCooldown -= dt;

    // đang xoạc → chỉ trôi theo kháng lực
    if (tackling){
        tackleTimer -= dt;
        if (tackleTimer <= 0) tackling = false;
        float dmp = std::exp(-drag * dt);
        tf.vel *= dmp;
        return;
    }

    // điều khiển
    if (in.x != 0.0f || in.y != 0.0f) {
        Vec2 moveDir(in.x, in.y); moveDir = moveDir.normalized();

        // Thay vì facing = rotateTowards(facing, moveDir, MAX_FACE_TURN*dt);
        Vec2 targetDir = moveDir.normalized();
        facing = rotateTowards(facing, targetDir, MAX_FACE_TURN * dt * 1.5f); 


        // tiến tới vận tốc mong muốn bằng “gia tốc tối đa”
        Vec2 desired = moveDir * vmax;
        Vec2 delta   = desired - tf.vel;
        float maxDv  = accel * dt;
        float len    = delta.length();
        if (len > maxDv && len > 1e-6f) delta = delta * (maxDv/len);
        tf.vel += delta;
    } else {
        // buông phím → giảm tốc mượt
        float dmp = std::exp(-drag * dt * 0.5f);
        tf.vel *= dmp;
    }

    // kẹp vmax
    float sp2 = tf.vel.length2();
    if (sp2 > vmax*vmax){
        float sp = std::sqrt(sp2);
        tf.vel = tf.vel * (vmax / sp);
    }
}

// --- Sút: aim CHUẨN theo input, lực mạnh; đặt bóng ra trước mũi giày để khỏi lệch do va chạm frame đầu
bool Player::tryShoot(Ball& ball){
    Vec2 aim = currentAimDir(*this);
    if (aim.length() < 1e-6f) aim = Vec2(1,0);

    // cửa sổ trước mũi chân (không bắt buộc owner)
    float baseLead = radius + ball.radius + 10.0f;
    float minLong  = baseLead - 10.0f;
    float maxLong  = baseLead + 28.0f;
    float maxLat   = 12.0f;

    Vec2 rel    = ball.tf.pos - tf.pos;
    float longi = Vec2::dot(rel, aim);
    float lat   = std::abs(rel.x*aim.y - rel.y*aim.x);

    bool inFrontWindow = (longi >= minLong && longi <= maxLong && lat <= maxLat);
    float nearR = radius + ball.radius + 18.0f;
    bool veryClose = (rel.length2() <= nearR*nearR);

    if (!(ball.owner == this || inFrontWindow || veryClose)) return false;

    if(kickSfx) Mix_PlayChannel(-1, kickSfx, 0);

    // Lực bắn: base mạnh + bonus theo tốc độ chạy
    float baseSpeed = 16.8f * PPM;
    float runBoost  = std::min(tf.vel.length() * 0.60f, 5.0f * PPM);
    float power     = baseSpeed + runBoost;

    // đặt bóng ra ngoài “hình cầu” của người để tránh lệch hướng tức thời
    float safeLead  = radius + ball.radius + 3.0f;
    ball.owner = nullptr;
    ball.tf.pos = tf.pos + aim * safeLead;
    ball.tf.vel = aim * power;

    ball.lastKickerId = this->id;
    ball.justKicked   = 0.33f;
    return true;
}

// --- Xoạc: hất bóng khi trong tầm
void Player::trySlide(Ball& ball, float /*dt*/){
    if (slideCooldown > 0 || tackling) return;

    tackling = true;
    tackleTimer = 0.25f;
    slideCooldown = 1.0f;

    tf.vel = currentAimDir(*this) * (8.0f * PPM);

    float reach = radius + ball.radius + 12.0f;
    Vec2 toBall = ball.tf.pos - tf.pos;
    if (toBall.length2() <= reach*reach) {
        Vec2 n = toBall.normalized();
        if (n.length() < 1e-6f) n = currentAimDir(*this);
        float knock = 10.0f * PPM;
        ball.owner = nullptr;
        ball.tf.vel = n * knock;

        ball.lastKickerId = this->id;
        ball.justKicked   = 0.28f;
    }
}

// --- Rê: TAP–TAP (bóng ĐƯỢC ĐẨY đi TRƯỚC, không bị “hút”) ---
void Player::assistDribble(Ball& ball, float dt){
    DrbState& S = g_drb[this];
    Vec2 rawAim = currentAimDir(*this);
    if (S.aim.length() < 1e-4f) S.aim = rawAim; // init
    S.aim = rotateTowards(S.aim, rawAim, S.turnR * dt);

    // 1) Nhận bóng khi tự do (cone/tầm/tốc độ)
    if (ball.owner == nullptr && !(ball.justKicked > 0 && ball.lastKickerId == this->id)) {
        Vec2 toBall = ball.tf.pos - tf.pos; float d = toBall.length();
        if (d > 1e-6f){
            Vec2 dirToBall = toBall * (1.0f/d);
            float coneCos  = std::cos(CAPTURE_CONE_DEG * PI/180.0f);
            float cosA     = Vec2::dot(dirToBall, S.aim);
            float capRange = radius + ball.radius + 18.0f;
            float maxSp    = 6.5f * PPM;
            if (cosA > coneCos && d < capRange && ball.tf.vel.length() < maxSp) {
                ball.owner = this;
                S.clock = 0.0f; // ép tap ngay
            }
        }
    }
    if (ball.owner != this) return;

    // 2) Hệ trục dọc–ngang theo hướng di chuyển (fix dây thun khi đi lùi)
    Vec2 axis = currentAimDir(*this);  // dùng input làm chuẩn
    if (axis.length() < 1e-6f) axis = facing; // fallback khi đứng yên
    axis = axis.normalized();
    Vec2 perp(-axis.y, axis.x);


    float speed   = tf.vel.length();
    float lead    = (radius + ball.radius + 8.0f) + 0.040f * speed; // chạy nhanh → bóng xa chân hơn

    Vec2 rel = ball.tf.pos - tf.pos;
    float longi = Vec2::dot(rel, axis);
    float lat   = Vec2::dot(rel, perp);

    // 3) Đến nhịp (hoặc bóng tụt/đi quá chậm) → TAP: đẩy bóng về TRƯỚC
    S.clock -= dt;
    bool needTap = (S.clock <= 0.0f) || (longi < 0.85f*lead) || (ball.tf.vel.length() < S.minSp);
    if (needTap){
        S.clock = 1.0f / S.tps;

        // vị trí mục tiêu khi tap: trước mũi chân một chút + giảm sai số ngang
        Vec2 targetPos = tf.pos + axis * (lead + S.extra) + perp * (lat * 0.35f);
        // blend ít hơn để không dịch chuyển mạnh
        float posBlend = 0.25f;
        ball.tf.pos = ball.tf.pos + (targetPos - ball.tf.pos) * posBlend;

        // vận tốc: blend giữa vel cũ và vel mong muốn
        Vec2 desiredVel = axis * S.touchSp + tf.vel * S.carryK;
        ball.tf.vel = ball.tf.vel * 0.6f + desiredVel * 0.4f;


        // kẹp tốc độ tối đa lúc rê
        float bsp = ball.tf.vel.length();
        if (bsp > S.maxSp) ball.tf.vel = ball.tf.vel * (S.maxSp / bsp);

        return; // xong nhịp tap, hết frame này
    }

    // 4) Giữa các nhịp: steer + giữ bóng ở TRƯỚC (không kéo cứng)
    // steer vận tốc về gần axis
    float sp = ball.tf.vel.length();
    if (sp > 1e-4f){
        Vec2 vdir = ball.tf.vel * (1.0f / sp);
        Vec2 v2   = rotateTowards(vdir, axis, (S.turnR*0.55f) * dt);
        // blend với vel cũ để mượt hơn
        ball.tf.vel = ball.tf.vel * 0.85f + v2 * (sp * 0.15f);

    }

    // hiệu chỉnh nhẹ vị trí dọc–ngang để duy trì “ở trước”
    float aLong = 1.0f - std::exp(-10.0f * dt);
    float aLat  = 1.0f - std::exp(-16.0f * dt);
    float wantLong = lead;
    float wantLat  = 0.0f;
    longi += (wantLong - longi) * aLong;
    lat   += (wantLat  - lat)   * aLat;

    Vec2 desiredPos = tf.pos + axis * longi + perp * lat;
    float posAlpha  = 1.0f - std::exp(-12.0f * dt);
    ball.tf.pos = ball.tf.pos + (desiredPos - ball.tf.pos) * posAlpha;

    // kẹp tốc độ tối đa lúc rê
    float bsp = ball.tf.vel.length();
    if (bsp > S.maxSp) ball.tf.vel = ball.tf.vel * (S.maxSp / bsp);

    // đứng gần yên → dập nhanh để bóng “kẹt” ổn định ngay trước mũi giày
    if (speed < 0.22f * vmax) {
        float extra = 1.0f - std::exp(-18.0f * dt);
        ball.tf.vel = ball.tf.vel * (1.0f - extra);
        // kéo bóng sát trục dọc hơn
        float snap = 1.0f - std::exp(-20.0f * dt);
        ball.tf.pos = ball.tf.pos + (tf.pos + axis*lead - ball.tf.pos) * snap;
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

