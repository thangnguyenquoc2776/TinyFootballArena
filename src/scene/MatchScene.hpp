#pragma once
#include <SDL.h>
#include <vector>
#include "core/Config.hpp"
#include "ecs/Ball.hpp"
#include "ecs/Player.hpp"
#include "ecs/Goal.hpp"
#include "ecs/Goalkeeper.hpp"
#include "sys/Physics.hpp"
#include <SDL_mixer.h>

// Các trạng thái của trận đấu
enum class MatchState { Kickoff, Playing, GoalFreeze, HalfTimeBreak, FullTime };

class HUD;

class MatchScene {
public:
    MatchScene() = default;
    ~MatchScene();

    // Khởi tạo scene: tạo thực thể bóng, cầu thủ, thủ môn, thiết lập sân
    void init(const Config& config, SDL_Renderer* renderer, HUD* hud);

    // Cập nhật logic scene mỗi frame
    void update(float dt);

    // Vẽ scene (sân, thực thể) và HUD
    void render(SDL_Renderer* renderer, bool paused);

    // Getter cho người chơi P1, P2
    Player* getPlayer1() { return &player1; }
    Player* getPlayer2() { return &player2; }

private:
    // Renderer & HUD
    SDL_Renderer* mRenderer = nullptr;
    HUD* hud = nullptr;

    // Asset (load 1 lần)
    SDL_Texture* pitchTex = nullptr;
    SDL_Texture* ballTex  = nullptr;
    SDL_Texture* p1Tex    = nullptr;
    SDL_Texture* p2Tex    = nullptr;
    SDL_Texture* gkTex    = nullptr;
    Mix_Chunk* goalSfx    = nullptr;
    Mix_Music* crowdMusic = nullptr;

    // Thông số thời gian hiệp
    float halfTimeSeconds = 0.0f;
    float kickoffLockTime = 0.0f;

    // Các thực thể trong trận
    Ball ball;
    Player player1;
    Player player2;
    Goalkeeper gk1;
    Goalkeeper gk2;
    Goals goals;            // quản lý khung thành và điểm số
    PhysicsSystem physics;  // hệ thống vật lý va chạm

    // Trạng thái trận đấu
    MatchState state = MatchState::Kickoff;
    int   currentHalf = 1;
    float timeRemaining = 0.0f;    // thời gian còn lại của hiệp (giây)
    float stateTimer    = 0.0f;    // thời gian đếm lùi của trạng thái (kickoff lock, goal freeze, half break)

    // Kích thước sân
    int fieldW = 0, fieldH = 0;
    float centerY = 0.0f;

    // Vị trí xuất phát ban đầu
    Vec2 initPosP1, initPosP2;
    Vec2 initPosGK1, initPosGK2;
    Vec2 initPosBall;

    // --- External Forces (Wind Mode) ---
    struct WindParams {
        // cường độ gió nền (px/s^2) — nên tầm 120..260 cho game hiện tại
        float baseStrengthMin = 120.0f;
        float baseStrengthMax = 260.0f;

        // khoảng thời gian đổi hướng gió nền (giây)
        float dirChangeMin = 3.0f;
        float dirChangeMax = 7.0f;

        // khoảng thời gian giữa các “gust” (giây)
        float gustIntervalMin = 1.5f;
        float gustIntervalMax = 3.5f;

        // lực “gust” (px/s) cộng vào vận tốc bóng theo hướng gió (có jitter)
        float gustPower = 140.0f;

        // nếu bóng đang được giữ: giảm ảnh hưởng gió
        float ownerScale = 0.55f;

        // scale drag khi bật gió (slippery nhẹ)
        float dragScaleBall   = 0.60f;
        float dragScalePlayer = 0.70f;
    } windCfg;

    bool extForces = false; // đang bật gió?
    bool key2Prev  = false; // rising-edge key '2'

    float baseBallDrag = 0.f;
    float baseP1Drag   = 0.f, baseP2Drag = 0.f;
    float baseGK1Drag  = 0.f, baseGK2Drag = 0.f;

    Vec2  wind       = Vec2(0,0); // gia tốc gió nền (px/s^2)
    float gustTimer  = 0.0f;      // đếm tới lần gust tiếp theo
    float windDirTimer = 0.0f;    // đếm tới lần đổi hướng/độ mạnh gió nền tiếp theo


};
