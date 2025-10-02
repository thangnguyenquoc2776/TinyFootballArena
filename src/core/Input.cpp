#include "core/Input.hpp"
#include <SDL_keyboard.h>

static inline int mapKeyName(const std::string& name) {
    if (name.size()==1) {
        char c=name[0];
        if (c>='A'&&c<='Z') return SDL_SCANCODE_A+(c-'A');
        if (c>='a'&&c<='z') return SDL_SCANCODE_A+(c-'a');
        if (c>='0'&&c<='9') return SDL_SCANCODE_0+(c-'0');
    } else {
        if (name=="UP") return SDL_SCANCODE_UP;
        if (name=="DOWN") return SDL_SCANCODE_DOWN;
        if (name=="LEFT") return SDL_SCANCODE_LEFT;
        if (name=="RIGHT") return SDL_SCANCODE_RIGHT;
        if (name=="RCTRL") return SDL_SCANCODE_RCTRL;
        if (name=="RSHIFT") return SDL_SCANCODE_RSHIFT;
        if (name=="LCTRL") return SDL_SCANCODE_LCTRL;
        if (name=="LSHIFT") return SDL_SCANCODE_LSHIFT;
        if (name=="ENTER") return SDL_SCANCODE_RETURN;
    }
    return SDL_SCANCODE_UNKNOWN;
}

void InputSystem::init(const Config& cfg) {
    // P1
    scancodeP1_up    = (SDL_Scancode)mapKeyName(cfg.keysP1.up);
    scancodeP1_down  = (SDL_Scancode)mapKeyName(cfg.keysP1.down);
    scancodeP1_left  = (SDL_Scancode)mapKeyName(cfg.keysP1.left);
    scancodeP1_right = (SDL_Scancode)mapKeyName(cfg.keysP1.right);
    scancodeP1_shoot = (SDL_Scancode)mapKeyName(cfg.keysP1.shoot);
    scancodeP1_slide = (SDL_Scancode)mapKeyName(cfg.keysP1.slide);
    // nếu config có switchGK thì dùng, không thì mặc định G
    scancodeP1_switchGK = cfg.keysP1.switchGK.empty() ? SDL_SCANCODE_G
                                                      : (SDL_Scancode)mapKeyName(cfg.keysP1.switchGK);

    // P2
    scancodeP2_up    = (SDL_Scancode)mapKeyName(cfg.keysP2.up);
    scancodeP2_down  = (SDL_Scancode)mapKeyName(cfg.keysP2.down);
    scancodeP2_left  = (SDL_Scancode)mapKeyName(cfg.keysP2.left);
    scancodeP2_right = (SDL_Scancode)mapKeyName(cfg.keysP2.right);
    scancodeP2_shoot = (SDL_Scancode)mapKeyName(cfg.keysP2.shoot);
    scancodeP2_slide = (SDL_Scancode)mapKeyName(cfg.keysP2.slide);
    scancodeP2_switchGK = cfg.keysP2.switchGK.empty() ? SDL_SCANCODE_P
                                                      : (SDL_Scancode)mapKeyName(cfg.keysP2.switchGK);
}

void InputSystem::bindPlayers(Player* p1, Player* p2) {
    player1 = p1; player2 = p2;
}

void InputSystem::handleEvent(const SDL_Event& e) {
    if (e.type==SDL_KEYDOWN && e.key.repeat==0) {
        SDL_Scancode code = e.key.keysym.scancode;

        // P1
        if (code==scancodeP1_shoot)      p1ShootPressed = true;
        else if (code==scancodeP1_slide) p1SlidePressed = true;
        else if (code==scancodeP1_switchGK) p1SwitchGKPressed = true;

        // P2
        else if (code==scancodeP2_shoot)      p2ShootPressed = true;
        else if (code==scancodeP2_slide)      p2SlidePressed = true;
        else if (code==scancodeP2_switchGK)   p2SwitchGKPressed = true;

        else if (e.key.keysym.sym==SDLK_ESCAPE) pausePressed = true;
    }
}

void InputSystem::update() {
    const Uint8* ks = SDL_GetKeyboardState(nullptr);

    if (player1) {
        int dx=0, dy=0;
        if (ks[scancodeP1_left])  dx -= 1;
        if (ks[scancodeP1_right]) dx += 1;
        if (ks[scancodeP1_up])    dy -= 1;
        if (ks[scancodeP1_down])  dy += 1;
        player1->in.x = (float)dx;
        player1->in.y = (float)dy;
        player1->in.shoot    = p1ShootPressed;
        player1->in.slide    = p1SlidePressed;
        player1->in.switchGK = p1SwitchGKPressed;
    }

    if (player2) {
        int dx=0, dy=0;
        if (ks[scancodeP2_left])  dx -= 1;
        if (ks[scancodeP2_right]) dx += 1;
        if (ks[scancodeP2_up])    dy -= 1;
        if (ks[scancodeP2_down])  dy += 1;
        player2->in.x = (float)dx;
        player2->in.y = (float)dy;
        player2->in.shoot    = p2ShootPressed;
        player2->in.slide    = p2SlidePressed;
        player2->in.switchGK = p2SwitchGKPressed;
    }

    // one-frame reset
    p1ShootPressed=p1SlidePressed=p1SwitchGKPressed=false;
    p2ShootPressed=p2SlidePressed=p2SwitchGKPressed=false;
}
