#include "scene/systems/KeeperSystem.hpp"
#include <algorithm>
#include <cmath>

static const float PI = 3.14159265358979323846f;

float KeeperSystem::clampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }

Vec2 KeeperSystem::rotateTowards(const Vec2& a, const Vec2& b, float maxRad) {
    Vec2 from=a.normalized(); if (from.length()<1e-6f) from=Vec2(1,0);
    Vec2 to  =b.normalized(); if (to.length()  <1e-6f) to  =Vec2(1,0);
    float dot=clampf(Vec2::dot(from,to), -1.0f, 1.0f);
    float ang=std::acos(dot);
    if (ang<=maxRad || ang<1e-4f) return to;
    float cross=from.x*to.y - from.y*to.x;
    float sgn=(cross>=0.f)?1.f:-1.f;
    float rot=maxRad*sgn;
    float cs=std::cos(rot), sn=std::sin(rot);
    return Vec2(from.x*cs - from.y*sn, from.x*sn + from.y*cs);
}

static inline float pointSegDist2(const Vec2& A, const Vec2& B, const Vec2& P, float& tOut){
    Vec2 AB = B - A;
    float L2 = AB.length2();
    if (L2 < 1e-6f) { tOut = 0.0f; return (P - A).length2(); }
    float t = Vec2::dot(P - A, AB) / L2;
    // kẹp [0,1] mà không đụng hàm private
    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    tOut = t;
    Vec2 proj = A + AB * t;
    return (P - proj).length2();
}

bool KeeperSystem::occludedBy(const Player& attacker, const Vec2& gkPos, const Vec2& ballPos, float margin){
    float t; float d2=pointSegDist2(gkPos,ballPos,attacker.tf.pos,t);
    float R=attacker.radius+margin;
    return (t>0.05f && t<0.95f && d2<=R*R);
}

void KeeperSystem::updatePair(Ball& ball,
                              Player& gk1, Player& gk2,
                              Player& p1,  Player& p2,
                              float fieldW, float fieldH, float centerY, float dt,
                              float& pickupCooldown)
{
    const float boxDepth  = fieldW * P.boxDepthRatio;
    const float leftEdge  = fieldW * 0.55f;
    const float rightEdge = fieldW * 0.45f;
    bool ballInLeft  = (ball.tf.pos.x <= leftEdge);
    bool ballInRight = (ball.tf.pos.x >= rightEdge);

    updateOne(ball, gk1, p1, p2, true,  ballInLeft,  ctx1, fieldW, fieldH, centerY, boxDepth, dt, pickupCooldown);
    updateOne(ball, gk2, p2, p1, false, ballInRight, ctx2, fieldW, fieldH, centerY, boxDepth, dt, pickupCooldown);
}

