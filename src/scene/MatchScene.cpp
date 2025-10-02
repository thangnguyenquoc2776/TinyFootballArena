#include "scene/MatchScene.hpp"
#include "ui/HUD.hpp"
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <algorithm>   // std::max, std::min

// ===== Helpers =====
static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi ? hi : v);
}
static const float PI = 3.14159265358979323846f;

// Xoay vector a về b với tốc độ góc tối đa maxRad (rad per call)
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

// ===== MatchScene =====
MatchScene::~MatchScene() {
    if (pitchTex)   { SDL_DestroyTexture(pitchTex);   pitchTex   = nullptr; }
    if (ballTex)    { SDL_DestroyTexture(ballTex);    ballTex    = nullptr; }
    if (p1Tex)      { SDL_DestroyTexture(p1Tex);      p1Tex      = nullptr; }
    if (p2Tex)      { SDL_DestroyTexture(p2Tex);      p2Tex      = nullptr; }
    if (gkTex)      { SDL_DestroyTexture(gkTex);      gkTex      = nullptr; }
    if (goalSfx)    { Mix_FreeChunk(goalSfx);         goalSfx    = nullptr; }
    if (crowdMusic) { Mix_FreeMusic(crowdMusic);      crowdMusic = nullptr; }
}

void MatchScene::init(const Config& config, SDL_Renderer* renderer, HUD* hud_) {
    mRenderer = renderer;
    hud = hud_;

    fieldW = config.fieldWidth;
    fieldH = config.fieldHeight;
    centerY = fieldH * 0.5f;

    halfTimeSeconds = static_cast<float>(config.halfTimeSeconds);
    kickoffLockTime = config.kickoffLockTime;

    // --- Entities from config ---
    ball.id     = 0;
    ball.radius = config.ballRadius;
    ball.mass   = config.ballMass;
    ball.drag   = config.ballDrag;
    ball.e_wall = config.ballElasticityWall;

    player1.id     = 1;
    player1.radius = config.playerRadius;
    player1.mass   = config.playerMass;
    player1.drag   = config.playerDrag;
    player1.e_wall = config.playerElasticityWall;
    player1.accel  = config.playerAccel;
    player1.vmax   = config.playerMaxSpeed;

    player2.id     = 2;
    player2.radius = config.playerRadius;
    player2.mass   = config.playerMass;
    player2.drag   = config.playerDrag;
    player2.e_wall = config.playerElasticityWall;
    player2.accel  = config.playerAccel;
    player2.vmax   = config.playerMaxSpeed;

    gk1.id     = 3;
    gk1.radius = config.gkRadius;
    gk1.mass   = config.gkMass;
    gk1.drag   = config.gkDrag;
    gk1.e_wall = config.gkElasticityWall;
    gk1.accel  = config.gkAccel;
    gk1.vmax   = config.gkMaxSpeed;

    gk2.id     = 4;
    gk2.radius = config.gkRadius;
    gk2.mass   = config.gkMass;
    gk2.drag   = config.gkDrag;
    gk2.e_wall = config.gkElasticityWall;
    gk2.accel  = config.gkAccel;
    gk2.vmax   = config.gkMaxSpeed;

    // --- Goals ---
    float goalHalfHeight = 3.0f * 40.0f / 2.0f; // 3 m → 120 px, half = 60
    float postRadius = 8.0f;
    goals.init(fieldW, fieldH, goalHalfHeight, postRadius);

    // --- Initial positions ---
    initPosBall = Vec2(fieldW * 0.5f, centerY);
    initPosP1   = Vec2(fieldW * 0.25f, centerY);
    initPosP2   = Vec2(fieldW * 0.75f, centerY);
    initPosGK1  = Vec2(config.gkFrontOffset + player1.radius, centerY);
    initPosGK2  = Vec2(fieldW - config.gkFrontOffset - player2.radius, centerY);

    ball.tf.pos    = initPosBall; ball.tf.vel    = Vec2(0,0);
    player1.tf.pos = initPosP1;   player1.tf.vel = Vec2(0,0);
    player2.tf.pos = initPosP2;   player2.tf.vel = Vec2(0,0);
    gk1.tf.pos     = initPosGK1;  gk1.tf.vel     = Vec2(0,0);
    gk2.tf.pos     = initPosGK2;  gk2.tf.vel     = Vec2(0,0);

    // Facing
    player1.facing = Vec2( 1.0f, 0.0f);
    player2.facing = Vec2(-1.0f, 0.0f);
    gk1.facing     = Vec2( 1.0f, 0.0f);
    gk2.facing     = Vec2(-1.0f, 0.0f);

    // --- Match state ---
    currentHalf   = 1;
    timeRemaining = halfTimeSeconds;
    state         = MatchState::Kickoff;
    stateTimer    = kickoffLockTime;

    // --- Assets ---
    pitchTex = IMG_LoadTexture(mRenderer, "assets/images/pitch.png");
    ballTex  = IMG_LoadTexture(mRenderer, "assets/images/ball.png");
    p1Tex    = IMG_LoadTexture(mRenderer, "assets/images/player1.png");
    p2Tex    = IMG_LoadTexture(mRenderer, "assets/images/player2.png");
    gkTex    = IMG_LoadTexture(mRenderer, "assets/images/gk.png");

    goalSfx  = Mix_LoadWAV("assets/audio/goal.wav");
    crowdMusic = Mix_LoadMUS("assets/audio/crowd_loop.ogg");
    if (crowdMusic) {
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
        Mix_PlayMusic(crowdMusic, -1);
    }
}

