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
};
