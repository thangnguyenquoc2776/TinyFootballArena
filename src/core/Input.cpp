#include "core/Input.hpp"
#include <SDL_keyboard.h>

// Hàm trợ giúp: ánh xạ tên phím sang SDL_Scancode (định nghĩa inline ở Config.cpp)
static inline int mapKeyName(const std::string& name) {
    if (name.size() == 1) {
        char c = name[0];
        if (c >= 'A' && c <= 'Z') return SDL_SCANCODE_A + (c - 'A');
        if (c >= 'a' && c <= 'z') return SDL_SCANCODE_A + (c - 'a');
        if (c >= '0' && c <= '9') return SDL_SCANCODE_0 + (c - '0');
    } else {
        if (name == "UP") return SDL_SCANCODE_UP;
        if (name == "DOWN") return SDL_SCANCODE_DOWN;
        if (name == "LEFT") return SDL_SCANCODE_LEFT;
        if (name == "RIGHT") return SDL_SCANCODE_RIGHT;
        if (name == "RCTRL") return SDL_SCANCODE_RCTRL;
        if (name == "RSHIFT") return SDL_SCANCODE_RSHIFT;
        if (name == "LCTRL") return SDL_SCANCODE_LCTRL;
        if (name == "LSHIFT") return SDL_SCANCODE_LSHIFT;
    }
    return SDL_SCANCODE_UNKNOWN;
}

void InputSystem::init(const Config& config) {
    // Lấy mã scancode cho các phím cấu hình P1
    scancodeP1_up    = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.up));
    scancodeP1_down  = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.down));
    scancodeP1_left  = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.left));
    scancodeP1_right = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.right));
    scancodeP1_shoot = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.shoot));
    scancodeP1_slide = static_cast<SDL_Scancode>(mapKeyName(config.keysP1.slide));
    // Mã scancode cho P2
    scancodeP2_up    = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.up));
    scancodeP2_down  = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.down));
    scancodeP2_left  = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.left));
    scancodeP2_right = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.right));
    scancodeP2_shoot = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.shoot));
    scancodeP2_slide = static_cast<SDL_Scancode>(mapKeyName(config.keysP2.slide));
}

void InputSystem::bindPlayers(Player* p1, Player* p2) {
    player1 = p1;
    player2 = p2;
}

void InputSystem::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
        SDL_Scancode code = e.key.keysym.scancode;
        if (code == scancodeP1_shoot) {
            p1ShootPressed = true;
        } else if (code == scancodeP1_slide) {
            p1SlidePressed = true;
        } else if (code == scancodeP2_shoot) {
            p2ShootPressed = true;
        } else if (code == scancodeP2_slide) {
            p2SlidePressed = true;
        } else if (e.key.keysym.sym == SDLK_ESCAPE) {
            // Nhấn ESC: yêu cầu pause hoặc resume
            pausePressed = true;
        }
    }
}

void InputSystem::update() {
    // Lấy trạng thái phím để xử lý di chuyển
    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    if (player1) {
        int dx = 0, dy = 0;
        if (keyState[scancodeP1_left])  dx -= 1;
        if (keyState[scancodeP1_right]) dx += 1;
        if (keyState[scancodeP1_up])    dy -= 1;
        if (keyState[scancodeP1_down])  dy += 1;
        player1->in.x = static_cast<float>(dx);
        player1->in.y = static_cast<float>(dy);
        // Gán cờ shoot/slide (chỉ có giá trị đúng trong frame nhấn)
        player1->in.shoot = p1ShootPressed;
        player1->in.slide = p1SlidePressed;
    }
    if (player2) {
        int dx = 0, dy = 0;
        if (keyState[scancodeP2_left])  dx -= 1;
        if (keyState[scancodeP2_right]) dx += 1;
        if (keyState[scancodeP2_up])    dy -= 1;
        if (keyState[scancodeP2_down])  dy += 1;
        player2->in.x = static_cast<float>(dx);
        player2->in.y = static_cast<float>(dy);
        player2->in.shoot = p2ShootPressed;
        player2->in.slide = p2SlidePressed;
    }
    // Reset cờ sự kiện (đảm bảo shoot, slide chỉ giữ true trong 1 frame)
    p1ShootPressed = p1SlidePressed = false;
    p2ShootPressed = p2SlidePressed = false;
}
