#pragma once
#include <SDL.h>
#include "core/Config.hpp"
#include "ecs/Player.hpp"

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

    // P1 mappings
    SDL_Scancode scancodeP1_up{}, scancodeP1_down{}, scancodeP1_left{}, scancodeP1_right{};
    SDL_Scancode scancodeP1_shoot{}, scancodeP1_slide{}, scancodeP1_switchGK{};

    // P2 mappings
    SDL_Scancode scancodeP2_up{}, scancodeP2_down{}, scancodeP2_left{}, scancodeP2_right{};
    SDL_Scancode scancodeP2_shoot{}, scancodeP2_slide{}, scancodeP2_switchGK{};

    // one-frame flags
    bool p1ShootPressed=false, p1SlidePressed=false, p1SwitchGKPressed=false;
    bool p2ShootPressed=false, p2SlidePressed=false, p2SwitchGKPressed=false;
};
