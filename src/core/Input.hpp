#pragma once
#include <SDL.h>
#include "core/Config.hpp"
#include "ecs/Player.hpp"

// Hệ thống Input: xử lý sự kiện bàn phím và cập nhật InputIntent cho người chơi
class InputSystem {
public:
    void init(const Config& config);
    void bindPlayers(Player* p1, Player* p2);
    void handleEvent(const SDL_Event& e);
    void update();

    bool pausePressed = false;

private:
    Player* player1 = nullptr;
    Player* player2 = nullptr;

    // mapping phím P1
    SDL_Scancode scancodeP1_up, scancodeP1_down, scancodeP1_left, scancodeP1_right;
    SDL_Scancode scancodeP1_shoot, scancodeP1_slide;

    // mapping phím P2
    SDL_Scancode scancodeP2_up, scancodeP2_down, scancodeP2_left, scancodeP2_right;
    SDL_Scancode scancodeP2_shoot, scancodeP2_slide;

    SDL_Scancode scancodeP1_switchGK;
    SDL_Scancode scancodeP2_switchGK;

    bool p1SwitchGKPressed = false;
    bool p2SwitchGKPressed = false;


    // flags một lần / frame
    bool p1ShootPressed = false, p1SlidePressed = false;
    bool p2ShootPressed = false, p2SlidePressed = false;
};
