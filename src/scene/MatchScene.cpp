#include "scene/MatchScene.hpp"
#include "ui/HUD.hpp"

// Subsystems còn dùng
#include "systems/KeeperSystem.hpp"
#include "systems/PossessionSystem.hpp"

#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <algorithm>   // std::max, std::min
#include "ui/Animation.hpp"

static inline float clampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }
static const float PI = 3.14159265358979323846f;

// ===== Subsystem singletons =====
static KeeperSystem gKeeper;

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

    player1.isGoalkeeper = false;
    player2.isGoalkeeper = false;
    gk1.isGoalkeeper = true;
    gk2.isGoalkeeper = true;

    // mặc định 2 cầu thủ thường được điều khiển
    player1.isControlled = true;
    player2.isControlled = true;
    gk1.isControlled = false;
    gk2.isControlled = false;


    // Goals & spawns
    goals.init(fieldW, fieldH, 9.0f*40.0f/3.0f, 8.0f);
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

    // --- Assets ---
    pitchTex = IMG_LoadTexture(mRenderer, "assets/images/pitch2.png");
    ballTex  = IMG_LoadTexture(mRenderer, "assets/images/ball.png");
    gkTex    = IMG_LoadTexture(mRenderer, "assets/images/gk.png");


    auto loadAnim = [&](Animation& anim, std::vector<std::string> files){
    for (auto& f : files) {
        SDL_Texture* tex = IMG_LoadTexture(mRenderer, f.c_str());
        if (tex) anim.frames.push_back(tex);
    }
};

    // GK1 idle
    // loadAnim(gk1.idle[0], {"assets/images/player1/idle/idle_down.png"});
    // loadAnim(gk1.idle[1], {"assets/images/player1/idle/idle_left.png"});
    loadAnim(gk1.idle[0], {"assets/images/player1/idle/idle_right.png"});
    // loadAnim(gk1.idle[3], {"assets/images/player1/idle/idle_up.png"});
    // GK2 run
    // loadAnim(gk1.run[0], {"assets/images/player1/run/run_down_1.png", "assets/images/player1/run/run_down_2.png"});
    // loadAnim(gk1.run[1], {"assets/images/player1/run/run_left_1.png",
    // "assets/images/player1/idle/idle_left.png"});
    // loadAnim(gk1.run[2], {"assets/images/player1/run/run_right_1.png",
    // "assets/images/player1/idle/idle_right.png"});
    // loadAnim(gk1.run[3], {"assets/images/player1/run/run_up_1.png", "assets/images/player1/run/run_up_2.png"});


    // GK2 idle
    // loadAnim(gk2.idle[0], {"assets/images/player2/idle/idle_down.png"});
    loadAnim(gk2.idle[0], {"assets/images/player2/idle/idle_left.png"});
    // loadAnim(gk2.idle[2], {"assets/images/player2/idle/idle_right.png"});
    // loadAnim(gk2.idle[3], {"assets/images/player2/idle/idle_up.png"});

    // GK2 run
    // loadAnim(gk2.run[0], {"assets/images/player2/run/run_down_1.png", "assets/images/player2/run/run_down_2.png"});
    // loadAnim(gk2.run[1], {"assets/images/player2/run/run_left_1.png",
    // "assets/images/player2/idle/idle_left.png"});
    // loadAnim(gk2.run[2], {"assets/images/player2/run/run_right_1.png",
    // "assets/images/player2/idle/idle_right.png"});
    // loadAnim(gk2.run[3], {"assets/images/player2/run/run_up_1.png", "assets/images/player2/run/run_up_2.png"});

    // Player1 idle
    loadAnim(player1.idle[0], {"assets/images/player1/idle/idle_down.png"});
    loadAnim(player1.idle[1], {"assets/images/player1/idle/idle_left.png"});
    loadAnim(player1.idle[2], {"assets/images/player1/idle/idle_right.png"});
    loadAnim(player1.idle[3], {"assets/images/player1/idle/idle_up.png"});
    // Player1 run (2–3 frames mỗi hướng)
    loadAnim(player1.run[0], {"assets/images/player1/run/run_down_1.png", "assets/images/player1/run/run_down_2.png"});
    loadAnim(player1.run[1], {"assets/images/player1/run/run_left_1.png",
    "assets/images/player1/idle/idle_left.png"});
    loadAnim(player1.run[2], {"assets/images/player1/run/run_right_1.png",
    "assets/images/player1/idle/idle_right.png"});
    loadAnim(player1.run[3], {"assets/images/player1/run/run_up_1.png", "assets/images/player1/run/run_up_2.png"});


    // Player2 idle
    loadAnim(player2.idle[0], {"assets/images/player2/idle/idle_down.png"});
    loadAnim(player2.idle[1], {"assets/images/player2/idle/idle_left.png"});
    loadAnim(player2.idle[2], {"assets/images/player2/idle/idle_right.png"});
    loadAnim(player2.idle[3], {"assets/images/player2/idle/idle_up.png"});

    // Player2 run (2–3 frames mỗi hướng)
    loadAnim(player2.run[0], {"assets/images/player2/run/run_down_1.png", "assets/images/player2/run/run_down_2.png"});
    loadAnim(player2.run[1], {"assets/images/player2/run/run_left_1.png",
    "assets/images/player2/idle/idle_left.png"});
    loadAnim(player2.run[2], {"assets/images/player2/run/run_right_1.png",
    "assets/images/player2/idle/idle_right.png"});
    loadAnim(player2.run[3], {"assets/images/player2/run/run_up_1.png", "assets/images/player2/run/run_up_2.png"});

    goalSfx  = Mix_LoadWAV("assets/audio/goal.wav");
    crowdMusic = Mix_LoadMUS("assets/audio/crowd_loop.ogg");
    if (crowdMusic) {
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
        Mix_PlayMusic(crowdMusic, -1);
    }

    gKeeper.reset();
}

