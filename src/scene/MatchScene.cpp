#include "scene/MatchScene.hpp"
#include "ui/HUD.hpp"
#include <SDL_keyboard.h>
#include <cstdlib>
// Subsystems còn dùng
#include "systems/KeeperSystem.hpp"
#include "scene/systems/PossessionSystem.hpp"

#include <SDL_image.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstdio>
#include <algorithm>   // std::max, std::min
#include "ui/Animation.hpp"

static inline float clampf(float v, float lo, float hi){ return (v<lo)?lo:((v>hi)?hi:v); }
static inline float frand(float a, float b){
    return a + (b - a) * (std::rand() / (float)RAND_MAX);
}

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


    // Lưu drag gốc để scale theo mode
    baseBallDrag = ball.drag;
    baseP1Drag   = player1.drag;
    baseP2Drag   = player2.drag;
    baseGK1Drag  = gk1.drag;
    baseGK2Drag  = gk2.drag;

    // Init wind
    extForces = false; key2Prev = false;
    wind = Vec2(0,0); gustTimer = 0.f; windDirTimer = 0.f;

    // seed random
    std::srand((unsigned)SDL_GetTicks());




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
    // === timers & constants ===
    static float pickupCooldown = 0.f;
    static float gk1Hold = 0.f, gk2Hold = 0.f; // timer giữ bóng của GK
    pickupCooldown = std::max(0.0f, pickupCooldown - dt);
    const float boxDepth = fieldW * 0.18f;
    if (ball.justKicked > 0.0f) ball.justKicked = std::max(0.0f, ball.justKicked - dt);

    // --- State machine (nguyên bản) ---
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

    // ===== EXTERNAL FORCES: toggle '2' (rising-edge) =====
    {
        const Uint8* ks = SDL_GetKeyboardState(NULL);
        bool key2 = ks[SDL_SCANCODE_2] != 0;
        if (key2 && !key2Prev) {
            extForces = !extForces;
            if (extForces) {
                // Lập tức random gió nền + hẹn lần đổi tiếp theo
                float ang = frand(0.f, 2.f*PI);
                float strength = frand(windCfg.baseStrengthMin, windCfg.baseStrengthMax);
                wind = Vec2(std::cos(ang), std::sin(ang)) * strength;

                windDirTimer = frand(windCfg.dirChangeMin, windCfg.dirChangeMax);
                gustTimer    = frand(windCfg.gustIntervalMin, windCfg.gustIntervalMax);
            }
        }
        key2Prev = key2;
    }


    // Scale drag theo mode (ice/slippery nhẹ)
    if (extForces) {
        ball.drag    = baseBallDrag * windCfg.dragScaleBall;
        player1.drag = baseP1Drag   * windCfg.dragScalePlayer;
        player2.drag = baseP2Drag   * windCfg.dragScalePlayer;
        gk1.drag     = baseGK1Drag  * windCfg.dragScalePlayer;
        gk2.drag     = baseGK2Drag  * windCfg.dragScalePlayer;
    } else {
        ball.drag    = baseBallDrag;
        player1.drag = baseP1Drag;
        player2.drag = baseP2Drag;
        gk1.drag     = baseGK1Drag;
        gk2.drag     = baseGK2Drag;
    }



    // ===== 1) SNAPSHOT input gốc (đến từ InputSystem) =====
    const InputIntent srcP1 = player1.in;
    const InputIntent srcP2 = player2.in;

    // ===== 2) FLIP quyền điều khiển (mỗi bên độc lập, 1 lần/khung) =====
    auto canFlipSide = [&](bool left)->bool{
        Player& p  = left ? player1 : player2;
        Player& gk = left ? gk1     : gk2;
        Entity* controlled = p.isControlled ? (Entity*)&p : (Entity*)&gk;
        // Không cho flip nếu thực thể đang được điều khiển hiện tại đang ôm bóng
        return !(ball.owner == controlled);
    };

    if (srcP1.switchGK && canFlipSide(true))  { player1.isControlled = !player1.isControlled; gk1.isControlled = !gk1.isControlled; }
    if (srcP2.switchGK && canFlipSide(false)) { player2.isControlled = !player2.isControlled; gk2.isControlled = !gk2.isControlled; }

    // ===== 3) ROUTE input theo trạng thái sau flip (xóa switchGK để không lan frame) =====
    auto clearInput = [](InputIntent& in){ in.x=in.y=0; in.shoot=in.slide=in.switchGK=false; };
    auto copyInput  = [](const InputIntent& src, InputIntent& dst){ dst=src; dst.switchGK=false; };

    if (gk1.isControlled) { copyInput(srcP1, gk1.in); clearInput(player1.in); }
    else                  { player1.in = srcP1;       clearInput(gk1.in);     }

    if (gk2.isControlled) { copyInput(srcP2, gk2.in); clearInput(player2.in); }
    else                  { player2.in = srcP2;       clearInput(gk2.in);     }

    // ===== 4) APPLY INPUT + ACTION =====
    bool shot1=false, shot2=false;

    // Bên trái
    if (gk1.isControlled) {
        gk1.applyInput(dt); gk1.updateAnim(dt);
        if (gk1.in.shoot) {
            if (ball.owner == &gk1) PossessionSystem::updateKeeperBallLogic(ball, gk1, gk1Hold, dt);
            else                     shot1 = gk1.tryShoot(ball);
        }
        if (gk1.in.slide) gk1.trySlide(ball, dt);
    } else {
        player1.applyInput(dt); player1.updateAnim(dt);
        if (player1.in.shoot) shot1 = player1.tryShoot(ball);
        if (player1.in.slide) player1.trySlide(ball, dt);
    }

    // Bên phải
    if (gk2.isControlled) {
        gk2.applyInput(dt); gk2.updateAnim(dt);
        if (gk2.in.shoot) {
            if (ball.owner == &gk2) PossessionSystem::updateKeeperBallLogic(ball, gk2, gk2Hold, dt);
            else                     shot2 = gk2.tryShoot(ball);
        }
        if (gk2.in.slide) gk2.trySlide(ball, dt);
    } else {
        player2.applyInput(dt); player2.updateAnim(dt);
        if (player2.in.shoot) shot2 = player2.tryShoot(ball);
        if (player2.in.slide) player2.trySlide(ball, dt);
    }

    if (shot1 || shot2) pickupCooldown = std::max(pickupCooldown, 0.22f);

    // ===== 5) DRIBBLE ASSIST (chỉ cầu thủ thường) =====
    if      (ball.owner == &player1) player1.assistDribble(ball, dt);
    else if (ball.owner == &player2) player2.assistDribble(ball, dt);
    else { player1.assistDribble(ball, dt); player2.assistDribble(ball, dt); }

    // ===== 6) GK AI — không đè GK đang manual =====
    // Nếu keeper AI của bạn chỉ có updatePair(...), ta dùng snapshot/restore GK đang manual:
    {
        Vec2 gk1Pos = gk1.tf.pos, gk1Vel = gk1.tf.vel;
        Vec2 gk2Pos = gk2.tf.pos, gk2Vel = gk2.tf.vel;

        // Gọi AI một phát cho đủ logic phối hợp
        gKeeper.updatePair(ball, gk1, gk2, player1, player2,
                           fieldW, fieldH, centerY, dt, pickupCooldown);

        // Khóa lại GK đang manual (AI không được thay đổi)
        if (gk1.isControlled) { gk1.tf.pos = gk1Pos; gk1.tf.vel = gk1Vel; }
        if (gk2.isControlled) { gk2.tf.pos = gk2Pos; gk2.tf.vel = gk2Vel; }
    }

    // ===== 7) POSSESSION =====
    PossessionSystem::tryTakeAll(ball, player1, player2, gk1, gk2,
                                 fieldW, boxDepth, pickupCooldown, dt);


    // ===== EXTERNAL FORCES: gió nền + gust =====
    if (extForces) {
        // 1) Tự đổi gió nền sau mỗi khoảng thời gian
        windDirTimer -= dt;
        if (windDirTimer <= 0.0f) {
            float ang = frand(0.f, 2.f*PI);
            float strength = frand(windCfg.baseStrengthMin, windCfg.baseStrengthMax);
            wind = Vec2(std::cos(ang), std::sin(ang)) * strength;
            windDirTimer = frand(windCfg.dirChangeMin, windCfg.dirChangeMax);
        }

        // 2) Gió tác động như gia tốc lên bóng
        float scale = (ball.owner ? windCfg.ownerScale : 1.0f);
        ball.tf.vel += wind * (scale * dt);

        // 3) Gust ngắt quãng — cộng thêm một xung vận tốc theo hướng gió (jitter)
        gustTimer -= dt;
        if (gustTimer <= 0.0f) {
            // jitter ±0.35 rad quanh hướng gió
            float jitter = frand(-0.35f, 0.35f);
            float cs = std::cos(jitter), sn = std::sin(jitter);
            Vec2 gust(wind.x*cs - wind.y*sn, wind.x*sn + wind.y*cs);

            if (gust.length() > 1e-4f) {
                Vec2 gv = gust.normalized() * windCfg.gustPower;
                ball.tf.vel += gv;
            }
            gustTimer = frand(windCfg.gustIntervalMin, windCfg.gustIntervalMax);
        }
    }



    // ===== 8) PHYSICS =====
    std::vector<Entity*> ents; ents.reserve(5);
    if (ball.owner == nullptr) ents.push_back(&ball);
    ents.push_back(&player1); ents.push_back(&player2);
    ents.push_back(&gk1);     ents.push_back(&gk2);
    physics.step(dt, ents, goals, fieldW, fieldH);

    // ===== 9) GOAL CHECK =====
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
    
        // === Selection pointers (mỗi bên 1 màu, mũi tên trỏ xuống) ===
    auto drawPointerDown = [&](const Player& who, Uint8 r, Uint8 g, Uint8 b){
        int sw, sh; SDL_GetRendererOutputSize(renderer,&sw,&sh);
        const float sx=(float)sw/(float)fieldW, sy=(float)sh/(float)fieldH;

        // bắt đầu ngay TRÊN đỉnh đầu rồi vẽ xuống dưới
        int cx   = (int)(who.tf.pos.x * sx);
        int topY = (int)((who.tf.pos.y - who.radius - 10.0f) * sy);

        int H = std::max(6, (int)(10.0f * sy));   // chiều cao tam giác
        int W = std::max(8, (int)(14.0f * sx));   // bề rộng đáy

        SDL_SetRenderDrawColor(renderer, r,g,b, 255);
        for (int y = 0; y < H; ++y){
            float t = (H <= 1) ? 1.0f : (float)y / (float)(H - 1);
            int half = (int)((1.0f - t) * (W/2));     // nhỏ dần về mũi
            int ypix = topY + y;                      // vẽ xuống dưới
            SDL_RenderDrawLine(renderer, cx - half, ypix, cx + half, ypix);
        }
    };

    // Bên trái: Cyan
    if (player1.isControlled) drawPointerDown(player1, 0, 200, 255);
    else                      drawPointerDown(gk1,     0, 200, 255);

    // Bên phải: Đỏ cam
    if (player2.isControlled) drawPointerDown(player2, 255, 80, 60);
    else                      drawPointerDown(gk2,     255, 80, 60);


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

    // Indicator nhỏ cho ngoại lực (trên cùng trái)
    if (extForces) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 200);
        SDL_Rect badge{ 10, 10, 80, 22 };
        SDL_RenderFillRect(renderer, &badge);

        // Vẽ biểu tượng mũi tên gió
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        int cx = 50, cy = 21;
        int len = 28;
        int x2 = cx + (int)(len * (wind.length() > 1e-4f ? (wind.x / (std::max(std::abs(wind.x)+std::abs(wind.y), 1.0f))) : 1));
        int y2 = cy + (int)(len * (wind.length() > 1e-4f ? (wind.y / (std::max(std::abs(wind.x)+std::abs(wind.y), 1.0f))) : 0));
        SDL_RenderDrawLine(renderer, cx-12, cy, cx+12, cy); // nền
        SDL_RenderDrawLine(renderer, cx, cy, x2, y2);       // hướng gió
    }


    if (hud) hud->render(sc,t,banner);
}
