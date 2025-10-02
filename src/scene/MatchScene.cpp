#include "scene/MatchScene.hpp"
#include "ui/HUD.hpp"
#include <SDL_image.h>
#include <cmath>
#include <cstdio>

MatchScene::~MatchScene() {
    if (pitchTex) { SDL_DestroyTexture(pitchTex); pitchTex = nullptr; }
    if (ballTex)  { SDL_DestroyTexture(ballTex);  ballTex  = nullptr; }
    if (p1Tex)    { SDL_DestroyTexture(p1Tex);    p1Tex    = nullptr; }
    if (p2Tex)    { SDL_DestroyTexture(p2Tex);    p2Tex    = nullptr; }
    if (gkTex)    { SDL_DestroyTexture(gkTex);    gkTex    = nullptr; }
    if (goalSfx)  { Mix_FreeChunk(goalSfx);       goalSfx  = nullptr; }
    if (crowdMusic) { Mix_FreeMusic(crowdMusic);  crowdMusic = nullptr; }
}

void MatchScene::init(const Config& config, SDL_Renderer* renderer, HUD* hud_) {
    mRenderer = renderer;
    hud = hud_;

    fieldW = config.fieldWidth;
    fieldH = config.fieldHeight;
    centerY = fieldH * 0.5f;

    halfTimeSeconds = static_cast<float>(config.halfTimeSeconds);
    kickoffLockTime = config.kickoffLockTime;

    // --- Khởi tạo thực thể theo config ---
    ball.id = 0;
    ball.radius = config.ballRadius;
    ball.mass = config.ballMass;
    ball.drag = config.ballDrag;
    ball.e_wall = config.ballElasticityWall;

    player1.id = 1;
    player1.radius = config.playerRadius;
    player1.mass = config.playerMass;
    player1.drag = config.playerDrag;
    player1.e_wall = config.playerElasticityWall;
    player1.accel = config.playerAccel;
    player1.vmax  = config.playerMaxSpeed;

    player2.id = 2;
    player2.radius = config.playerRadius;
    player2.mass   = config.playerMass;
    player2.drag   = config.playerDrag;
    player2.e_wall = config.playerElasticityWall;
    player2.accel  = config.playerAccel;
    player2.vmax   = config.playerMaxSpeed;

    gk1.id = 3;
    gk1.radius = config.gkRadius;
    gk1.mass   = config.gkMass;
    gk1.drag   = config.gkDrag;
    gk1.e_wall = config.gkElasticityWall;
    gk1.accel  = config.gkAccel;
    gk1.vmax   = config.gkMaxSpeed;

    gk2.id = 4;
    gk2.radius = config.gkRadius;
    gk2.mass   = config.gkMass;
    gk2.drag   = config.gkDrag;
    gk2.e_wall = config.gkElasticityWall;
    gk2.accel  = config.gkAccel;
    gk2.vmax   = config.gkMaxSpeed;

    // --- Khung thành ---
    float goalHalfHeight = 3.0f * 40.0f / 2.0f; // 3m ~ 120px → half = 60px (tùy scale bạn)
    float postRadius = 8.0f;
    goals.init(fieldW, fieldH, goalHalfHeight, postRadius);

    // --- Vị trí xuất phát ---
    initPosBall = Vec2(fieldW * 0.5f, centerY);
    initPosP1   = Vec2(fieldW * 0.25f, centerY);
    initPosP2   = Vec2(fieldW * 0.75f, centerY);
    initPosGK1  = Vec2(config.gkFrontOffset + player1.radius, centerY);
    initPosGK2  = Vec2(fieldW - config.gkFrontOffset - player2.radius, centerY);

    ball.tf.pos = initPosBall; ball.tf.vel = Vec2(0,0);
    player1.tf.pos = initPosP1; player1.tf.vel = Vec2(0,0);
    player2.tf.pos = initPosP2; player2.tf.vel = Vec2(0,0);
    gk1.tf.pos = initPosGK1; gk1.tf.vel = Vec2(0,0);
    gk2.tf.pos = initPosGK2; gk2.tf.vel = Vec2(0,0);

    // --- Trạng thái bắt đầu ---
    currentHalf   = 1;
    timeRemaining = halfTimeSeconds;
    state         = MatchState::Kickoff;
    stateTimer    = kickoffLockTime;

    // --- Load asset 1 lần ---
    pitchTex = IMG_LoadTexture(mRenderer, "assets/images/pitch.png");
    ballTex  = IMG_LoadTexture(mRenderer, "assets/images/ball.png");
    p1Tex    = IMG_LoadTexture(mRenderer, "assets/images/player1.png");
    p2Tex    = IMG_LoadTexture(mRenderer, "assets/images/player2.png");
    gkTex    = IMG_LoadTexture(mRenderer, "assets/images/gk.png");

    goalSfx  = Mix_LoadWAV("assets/audio/goal.wav");

    // Nhạc nền khán giả (loop)
    crowdMusic = Mix_LoadMUS("assets/audio/crowd_loop.ogg");
    if (crowdMusic) {
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
        Mix_PlayMusic(crowdMusic, -1);
    }
}

