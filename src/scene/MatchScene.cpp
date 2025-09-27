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
    switch (state) {
    case MatchState::Kickoff:
        stateTimer -= dt;
        if (stateTimer <= 0.0f) state = MatchState::Playing;
        return;

    case MatchState::GoalFreeze:
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            if (timeRemaining <= 0.0f) {
                if (currentHalf < 2) {
                    state = MatchState::HalfTimeBreak;
                    stateTimer = 2.0f;
                } else {
                    state = MatchState::FullTime;
                }
            } else {
                state = MatchState::Kickoff;
                stateTimer = 1.0f;
            }
            // Reset vị trí giao bóng
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
            // Bắt đầu hiệp 2
            currentHalf   = 2;
            timeRemaining = halfTimeSeconds;   // reset đúng thời lượng hiệp 2
            state         = MatchState::Kickoff;
            stateTimer    = 1.0f;

            // Reset vị trí giao bóng
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

    // Đồng hồ hiệp
    timeRemaining -= dt;
    if (timeRemaining <= 0.0f) {
        timeRemaining = 0.0f;
        if (currentHalf < 2) {
            state = MatchState::HalfTimeBreak;
            stateTimer = 2.0f;
        } else {
            state = MatchState::FullTime;
        }
        return;
    }

    // AI GK + input người chơi + physics
    gk1.updateAI(ball, centerY, dt);
    gk2.updateAI(ball, centerY, dt);

    player1.applyInput(dt);
    player2.applyInput(dt);

    std::vector<Entity*> entities = { &ball, &player1, &player2, &gk1, &gk2 };
    physics.step(dt, entities, goals, fieldW, fieldH);

    // Check goal
    int goalSide = goals.checkGoal(ball);
    if (goalSide != 0) {
        if (goalSide == 1) goals.scoreLeft  += 1;
        if (goalSide == 2) goals.scoreRight += 1;

        state = MatchState::GoalFreeze;
        stateTimer = 2.0f;

        ball.tf.vel = Vec2(0,0);
        player1.tf.vel = Vec2(0,0);
        player2.tf.vel = Vec2(0,0);
        gk1.tf.vel = Vec2(0,0);
        gk2.tf.vel = Vec2(0,0);

        if (goalSfx) Mix_PlayChannel(-1, goalSfx, 0);
    }
}

void MatchScene::render(SDL_Renderer* renderer, bool paused) {
    // Sân
    if (pitchTex) {
        SDL_RenderCopy(renderer, pitchTex, nullptr, nullptr);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
        SDL_Rect fieldRect = {0, 0, fieldW, fieldH};
        SDL_RenderFillRect(renderer, &fieldRect);
    }

    // Bóng
    SDL_Rect dst{};
    dst.x = static_cast<int>(ball.tf.pos.x - ball.radius);
    dst.y = static_cast<int>(ball.tf.pos.y - ball.radius);
    dst.w = dst.h = static_cast<int>(ball.radius * 2);
    if (ballTex) {
        SDL_RenderCopy(renderer, ballTex, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &dst);
    }

    // P1
    dst.x = static_cast<int>(player1.tf.pos.x - player1.radius);
    dst.y = static_cast<int>(player1.tf.pos.y - player1.radius);
    dst.w = dst.h = static_cast<int>(player1.radius * 2);
    if (p1Tex) {
        SDL_RenderCopy(renderer, p1Tex, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &dst);
    }

    // P2
    dst.x = static_cast<int>(player2.tf.pos.x - player2.radius);
    dst.y = static_cast<int>(player2.tf.pos.y - player2.radius);
    dst.w = dst.h = static_cast<int>(player2.radius * 2);
    if (p2Tex) {
        SDL_RenderCopy(renderer, p2Tex, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &dst);
    }

    // GK
    // GK1
    dst.x = static_cast<int>(gk1.tf.pos.x - gk1.radius);
    dst.y = static_cast<int>(gk1.tf.pos.y - gk1.radius);
    dst.w = dst.h = static_cast<int>(gk1.radius * 2);
    if (gkTex) {
        SDL_RenderCopy(renderer, gkTex, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &dst);
    }
    // GK2
    dst.x = static_cast<int>(gk2.tf.pos.x - gk2.radius);
    dst.y = static_cast<int>(gk2.tf.pos.y - gk2.radius);
    dst.w = dst.h = static_cast<int>(gk2.radius * 2);
    if (gkTex) {
        SDL_RenderCopy(renderer, gkTex, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &dst);
    }

    // Cột gôn (trụ)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 2; ++i) {
        SDL_Rect postRect;
        postRect.x = static_cast<int>(goals.leftPosts[i].pos.x - goals.leftPosts[i].radius);
        postRect.y = static_cast<int>(goals.leftPosts[i].pos.y - goals.leftPosts[i].radius);
        postRect.w = postRect.h = static_cast<int>(goals.leftPosts[i].radius * 2);
        SDL_RenderFillRect(renderer, &postRect);

        postRect.x = static_cast<int>(goals.rightPosts[i].pos.x - goals.rightPosts[i].radius);
        postRect.y = static_cast<int>(goals.rightPosts[i].pos.y - goals.rightPosts[i].radius);
        postRect.w = postRect.h = static_cast<int>(goals.rightPosts[i].radius * 2);
        SDL_RenderFillRect(renderer, &postRect);
    }

    // HUD strings
    int minutes = static_cast<int>(timeRemaining) / 60;
    int seconds = static_cast<int>(timeRemaining) % 60;
    char timeStr[6];
    std::sprintf(timeStr, "%02d:%02d", minutes, seconds);

    char scoreStr[16];
    std::sprintf(scoreStr, "%d - %d", goals.scoreLeft, goals.scoreRight);

    const char* bannerText = "";
    if (paused) {
        bannerText = "PAUSED";
    } else if (state == MatchState::GoalFreeze) {
        bannerText = "GOAL!";
    } else if (state == MatchState::Kickoff && currentHalf == 1
               && goals.scoreLeft == 0 && goals.scoreRight == 0) {
        bannerText = "KICK OFF";
    } else if (state == MatchState::HalfTimeBreak) {
        bannerText = "HALF TIME";
    } else if (state == MatchState::FullTime) {
        bannerText = "FULL TIME";
    }

    // Vẽ HUD
    if (hud) {
        hud->render(scoreStr, timeStr, bannerText);
    }
}
