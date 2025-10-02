#pragma once
#include "ecs/Player.hpp"
#include "ecs/Ball.hpp"

class KeeperSystem {
public:
    struct Params {
        float boxDepthRatio   = 0.18f;
        float turnRate        = 6.0f;       // rad/s
        float walkFactor      = 0.55f;
        float rushFactor      = 0.95f;
        float chargeExtendW   = 0.10f;      // % bề ngang sân khi Charge
        float catchSpeed      = 7.0f * 40.0f;
        float parrySpeed      = 9.5f * 40.0f;
        float clearSpeed      = 10.3f * 40.0f;
        float maxHold         = 2.5f;
        float pickupCooldown  = 0.25f;
    };

    KeeperSystem() : P{} {}                   // ✅ mặc định
    explicit KeeperSystem(const Params& p)    // ✅ nhận params
        : P(p) {}

    void reset() { ctx1 = Ctx{}; ctx2 = Ctx{}; }

    void updatePair(Ball& ball,
                    Player& gk1, Player& gk2,
                    Player& p1,  Player& p2,
                    float fieldW, float fieldH, float centerY, float dt,
                    float& pickupCooldown);

private:
    enum GKState { Set, Charge, Hold };
    struct Ctx { GKState st=Set; float stTime=0.f; float hold=0.f; };

    Params P;
    Ctx ctx1, ctx2;

    static bool  occludedBy(const Player& attacker, const Vec2& gkPos, const Vec2& ballPos, float margin);
    static float clampf(float v, float lo, float hi);
    static Vec2  rotateTowards(const Vec2& a, const Vec2& b, float maxRad);

    void updateOne(Ball& ball, Player& gk, Player& mate, Player& opp, bool leftSide,
                   bool activeSide, Ctx& C, float fieldW, float fieldH, float centerY,
                   float boxDepth, float dt, float& pickupCooldown);
};