void MatchScene::update(float dt) {
    // --- GK timers / constants ---
    static float gk1Hold = 0.0f, gk2Hold = 0.0f;
    static float pickupCooldown = 0.0f;
    const float KEEPER_MAX_HOLD = 2.5f;
    const float PICKUP_COOLDOWN = 0.25f;

    // Kích thước "vòng cấm" (approx theo bề ngang)
    const float BOX_DEPTH_RATIO = 0.18f;                // sâu ~18% sân
    const float boxDepth = fieldW * BOX_DEPTH_RATIO;
    const float boxMinY = 20.0f, boxMaxY = fieldH - 20.0f;

    pickupCooldown = std::max(0.0f, pickupCooldown - dt);

    // --- State machine ---
    switch (state) {
    case MatchState::Kickoff:
        stateTimer -= dt;
        if (stateTimer <= 0.0f) state = MatchState::Playing;
        ball.owner = nullptr; gk1Hold = gk2Hold = 0.0f; pickupCooldown = 0.0f;
        return;

    case MatchState::GoalFreeze:
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            if (timeRemaining <= 0.0f) {
                if (currentHalf < 2) { state = MatchState::HalfTimeBreak; stateTimer = 2.0f; }
                else                 { state = MatchState::FullTime; }
            } else {
                state = MatchState::Kickoff; stateTimer = 1.0f;
            }
            ball.owner = nullptr; gk1Hold = gk2Hold = 0.0f; pickupCooldown = 0.0f;
            ball.tf.pos = initPosBall; ball.tf.vel = Vec2(0,0);
            player1.tf.pos = initPosP1; player1.tf.vel = Vec2(0,0);
            player2.tf.pos = initPosP2; player2.tf.vel = Vec2(0,0);
            gk1.tf.pos = initPosGK1; gk1.tf.vel = Vec2(0,0);
            gk2.tf.pos = initPosGK2; gk2.tf.vel = Vec2(0,0);
        }
        return;

    case MatchState::HalfTimeBreak:
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            currentHalf = 2;
            timeRemaining = halfTimeSeconds;
            state = MatchState::Kickoff; stateTimer = 1.0f;
            ball.owner = nullptr; gk1Hold = gk2Hold = 0.0f; pickupCooldown = 0.0f;
            ball.tf.pos = initPosBall; ball.tf.vel = Vec2(0,0);
            player1.tf.pos = initPosP1; player1.tf.vel = Vec2(0,0);
            player2.tf.pos = initPosP2; player2.tf.vel = Vec2(0,0);
            gk1.tf.pos = initPosGK1; gk1.tf.vel = Vec2(0,0);
            gk2.tf.pos = initPosGK2; gk2.tf.vel = Vec2(0,0);
        }
        return;

    case MatchState::FullTime:
        return;

    case MatchState::Playing:
        break;
    }

    // --- Clock ---
    timeRemaining -= dt;
    if (timeRemaining <= 0.0f) {
        timeRemaining = 0.0f;
        if (currentHalf < 2) { state = MatchState::HalfTimeBreak; stateTimer = 2.0f; }
        else                 { state = MatchState::FullTime; }
        return;
    }

    // --- Inputs for players ---
    player1.applyInput(dt);
    player2.applyInput(dt);

    if (player1.in.shoot) player1.tryShoot(ball);
    if (player1.in.slide) player1.trySlide(ball, dt);
    if (player2.in.shoot) player2.tryShoot(ball);
    if (player2.in.slide) player2.trySlide(ball, dt);

    // Assist dribble chỉ khi bóng tự do
    if (!ball.owner) { player1.assistDribble(ball, dt); player2.assistDribble(ball, dt); }

    // --- GK Brain (trong vòng cấm, 2 chế độ: khép góc / lao ra 1v1) ---
    auto keeperAI = [&](Player& gk, Player& mate, Player& opp, bool leftSide){
        // Hộp vòng cấm
        float minX = leftSide ? 0.0f : (fieldW - boxDepth);
        float maxX = leftSide ? boxDepth : fieldW;
        float minY = boxMinY, maxY = boxMaxY;

        // Điều kiện 1v1: đối thủ có bóng & đã vượt qua hậu vệ mình & bóng áp sát vòng cấm
        bool oppHasBall   = (ball.owner == &opp);
        bool oppPastMate  = leftSide ? (opp.tf.pos.x < mate.tf.pos.x - 8.0f)
                                     : (opp.tf.pos.x > mate.tf.pos.x + 8.0f);
        bool nearBox      = leftSide ? (ball.tf.pos.x < maxX + 0.4f*boxDepth)
                                     : (ball.tf.pos.x > minX - 0.4f*boxDepth);
        bool chargeNow    = (oppHasBall && oppPastMate && nearBox);

        // Mục tiêu đứng khép góc: điểm trên đoạn thẳng từ tâm cầu môn → bóng (18%)
        Vec2 goalC = leftSide ? Vec2(minX + 12.0f, centerY) : Vec2(maxX - 12.0f, centerY);
        Vec2 cutPoint = goalC + (ball.tf.pos - goalC) * 0.18f;

        // Tốc độ mục tiêu
        float walkSpeed = gk.vmax * 0.55f;       // khép góc: nhẹ nhàng
        float rushSpeed = gk.vmax * 0.95f;       // lao 1v1: nhanh gần tối đa
        float maxTurn   = 5.0f;                  // rad/s xoay mặt

        Vec2 target = chargeNow ? ball.tf.pos : cutPoint;
        float speed = chargeNow ? rushSpeed     : walkSpeed;

        // Hướng nhìn luôn về phía bóng (xoay mượt)
        Vec2 dirToBall = (ball.tf.pos - gk.tf.pos).normalized();
        gk.facing = rotateTowards(gk.facing, dirToBall, maxTurn * dt);

        // Vận tốc mong muốn
        Vec2 toT = target - gk.tf.pos;
        float d = toT.length();
        Vec2 desiredV = (d > 1e-3f) ? (toT * (speed / d)) : Vec2(0,0);

        // Blend để không giật
        gk.tf.vel = gk.tf.vel * 0.80f + desiredV * 0.20f;

        // Nếu đang lao và đủ gần bóng → xoạc/phá
        if (chargeNow) {
            float reach = gk.radius + ball.radius + 12.0f;
            if ((ball.tf.pos - gk.tf.pos).length2() <= reach*reach) {
                // dùng trySlide để hất bóng ra
                gk.trySlide(ball, dt);
            }
        }

        // Clamp GK trong vòng cấm
        gk.tf.pos.x = clampf(gk.tf.pos.x, minX + 6.0f, maxX - 6.0f);
        gk.tf.pos.y = clampf(gk.tf.pos.y, minY, maxY);
    };

    // Gọi AI cho 2 GK
    keeperAI(gk1, player1, player2, /*leftSide=*/true);
    keeperAI(gk2, player2, player1, /*leftSide=*/false);

    // --- POSSESSION RULES (cập nhật cho GK chỉ bắt trong vòng cấm) ---
    auto inKeeperBox = [&](const Player* p)->bool {
        float minX = (p == &gk1) ? 0.0f : (fieldW - boxDepth);
        float maxX = (p == &gk1) ? boxDepth : fieldW;
        return (ball.tf.pos.x >= minX && ball.tf.pos.x <= maxX);
    };
    auto tryTake = [&](Player* p){
        if (ball.owner || pickupCooldown > 0.0f) return;
        Vec2 toBall = ball.tf.pos - p->tf.pos;
        float d = toBall.length(); if (d < 1e-3f) return;
        Vec2 fwd = p->facing.normalized();
        float cosA = Vec2::dot(toBall * (1.0f / d), fwd);

        bool isKeeper = (p == &gk1 || p == &gk2);
        if (isKeeper && !inKeeperBox(p)) return; // GK chỉ bắt trong box

        float captureRange = p->radius + ball.radius + (isKeeper ? 10.0f : 16.0f);
        float maxBallSpeed = isKeeper ? (3.0f * 40.0f) : (6.0f * 40.0f);

        if (cosA > std::cos(60.0f * PI/180.0f) && d < captureRange &&
            ball.tf.vel.length() < maxBallSpeed) {
            ball.owner = p;
            if (p == &gk1)      gk1Hold = 0.0f;
            else if (p == &gk2) gk2Hold = 0.0f;
        }
    };

    tryTake(&player1);
    tryTake(&player2);
    tryTake(&gk1);
    tryTake(&gk2);

    // --- Physics ---
    std::vector<Entity*> entities = { &ball, &player1, &player2, &gk1, &gk2 };
    physics.step(dt, entities, goals, fieldW, fieldH);

    // Re-clamp sau physics (chắc chắn GK không ló khỏi box)
    auto clampKeeper = [&](Player& gk, bool leftSide){
        float minX = leftSide ? 0.0f : (fieldW - boxDepth);
        float maxX = leftSide ? boxDepth : fieldW;
        gk.tf.pos.x = clampf(gk.tf.pos.x, minX + 6.0f, maxX - 6.0f);
        gk.tf.pos.y = clampf(gk.tf.pos.y, boxMinY, boxMaxY);
    };
    clampKeeper(gk1, true);
    clampKeeper(gk2, false);

    // --- Ball follow / Keeper hold ---
    // Dribble của cầu thủ: giữ nguyên như trước (đi cùng, xoay từ từ)
    if (ball.owner == &player1 || ball.owner == &player2) {
        Player* h = ball.owner;
        const float lead = h->radius + ball.radius + 7.0f;
        const float MAX_BALL_TURN = 2.4f;                  // rad/s
        const float MAX_DRIBBLE_V = 4.2f * 40.0f;          // ~4.2 m/s

        Vec2 rel = ball.tf.pos - h->tf.pos;
        float len = rel.length();
        Vec2 curDir = (len < 1e-3f) ? h->facing.normalized() : (rel * (1.0f/len));
        Vec2 newDir = rotateTowards(curDir, h->facing.normalized(), MAX_BALL_TURN * dt);

        Vec2 prevPos = ball.tf.pos;
        Vec2 newPos  = h->tf.pos + newDir * lead;
        ball.tf.pos  = newPos;

        Vec2 v = (newPos - prevPos) * (1.0f / std::max(1e-4f, dt));
        ball.tf.vel = ball.tf.vel * 0.15f + v * 0.85f;

        float sp = ball.tf.vel.length();
        if (sp > MAX_DRIBBLE_V) ball.tf.vel = ball.tf.vel * (MAX_DRIBBLE_V / sp);
    }
    else if (ball.owner == &gk1 || ball.owner == &gk2) {
        // GK ôm: xoay mặt ra sân rồi phất
        Player* k = ball.owner;
        float& hold = (k == &gk1) ? gk1Hold : gk2Hold;
        hold += dt;

        float minX = (k == &gk1) ? 0.0f : (fieldW - boxDepth);
        float maxX = (k == &gk1) ? boxDepth : fieldW;

        Vec2 clearTarget = (k == &gk1) ? Vec2(fieldW * 0.75f, centerY)
                                       : Vec2(fieldW * 0.25f, centerY);
        Vec2 desiredDir = (clearTarget - k->tf.pos).normalized();

        const float GK_TURN_RATE = 5.0f; // rad/s
        k->facing = rotateTowards(k->facing, desiredDir, GK_TURN_RATE * dt);

        Vec2 fwd = k->facing.normalized();
        float holdDist = k->radius + ball.radius + 4.0f;
        ball.tf.pos = k->tf.pos + fwd * holdDist;
        ball.tf.vel = Vec2(0,0);

        // Không cho lọt qua vạch gôn khi đang ôm
        if (k == &gk1 && ball.tf.pos.x - ball.radius < minX + 1.0f)
            ball.tf.pos.x = minX + ball.radius + 1.0f;
        if (k == &gk2 && ball.tf.pos.x + ball.radius > maxX - 1.0f)
            ball.tf.pos.x = maxX - ball.radius - 1.0f;

        float pressDist = 60.0f;
        bool pressured = (k == &gk1)
            ? ((player2.tf.pos - k->tf.pos).length() < pressDist)
            : ((player1.tf.pos - k->tf.pos).length() < pressDist);

        float dot = clampf(Vec2::dot(fwd, desiredDir), -1.0f, 1.0f);
        float ang = std::acos(dot);
        const float READY_ANGLE = 10.0f * PI / 180.0f;

        if (hold >= KEEPER_MAX_HOLD || (pressured && ang < READY_ANGLE) || ang < (6.0f * PI/180.0f)) {
            float clearSpeed = 10.5f * 40.0f; // ~10.5 m/s
            ball.owner = nullptr;
            ball.tf.vel = desiredDir * clearSpeed;
            hold = 0.0f;
            pickupCooldown = PICKUP_COOLDOWN;
        }
    }

    // --- Goals (chỉ tính khi bóng tự do) ---
    int goalSide = (ball.owner == nullptr) ? goals.checkGoal(ball) : 0;
    if (goalSide != 0) {
        if (goalSide == 1) goals.scoreLeft  += 1;
        if (goalSide == 2) goals.scoreRight += 1;
        state = MatchState::GoalFreeze; stateTimer = 2.0f;
        ball.owner = nullptr; gk1Hold = gk2Hold = 0.0f; pickupCooldown = 0.0f;
        ball.tf.vel = Vec2(0,0); player1.tf.vel = Vec2(0,0); player2.tf.vel = Vec2(0,0); gk1.tf.vel = Vec2(0,0); gk2.tf.vel = Vec2(0,0);
        if (goalSfx) Mix_PlayChannel(-1, goalSfx, 0);
    }
}

