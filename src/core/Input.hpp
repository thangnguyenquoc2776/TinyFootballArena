#pragma once
#include <SDL.h>
#include "core/Config.hpp"
#include "ecs/Player.hpp"

// Hệ thống Input: xử lý sự kiện bàn phím và cập nhật InputIntent cho người chơi
class InputSystem {
public:
    // Khởi tạo key mapping từ cấu hình
    void init(const Config& config);
    // Liên kết đối tượng Player của hai người chơi để gán input
    void bindPlayers(Player* p1, Player* p2);
    // Xử lý sự kiện bàn phím (được gọi trong vòng lặp SDL_PollEvent)
    void handleEvent(const SDL_Event& e);
    // Cập nhật trạng thái phím (cho di chuyển liên tục) mỗi frame
    void update();

    bool pausePressed = false;  // cờ yêu cầu Pause (nhấn Esc)

private:
    Player* player1 = nullptr;
    Player* player2 = nullptr;
    // Mã scancode cho các phím điều khiển của P1, P2
    SDL_Scancode scancodeP1_up, scancodeP1_down, scancodeP1_left, scancodeP1_right;
    SDL_Scancode scancodeP1_shoot, scancodeP1_slide;
    SDL_Scancode scancodeP2_up, scancodeP2_down, scancodeP2_left, scancodeP2_right;
    SDL_Scancode scancodeP2_shoot, scancodeP2_slide;
    // Cờ sự kiện bấm phím Shoot/Slide (chỉ kích hoạt một lần lúc nhấn)
    bool p1ShootPressed = false, p1SlidePressed = false;
    bool p2ShootPressed = false, p2SlidePressed = false;
};