void MatchScene::update(float dt){
    // cooldown chống nhặt lại ngay sau khi vừa sút/phát bóng
    static float pickupCooldown = 0.f;
    pickupCooldown = std::max(0.0f, pickupCooldown - dt);

    // “vòng cấm” cho KeeperSystem & possession
    const float boxDepth = fieldW * 0.18f;

    // giảm timer dấu vết sút của bóng (nếu Ball có biến này)
    if (ball.justKicked > 0.0f) ball.justKicked = std::max(0.0f, ball.justKicked - dt);

    // --- State machine ---
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
    player1.applyInput(dt);
    player1.updateAnim(dt);
    player2.applyInput(dt);
    player2.updateAnim(dt);   


    // Sau player1.applyInput(dt), player2.applyInput(dt)

    if (player1.in.switchGK) {
        player1.isControlled = !player1.isControlled;
        gk1.isControlled     = !gk1.isControlled;
    }

    if (player2.in.switchGK) {
        player2.isControlled = !player2.isControlled;
        gk2.isControlled     = !gk2.isControlled;
    }
        

    bool shot1 = false, shot2 = false;
    if (player1.in.shoot) shot1 = player1.tryShoot(ball);
    if (player2.in.shoot) shot2 = player2.tryShoot(ball);
    if (shot1 || shot2) pickupCooldown = std::max(pickupCooldown, 0.22f);

    // Slide
    if (player1.in.slide) player1.trySlide(ball, dt);
    if (player2.in.slide) player2.trySlide(ball, dt);


    // === GK CONTROL ===
    static float gk1Hold = 0.0f, gk2Hold = 0.0f;

    // Nếu người chơi đang điều khiển GK1
    if (gk1.isControlled) {
        gk1.applyInput(dt);

        if (gk1.in.shoot) {
            // Nếu GK đang giữ bóng → phất lên
            if (ball.owner == &gk1) {
                PossessionSystem::updateKeeperBallLogic(ball, gk1, gk1Hold, dt);
            } else {
                gk1.tryShoot(ball); // đá phá
            }
        }
        if (gk1.in.slide) gk1.trySlide(ball, dt);

        gk1.updateAnim(dt);
    }

    // Nếu người chơi đang điều khiển GK2
    if (gk2.isControlled) {
        gk2.applyInput(dt);

        if (gk2.in.shoot) {
            if (ball.owner == &gk2) {
                PossessionSystem::updateKeeperBallLogic(ball, gk2, gk2Hold, dt);
            } else {
                gk2.tryShoot(ball);
            }
        }
        if (gk2.in.slide) gk2.trySlide(ball, dt);

        gk2.updateAnim(dt);
    }

    // Nếu GK đang giữ bóng nhưng người chơi KHÔNG điều khiển → vẫn auto phất sau 6s
    PossessionSystem::updateKeeperBallLogic(ball, gk1, gk1Hold, dt);
    PossessionSystem::updateKeeperBallLogic(ball, gk2, gk2Hold, dt);



        // === DRIBBLE ASSIST (TRƯỚC PHYSICS) ===
    // === DRIBBLE ASSIST (TRƯỚC PHYSICS) ===
    // Nếu có owner thì chỉ update cho owner
    if (ball.owner == &player1) {
        player1.assistDribble(ball, dt);
    }
    else if (ball.owner == &player2) {
        player2.assistDribble(ball, dt);
    }
    else {
        // Bóng tự do → cho cả 2 thử “hỗ trợ kéo bóng”
        player1.assistDribble(ball, dt);
        player2.assistDribble(ball, dt);
    }

    // (GK không rê)

    // === KEEPER AI ===
    gKeeper.updatePair(ball, gk1, gk2, player1, player2,
                       fieldW, fieldH, centerY, dt, pickupCooldown);

    // === POSSESSION RULES ===
    PossessionSystem::tryTakeAll(ball, player1, player2, gk1, gk2,
                                 fieldW, boxDepth, pickupCooldown, dt);

    // === PHYSICS ===
    // Khi đang sở hữu, bỏ bóng khỏi physics để tránh “kéo co” với hệ rê
    std::vector<Entity*> ents;
    ents.reserve(5);
    if (ball.owner == nullptr) ents.push_back(&ball);
    ents.push_back(&player1); ents.push_back(&player2);
    ents.push_back(&gk1);     ents.push_back(&gk2);

    physics.step(dt, ents, goals, fieldW, fieldH);

    // === GOALS === (chỉ tính khi bóng tự do)
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

    // Ball
    SDL_Rect dst = rectFor(ball.tf.pos.x, ball.tf.pos.y, ball.radius);
    if (ballTex) SDL_RenderCopy(renderer,ballTex,nullptr,&dst); else { SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_RenderFillRect(renderer,&dst); }

    // Players
    dst = rectFor(player1.tf.pos.x, player1.tf.pos.y, player1.radius);
    // Xác định đang idle hay chạy
    bool moving = (fabs(player1.tf.vel.x) > 1 || fabs(player1.tf.vel.y) > 1);
    // Lấy texture frame tương ứng
    SDL_Texture* tex = moving 
        ? player1.run[player1.dir].getFrame()
        : player1.idle[player1.dir].getFrame();
    // Vẽ
    if (tex) {
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
    }

    dst = rectFor(player2.tf.pos.x, player2.tf.pos.y, player2.radius);
    bool moving2 = (fabs(player2.tf.vel.x) > 1 || fabs(player2.tf.vel.y) > 1);
    SDL_Texture* tex2 = moving2
        ? player2.run[player2.dir].getFrame()
        : player2.idle[player2.dir].getFrame();
    if (tex2) {
        SDL_RenderCopy(renderer, tex2, nullptr, &dst);}

    // GKs
    dst = rectFor(gk1.tf.pos.x, gk1.tf.pos.y, gk1.radius);
    SDL_Texture* texgk1 = gk1.idle[0].getFrame();
    if (texgk1) SDL_RenderCopy(renderer, texgk1, nullptr, &dst);

    dst = rectFor(gk2.tf.pos.x, gk2.tf.pos.y, gk2.radius);
    SDL_Texture* texgk2 = gk2.idle[0].getFrame();
    if (texgk2) SDL_RenderCopy(renderer, texgk2, nullptr, &dst);
    
    // Goal posts
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    auto drawPost=[&](const Post& p){ SDL_Rect pr=rectFor(p.pos.x,p.pos.y,p.radius); SDL_RenderFillRect(renderer,&pr); };
    drawPost(goals.leftPosts[0]); drawPost(goals.leftPosts[1]);
    drawPost(goals.rightPosts[0]); drawPost(goals.rightPosts[1]);

    // HUD
    int m=(int)timeRemaining/60, s=(int)timeRemaining%60;
    char t[6];  std::sprintf(t,"%02d:%02d",m,s);
    char sc[16]; std::sprintf(sc,"%d - %d",goals.scoreLeft,goals.scoreRight);
    const char* banner="";
    if (paused) banner="PAUSED";
    else if (state==MatchState::GoalFreeze) banner="GOAL!";
    else if (state==MatchState::Kickoff && currentHalf==1 && goals.scoreLeft==0 && goals.scoreRight==0) banner="KICK OFF";
    else if (state==MatchState::HalfTimeBreak) banner="HALF TIME";
    else if (state==MatchState::FullTime) banner="FULL TIME";
    if (hud) hud->render(sc,t,banner);
}
