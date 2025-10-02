#include "scene/MatchScene.hpp"
#include "ui/HUD.hpp"
#include "systems/DribbleSystem.hpp"
#include "systems/KeeperSystem.hpp"
#include "systems/PossessionSystem.hpp"

#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

static inline float clampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }
static const float PI = 3.14159265358979323846f;

// ==== Subsystems ====
static DribbleSystem gDribble;   // rê theo nhịp (đã có idle-settle trong DribbleSystem.cpp mới)
static KeeperSystem  gKeeper;    // AI thủ môn

MatchScene::~MatchScene(){
    if (pitchTex)   { SDL_DestroyTexture(pitchTex);   pitchTex   = nullptr; }
    if (ballTex)    { SDL_DestroyTexture(ballTex);    ballTex    = nullptr; }
    if (p1Tex)      { SDL_DestroyTexture(p1Tex);      p1Tex      = nullptr; }
    if (p2Tex)      { SDL_DestroyTexture(p2Tex);      p2Tex      = nullptr; }
    if (gkTex)      { SDL_DestroyTexture(gkTex);      gkTex      = nullptr; }
    if (goalSfx)    { Mix_FreeChunk(goalSfx);         goalSfx    = nullptr; }
    if (crowdMusic) { Mix_FreeMusic(crowdMusic);      crowdMusic = nullptr; }
}

void MatchScene::init(const Config& cfg, SDL_Renderer* renderer, HUD* hud_){
    mRenderer = renderer; hud = hud_;

    fieldW = cfg.fieldWidth; fieldH = cfg.fieldHeight; centerY = fieldH * 0.5f;
    halfTimeSeconds = (float)cfg.halfTimeSeconds; kickoffLockTime = cfg.kickoffLockTime;

    // Entities
    ball.id=0; ball.radius=cfg.ballRadius; ball.mass=cfg.ballMass;
    ball.drag=cfg.ballDrag; ball.e_wall=cfg.ballElasticityWall;

    player1.id=1; player1.radius=cfg.playerRadius; player1.mass=cfg.playerMass;
    player1.drag=cfg.playerDrag; player1.e_wall=cfg.playerElasticityWall;
    player1.accel=cfg.playerAccel; player1.vmax=cfg.playerMaxSpeed;

    player2.id=2; player2.radius=cfg.playerRadius; player2.mass=cfg.playerMass;
    player2.drag=cfg.playerDrag; player2.e_wall=cfg.playerElasticityWall;
    player2.accel=cfg.playerAccel; player2.vmax=cfg.playerMaxSpeed;

    gk1.id=3; gk1.radius=cfg.gkRadius; gk1.mass=cfg.gkMass; gk1.drag=cfg.gkDrag;
    gk1.e_wall=cfg.gkElasticityWall; gk1.accel=cfg.gkAccel; gk1.vmax=cfg.gkMaxSpeed;

    gk2.id=4; gk2.radius=cfg.gkRadius; gk2.mass=cfg.gkMass; gk2.drag=cfg.gkDrag;
    gk2.e_wall=cfg.gkElasticityWall; gk2.accel=cfg.gkAccel; gk2.vmax=cfg.gkMaxSpeed;

    // Goals & spawns
    goals.init(fieldW, fieldH, 3.0f*40.0f/2.0f, 8.0f);
    initPosBall = Vec2(fieldW*0.5f, centerY);
    initPosP1   = Vec2(fieldW*0.25f, centerY);
    initPosP2   = Vec2(fieldW*0.75f, centerY);
    initPosGK1  = Vec2(cfg.gkFrontOffset + player1.radius, centerY);
    initPosGK2  = Vec2(fieldW - cfg.gkFrontOffset - player2.radius, centerY);

    ball.tf.pos=initPosBall; ball.tf.vel=Vec2(0,0);
    player1.tf.pos=initPosP1; player1.tf.vel=Vec2(0,0); player1.facing=Vec2( 1,0);
    player2.tf.pos=initPosP2; player2.tf.vel=Vec2(0,0); player2.facing=Vec2(-1,0);
    gk1.tf.pos=initPosGK1; gk1.tf.vel=Vec2(0,0); gk1.facing=Vec2( 1,0);
    gk2.tf.pos=initPosGK2; gk2.tf.vel=Vec2(0,0); gk2.facing=Vec2(-1,0);

    currentHalf=1; timeRemaining=halfTimeSeconds;
    state=MatchState::Kickoff; stateTimer=kickoffLockTime;

    // Assets
    pitchTex=IMG_LoadTexture(mRenderer,"assets/images/pitch.png");
    ballTex =IMG_LoadTexture(mRenderer,"assets/images/ball.png");
    p1Tex   =IMG_LoadTexture(mRenderer,"assets/images/player1.png");
    p2Tex   =IMG_LoadTexture(mRenderer,"assets/images/player2.png");
    gkTex   =IMG_LoadTexture(mRenderer,"assets/images/gk.png");
    goalSfx =Mix_LoadWAV("assets/audio/goal.wav");
    crowdMusic=Mix_LoadMUS("assets/audio/crowd_loop.ogg");
    if (crowdMusic){ Mix_VolumeMusic(MIX_MAX_VOLUME/2); Mix_PlayMusic(crowdMusic,-1); }

    // reset subsystems
    gDribble.reset();
    gKeeper.reset();

    // 🎯 Tuning dribble: nhịp chậm hơn, xoay mượt, lực đẩy dịu
    DribbleSystem::Params dp;
    dp.tps         = 6.4f;
    dp.turnRate    = 2.6f;            // ít giật khi đổi hướng
    dp.touchSpeed  = 4.4f * 40.0f;    // lực đẩy mỗi chạm
    dp.maxSpeed    = 5.0f * 40.0f;
    dp.minSpeed    = 1.1f * 40.0f;
    dp.extraLead   = 6.0f;
    dp.loseDistance= 44.0f;
    gDribble.setParams(dp);
}

