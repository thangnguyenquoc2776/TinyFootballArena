#pragma once
#include <SDL.h>
#include "core/Config.hpp"
#include "ecs/Player.hpp"
#include "ui/HUD.hpp"
#include "scene/MatchScene.hpp"

// Lớp Game quản lý state toàn cục của trò chơi (scene hiện tại, pause, v.v.)
class Game {
public:
    // Khởi tạo Game (tạo Scene, HUD) với cấu hình và SDL_Renderer
    void init(const Config& config, SDL_Renderer* renderer);
    // Cập nhật logic game (gọi update scene nếu không pause)
    void update(float dt);
    // Vẽ frame (gọi render scene + HUD)
    void render(SDL_Renderer* renderer);
    // Chuyển đổi trạng thái Pause
    void togglePause();
    // Xóa dữ liệu game (xóa scene, hud)
    void cleanup();

    // Lấy trỏ đến người chơi (P1, P2) trong scene hiện tại để liên kết input
    Player* getPlayer1();
    Player* getPlayer2();

private:
    MatchScene* currentScene = nullptr;
    HUD* hud = nullptr;
    bool paused = false;
};