void MatchScene::render(SDL_Renderer* renderer, bool paused) {
    // Pitch
    if (pitchTex) SDL_RenderCopy(renderer, pitchTex, nullptr, nullptr);
    else {
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
        SDL_Rect fieldRect = {0,0,fieldW,fieldH};
        SDL_RenderFillRect(renderer, &fieldRect);
    }

    // Scale world -> screen
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    const float sx = (float)screenW / (float)fieldW;
    const float sy = (float)screenH / (float)fieldH;

    auto rectFor = [&](float cx, float cy, float r)->SDL_Rect {
        SDL_Rect dst;
        dst.x = (int)((cx - r) * sx);
        dst.y = (int)((cy - r) * sy);
        dst.w = (int)((r * 2.0f) * sx);
        dst.h = (int)((r * 2.0f) * sy);
        return dst;
    };

    // Ball
    SDL_Rect dst = rectFor(ball.tf.pos.x, ball.tf.pos.y, ball.radius);
    if (ballTex) SDL_RenderCopy(renderer, ballTex, nullptr, &dst);
    else { SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderFillRect(renderer, &dst); }

    // P1
    dst = rectFor(player1.tf.pos.x, player1.tf.pos.y, player1.radius);
    if (p1Tex) SDL_RenderCopy(renderer, p1Tex, nullptr, &dst);
    else { SDL_SetRenderDrawColor(renderer, 0,0,255,255); SDL_RenderFillRect(renderer, &dst); }

    // P2
    dst = rectFor(player2.tf.pos.x, player2.tf.pos.y, player2.radius);
    if (p2Tex) SDL_RenderCopy(renderer, p2Tex, nullptr, &dst);
    else { SDL_SetRenderDrawColor(renderer, 255,0,0,255); SDL_RenderFillRect(renderer, &dst); }

    // GK1
    dst = rectFor(gk1.tf.pos.x, gk1.tf.pos.y, gk1.radius);
    if (gkTex) SDL_RenderCopy(renderer, gkTex, nullptr, &dst);
    else { SDL_SetRenderDrawColor(renderer, 100,100,100,255); SDL_RenderFillRect(renderer, &dst); }

    // GK2
    dst = rectFor(gk2.tf.pos.x, gk2.tf.pos.y, gk2.radius);
    if (gkTex) SDL_RenderCopy(renderer, gkTex, nullptr, &dst);
    else { SDL_SetRenderDrawColor(renderer, 100,100,100,255); SDL_RenderFillRect(renderer, &dst); }

    // Goal posts
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    auto drawPost = [&](const Post& p){
        SDL_Rect pr = rectFor(p.pos.x, p.pos.y, p.radius);
        SDL_RenderFillRect(renderer, &pr);
    };
    drawPost(goals.leftPosts[0]);
    drawPost(goals.leftPosts[1]);
    drawPost(goals.rightPosts[0]);
    drawPost(goals.rightPosts[1]);

    // HUD
    int minutes = (int)timeRemaining / 60;
    int seconds = (int)timeRemaining % 60;
    char timeStr[6];  std::sprintf(timeStr, "%02d:%02d", minutes, seconds);
    char scoreStr[16]; std::sprintf(scoreStr, "%d - %d", goals.scoreLeft, goals.scoreRight);
    const char* bannerText = "";
    if (paused) bannerText = "PAUSED";
    else if (state == MatchState::GoalFreeze) bannerText = "GOAL!";
    else if (state == MatchState::Kickoff && currentHalf == 1 && goals.scoreLeft == 0 && goals.scoreRight == 0) bannerText = "KICK OFF";
    else if (state == MatchState::HalfTimeBreak) bannerText = "HALF TIME";
    else if (state == MatchState::FullTime) bannerText = "FULL TIME";
    if (hud) hud->render(scoreStr, timeStr, bannerText);
}