void MatchScene::update(float dt){
    static float pickupCooldown = 0.f;
    pickupCooldown = std::max(0.0f, pickupCooldown - dt);

    const float boxDepth = fieldW * 0.18f;

    // --- State machine (match flow) ---
    auto resetPositions = [&](){
        ball.owner=nullptr; pickupCooldown=0.f;
        ball.tf.pos=initPosBall; ball.tf.vel=Vec2(0,0);
        player1.tf.pos=initPosP1; player1.tf.vel=Vec2(0,0);
        player2.tf.pos=initPosP2; player2.tf.vel=Vec2(0,0);
        gk1.tf.pos=initPosGK1; gk1.tf.vel=Vec2(0,0);
        gk2.tf.pos=initPosGK2; gk2.tf.vel=Vec2(0,0);
    };

    switch (state){
    case MatchState::Kickoff:
        stateTimer -= dt; if (stateTimer<=0) state=MatchState::Playing;
        return;
    case MatchState::GoalFreeze:
        stateTimer -= dt;
        if (stateTimer<=0){
            if (timeRemaining<=0){
                if (currentHalf<2){ state=MatchState::HalfTimeBreak; stateTimer=2; }
                else              { state=MatchState::FullTime; }
            } else { state=MatchState::Kickoff; stateTimer=1; }
            resetPositions();
        }
        return;
    case MatchState::HalfTimeBreak:
        stateTimer -= dt;
        if (stateTimer<=0){
            currentHalf=2; timeRemaining=halfTimeSeconds;
            state=MatchState::Kickoff; stateTimer=1; resetPositions();
        } return;
    case MatchState::FullTime: return;
    case MatchState::Playing: break;
    }

    // Clock
    timeRemaining -= dt;
    if (timeRemaining<=0){
        timeRemaining=0;
        if (currentHalf<2){ state=MatchState::HalfTimeBreak; stateTimer=2; }
        else              { state=MatchState::FullTime; }
        return;
    }

    // Input & actions
    player1.applyInput(dt); player2.applyInput(dt);
    if (player1.in.shoot) player1.tryShoot(ball);
    if (player1.in.slide) player1.trySlide(ball, dt);
    if (player2.in.shoot) player2.tryShoot(ball);
    if (player2.in.slide) player2.trySlide(ball, dt);

    if (!ball.owner){ player1.assistDribble(ball, dt); player2.assistDribble(ball, dt); }

    // GK AI
    gKeeper.updatePair(ball, gk1, gk2, player1, player2, fieldW, fieldH, centerY, dt, pickupCooldown);

    // Possession rules (nhặt bóng hợp lệ)
    PossessionSystem::tryTakeAll(ball, player1, player2, gk1, gk2, fieldW, boxDepth, pickupCooldown);

    // 🔁 Đặt dribble TRƯỚC physics để tránh kéo-co gây giật
    if (ball.owner==&player1 || ball.owner==&player2) {
        gDribble.update(ball, *ball.owner, dt);
    }

    // Physics
    std::vector<Entity*> ents = { &ball, &player1, &player2, &gk1, &gk2 };
    physics.step(dt, ents, goals, fieldW, fieldH);

    // Goals (chỉ tính khi bóng tự do)
    int gs = (ball.owner==nullptr) ? goals.checkGoal(ball) : 0;
    if (gs!=0){
        if (gs==1) goals.scoreLeft  +=1;
        if (gs==2) goals.scoreRight +=1;
        state=MatchState::GoalFreeze; stateTimer=2.0f;
        ball.owner=nullptr; ball.tf.vel=Vec2(0,0);
        player1.tf.vel=player2.tf.vel=gk1.tf.vel=gk2.tf.vel=Vec2(0,0);
        if (goalSfx) Mix_PlayChannel(-1, goalSfx, 0);
    }
}