void KeeperSystem::updateOne(Ball& ball, Player& gk, Player& mate, Player& opp, bool leftSide,
                             bool activeSide, Ctx& C, float fieldW, float fieldH, float centerY,
                             float boxDepth, float dt, float& pickupCooldown)
{
    C.stTime += dt;
    float minX = leftSide ? 0.0f : (fieldW - boxDepth);
    float maxX = leftSide ? boxDepth : fieldW;
    float minY = 20.0f, maxY = fieldH - 20.0f;

    bool oppPastMate = leftSide ? (opp.tf.pos.x < mate.tf.pos.x - 8.0f)
                                : (opp.tf.pos.x > mate.tf.pos.x + 8.0f);
    bool oppHasBall  = (ball.owner == &opp);
    bool nearBox     = leftSide ? (ball.tf.pos.x < maxX + 0.35f*boxDepth)
                                : (ball.tf.pos.x > minX - 0.35f*boxDepth);
    bool canCharge   = activeSide && nearBox && (oppHasBall && oppPastMate);

    Vec2 goalC   = leftSide ? Vec2(minX+12.0f, centerY) : Vec2(maxX-12.0f, centerY);
    Vec2 cutPt   = goalC + (ball.tf.pos - goalC) * 0.18f;
    Vec2 intercp = ball.tf.pos + ball.tf.vel * 0.25f;

    const float MIN_CHARGE = 0.45f;
    if (C.st == Hold) {
        // xử lý riêng bên dưới
    } else if (C.st == Set) {
        if (canCharge) { C.st = Charge; C.stTime = 0.f; }
    } else { // Charge
        bool ballAway = leftSide ? (ball.tf.pos.x > maxX + 0.5f*boxDepth)
                                 : (ball.tf.pos.x < minX - 0.5f*boxDepth);
        if ((C.stTime>=MIN_CHARGE) && (!canCharge || ballAway)) { C.st = Set; C.stTime = 0.f; }
    }

    if (C.st == Hold) {
        gk.tf.vel = Vec2(0,0);
        Vec2 clrTgt = leftSide? Vec2(fieldW*0.75f, centerY) : Vec2(fieldW*0.25f, centerY);
        Vec2 desire = (clrTgt - gk.tf.pos).normalized();
        gk.facing = rotateTowards(gk.facing, desire, P.turnRate*dt);

        Vec2 fwd = gk.facing.normalized();
        float holdDist = gk.radius + ball.radius + 4.0f;
        ball.tf.pos = gk.tf.pos + fwd*holdDist;
        ball.tf.vel = Vec2(0,0);

        C.hold += dt;
        bool pressured = leftSide ? ((opp.tf.pos - gk.tf.pos).length() < 60.0f)
                                  : ((opp.tf.pos - gk.tf.pos).length() < 60.0f);
        float ang = std::acos(clampf(Vec2::dot(fwd, desire), -1.0f, 1.0f));
        const float READY = 10.0f*PI/180.0f;

        if (C.hold>=P.maxHold || (pressured && ang<READY) || ang<(6.0f*PI/180.0f)) {
            ball.owner=nullptr; ball.tf.vel = desire * P.clearSpeed;
            C.hold=0.f; C.st=Set; pickupCooldown=P.pickupCooldown;
        }
        return;
    }

    // Move (Set/Charge)
    float walk = gk.vmax*P.walkFactor, rush = gk.vmax*P.rushFactor;
    Vec2 target = (C.st==Set)? cutPt : intercp;
    float speed = (C.st==Set)? walk  : rush;
    Vec2 dirBall = (ball.tf.pos - gk.tf.pos).normalized();
    gk.facing = rotateTowards(gk.facing, dirBall, P.turnRate*dt);

    Vec2 toT = target - gk.tf.pos; float d = toT.length();
    Vec2 desireV = (d>1e-3f)? (toT*(speed/d)) : Vec2(0,0);
    gk.tf.vel = gk.tf.vel*0.80f + desireV*0.20f;

    float ext = (C.st==Charge)? (P.chargeExtendW*fieldW) : 0.f;
    float limMinX = leftSide? (0.f - ext) : (fieldW - boxDepth - ext);
    float limMaxX = leftSide? (boxDepth + ext) : (fieldW + ext);
    gk.tf.pos.x = clampf(gk.tf.pos.x, limMinX + 6.f, limMaxX - 6.f);
    gk.tf.pos.y = clampf(gk.tf.pos.y, minY, maxY);

    // Interaction (che bóng)
    float reach = gk.radius + ball.radius + 12.0f;
    float dist2 = (ball.tf.pos - gk.tf.pos).length2();
    bool insideBox = (gk.tf.pos.x >= (leftSide?0.f:(fieldW - boxDepth)) &&
                      gk.tf.pos.x <= (leftSide?boxDepth:fieldW));

    if (dist2 <= reach*reach) {
        bool nearFeet = ((ball.tf.pos - opp.tf.pos).length() <= (opp.radius + ball.radius + 12.0f)) ||
                        (ball.owner == &opp);
        bool blocked  = occludedBy(opp, gk.tf.pos, ball.tf.pos, 6.0f) && nearFeet;

        float v = ball.tf.vel.length();
        if (insideBox && !ball.owner && v <= P.catchSpeed && !blocked) {
            ball.owner = &gk; C.st = Hold; C.hold = 0.f; return;
        }
        // parry lệch hông attacker
        Vec2 nGK = (ball.tf.pos - gk.tf.pos).normalized();
        Vec2 attDir = (ball.tf.pos - opp.tf.pos).normalized();
        Vec2 side(-attDir.y, attDir.x);
        Vec2 outDir = (nGK*0.5f + side*0.8f).normalized();
        float outSp = std::min(P.parrySpeed, std::max(v, 6.0f*40.0f));
        if (!insideBox) outSp = P.parrySpeed;
        ball.tf.vel = outDir * outSp;
    }
}
