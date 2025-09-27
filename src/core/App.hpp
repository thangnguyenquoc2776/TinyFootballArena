#pragma once
#include <SDL.h>
#include "core/Config.hpp"
#include "core/Input.hpp"
#include "core/Game.hpp"

// Lớp App quản lý khởi tạo SDL, cửa sổ, renderer, vòng lặp chính
class App {
public:
    // Khởi tạo SDL, các hệ thống và load config
    bool init();
    // Chạy vòng lặp game chính
    void run();
    // Giải phóng tài nguyên và thoát SDL
    void cleanup();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    Config config;         // cấu hình game đọc từ JSON
    InputSystem input;     // hệ thống xử lý input
    Game game;             // đối tượng game (quản lý scene, trạng thái)
};