void MatchScene::render(SDL_Renderer* renderer, bool paused){
    if (pitchTex) SDL_RenderCopy(renderer, pitchTex, nullptr, nullptr);
    else { SDL_SetRenderDrawColor(renderer,0,100,0,255); SDL_Rect r{0,0,fieldW,fieldH}; SDL_RenderFillRect(renderer,&r); }

    int sw,sh; SDL_GetRendererOutputSize(renderer,&sw,&sh);
    const float sx=(float)sw/(float)fieldW, sy=(float)sh/(float)fieldH;
    auto rectFor=[&](float cx,float cy,float r){ SDL_Rect d; d.x=(int)((cx-r)*sx); d.y=(int)((cy-r)*sy); d.w=(int)((r*2)*sx); d.h=(int)((r*2)*sy); return d; };

    SDL_Rect dst = rectFor(ball.tf.pos.x, ball.tf.pos.y, ball.radius);
    if (ballTex) SDL_RenderCopy(renderer,ballTex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_RenderFillRect(renderer,&dst); }

    dst = rectFor(player1.tf.pos.x, player1.tf.pos.y, player1.radius);
    if (p1Tex) SDL_RenderCopy(renderer,p1Tex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,0,0,255,255); SDL_RenderFillRect(renderer,&dst); }

    dst = rectFor(player2.tf.pos.x, player2.tf.pos.y, player2.radius);
    if (p2Tex) SDL_RenderCopy(renderer,p2Tex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,255,0,0,255); SDL_RenderFillRect(renderer,&dst); }

    dst = rectFor(gk1.tf.pos.x, gk1.tf.pos.y, gk1.radius);
    if (gkTex) SDL_RenderCopy(renderer,gkTex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,100,100,100,255); SDL_RenderFillRect(renderer,&dst); }

    dst = rectFor(gk2.tf.pos.x, gk2.tf.pos.y, gk2.radius);
    if (gkTex) SDL_RenderCopy(renderer,gkTex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,100,100,100,255); SDL_RenderFillRect(renderer,&dst); }

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    auto drawPost=[&](const Post& p){ SDL_Rect pr=rectFor(p.pos.x,p.pos.y,p.radius); SDL_RenderFillRect(renderer,&pr); };
    drawPost(goals.leftPosts[0]); drawPost(goals.leftPosts[1]);
    drawPost(goals.rightPosts[0]); drawPost(goals.rightPosts[1]);

    int m=(int)timeRemaining/60, s=(int)timeRemaining%60;
    char t[6]; std::sprintf(t,"%02d:%02d",m,s);
    char sc[16]; std::sprintf(sc,"%d - %d",goals.scoreLeft,goals.scoreRight);
    const char* banner="";
    if (paused) banner="PAUSED";
    else if (state==MatchState::GoalFreeze) banner="GOAL!";
    else if (state==MatchState::Kickoff && currentHalf==1 && goals.scoreLeft==0 && goals.scoreRight==0) banner="KICK OFF";
    else if (state==MatchState::HalfTimeBreak) banner="HALF TIME";
    else if (state==MatchState::FullTime) banner="FULL TIME";
    if (hud) hud->render(sc,t,banner);
}