void MatchScene::update(float dt) {
    // --- timers giữ bóng của GK + cooldown nhặt bóng lại ngay sau khi phát ---
    static float gk1Hold = 0.0f, gk2Hold = 0.0f;
    static float pickupCooldown = 0.0f;
    const float KEEPER_MAX_HOLD = 2.5f;    // GK giữ tối đa 2.5s
    const float PICKUP_COOLDOWN = 0.25f;   // tránh “nhặt lại” ngay

    pickupCooldown = std::max(0.0f, pickupCooldown - dt);

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
                else { state = MatchState::FullTime; }
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

    // Đồng hồ
    timeRemaining -= dt;
    if (timeRemaining <= 0.0f) {
        timeRemaining = 0.0f;
        if (currentHalf < 2) { state = MatchState::HalfTimeBreak; stateTimer = 2.0f; }
        else { state = MatchState::FullTime; }
        return;
    }

    // AI GK (bám Y), rồi input
    gk1.updateAI(ball, centerY, dt);
    gk2.updateAI(ball, centerY, dt);
    player1.applyInput(dt);
    player2.applyInput(dt);

    // Hành vi theo input
    if (player1.in.shoot) player1.tryShoot(ball);
    if (player1.in.slide) player1.trySlide(ball, dt);
    if (player2.in.shoot) player2.tryShoot(ball);
    if (player2.in.slide) player2.trySlide(ball, dt);

    player1.assistDribble(ball, dt);
    player2.assistDribble(ball, dt);
    // ❌ KHÔNG gọi cho gk1/gk2


    // --- POSSESSION RULES ---
    auto inKeeperZone = [&](const Player* p)->bool {
        float zone = fieldW * 0.14f; // ~14% bề ngang sân
        if (p == &gk1) return ball.tf.pos.x < zone;
        if (p == &gk2) return ball.tf.pos.x > fieldW - zone;
        return true; // cầu thủ thường không bị hạn chế
    };
    auto tryTake = [&](Player* p){
        if (ball.owner || pickupCooldown > 0.0f) return;
        Vec2 toBall = ball.tf.pos - p->tf.pos;
        float d = toBall.length(); if (d < 1e-3f) return;
        Vec2 fwd = p->facing.normalized();
        float cosA = Vec2::dot(toBall * (1.0f / d), fwd);
        float captureRange = p->radius + ball.radius + 12.0f;
        float maxBallSpeed = 3.0f * 40.0f;

        bool isKeeper = (p == &gk1 || p == &gk2);
        if (isKeeper && !inKeeperZone(p)) return; // GK chỉ bắt trong khu của mình

        if (cosA > std::cos(60.0f * M_PI/180.0f) && d < captureRange &&
            ball.tf.vel.length() < maxBallSpeed) {
            ball.owner = p;
            if (p == &gk1) { gk1Hold = 0.0f; } 
            else if (p == &gk2) { gk2Hold = 0.0f; }
        }
    };

    tryTake(&player1);
    tryTake(&player2);
    tryTake(&gk1);
    tryTake(&gk2);

    // Vật lý
    std::vector<Entity*> entities = { &ball, &player1, &player2, &gk1, &gk2 };
    physics.step(dt, entities, goals, fieldW, fieldH);

    // Khóa GK trục X (đứng “trước khung”)
    gk1.tf.pos.x = initPosGK1.x;
    gk2.tf.pos.x = initPosGK2.x;

    // --- Ball follow logic ---
    if (ball.owner == &player1 || ball.owner == &player2) {
        // Cầu thủ: spring-damper để bám chậm, có quán tính (rê mềm)
        Player* h = ball.owner;
        float lead = h->radius + ball.radius + 6.0f;
        Vec2  fwd  = h->facing.normalized();
        Vec2  target = h->tf.pos + fwd * lead;

        const float K = 18.0f;   // độ cứng
        const float D = 6.5f;    // giảm chấn
        Vec2 delta = target - ball.tf.pos;
        Vec2 acc   = delta * K - ball.tf.vel * D;
        ball.tf.vel += acc * dt;

        const float MAX_V = 5.5f * 40.0f; // tốc độ tối đa khi rê
        float sp = ball.tf.vel.length();
        if (sp > MAX_V) ball.tf.vel = ball.tf.vel * (MAX_V / sp);

    } else if (ball.owner == &gk1 || ball.owner == &gk2) {
        // GK: KHÔNG rê. Ôm bóng đứng y, giữ tối đa 2.5s rồi phát.
        Player* k = ball.owner;
        float& hold = (k == &gk1) ? gk1Hold : gk2Hold;
        hold += dt;

        // giữ bóng ngay trước người, không di chuyển bóng theo vận tốc
        Vec2 fwd = k->facing.normalized();
        float holdDist = k->radius + ball.radius + 4.0f;
        ball.tf.pos = k->tf.pos + fwd * holdDist;
        ball.tf.vel = Vec2(0,0);

        // nếu có áp lực gần (đối thủ áp sát), phát ngay
        float pressDist = 60.0f;
        bool pressured = (k == &gk1)
            ? ((player2.tf.pos - k->tf.pos).length() < pressDist)
            : ((player1.tf.pos - k->tf.pos).length() < pressDist);

        if (hold >= KEEPER_MAX_HOLD || pressured) {
            // Hướng phát bóng: về phía nửa sân đối diện, hơi hướng tâm
            Vec2 target;
            if (k == &gk1) target = Vec2(fieldW * 0.75f, centerY);
            else           target = Vec2(fieldW * 0.25f, centerY);

            Vec2 dir = (target - k->tf.pos).normalized();
            float clearSpeed = 11.0f * 40.0f;    // ~11 m/s phát bóng
            ball.owner = nullptr;
            ball.tf.vel = dir * clearSpeed;

            hold = 0.0f;
            pickupCooldown = PICKUP_COOLDOWN;   // tránh cầm lại ngay
        }
    }

    // Ghi bàn
    int goalSide = goals.checkGoal(ball);
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
    // Vẽ nền sân (texture sẽ auto fit theo kích thước renderer)
    if (pitchTex) SDL_RenderCopy(renderer, pitchTex, nullptr, nullptr);
    else { SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); SDL_Rect fieldRect = {0,0,fieldW,fieldH}; SDL_RenderFillRect(renderer, &fieldRect); }

    // --- Thêm scale từ thế giới -> màn hình ---
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

    // Bóng
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

    // Cột gôn (scale vị trí + bán kính)
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
    char timeStr[6]; std::sprintf(timeStr, "%02d:%02d", minutes, seconds);
    char scoreStr[16]; std::sprintf(scoreStr, "%d - %d", goals.scoreLeft, goals.scoreRight);
    const char* bannerText = "";
    if (paused) bannerText = "PAUSED";
    else if (state == MatchState::GoalFreeze) bannerText = "GOAL!";
    else if (state == MatchState::Kickoff && currentHalf == 1 && goals.scoreLeft == 0 && goals.scoreRight == 0) bannerText = "KICK OFF";
    else if (state == MatchState::HalfTimeBreak) bannerText = "HALF TIME";
    else if (state == MatchState::FullTime) bannerText = "FULL TIME";
    if (hud) hud->render(scoreStr, timeStr, bannerText);
}

