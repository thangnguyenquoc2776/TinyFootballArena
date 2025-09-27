#include "core/App.hpp"
#include "core/LTimer.hpp"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <cstdio>

bool App::init() {
    // Đọc cấu hình từ file JSON
    if (!config.loadFromFile("config/game.json", "config/input.json")) {
        SDL_Log("Failed to load config files.\n");
        return false;
    }
    // Khởi tạo SDL (Video & Audio)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    // Tạo cửa sổ game
    window = SDL_CreateWindow("Tiny Football Arena",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config.windowWidth, config.windowHeight, 0);
    if (!window) {
        SDL_Log("Create Window Error: %s\n", SDL_GetError());
        return false;
    }
    // Tạo renderer với hoặc không VSYNC tùy theo config
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    if (config.vsync) {
        render_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    renderer = SDL_CreateRenderer(window, -1, render_flags);
    if (!renderer) {
        SDL_Log("Create Renderer Error: %s\n", SDL_GetError());
        return false;
    }
    // Khởi tạo SDL_image (hỗ trợ PNG)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("IMG_Init Error: %s\n", IMG_GetError());
        return false;
    }
    // Khởi tạo SDL_ttf
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init Error: %s\n", TTF_GetError());
        return false;
    }
    // Khởi tạo SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio Error: %s\n", Mix_GetError());
        return false;
    }
    // Khởi tạo decoder cho OGG (tùy chọn)
    Mix_Init(MIX_INIT_OGG);

    // Cấu hình hệ thống Input theo config
    input.init(config);
    // Khởi tạo game (tạo Scene, HUD, v.v.)
    game.init(config, renderer);
    // Liên kết player của scene với input system để nhận điều khiển
    input.bindPlayers(game.getPlayer1(), game.getPlayer2());
    return true;
}

void App::run() {
    bool quit = false;
    SDL_Event e;
    // Đối tượng Timer tính toán delta time mỗi frame
    LTimer frameTimer;
    frameTimer.setVSync(config.vsync);
    frameTimer.start();

    // Vòng lặp chính
    while (!quit) {
        // Xử lý sự kiện hệ thống (đóng cửa sổ, phím bấm,...)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else {
                input.handleEvent(e);
            }
        }
        if (quit) break;
        // Toggle Pause nếu nhấn Esc
        if (input.pausePressed) {
            game.togglePause();
            input.pausePressed = false;
        }
        // Cập nhật trạng thái input liên tục (phím mũi tên, WASD)
        input.update();
        // Tính thời gian frame
        float dt = frameTimer.getDeltaSeconds();
        // Cập nhật game logic (nếu không pause)
        game.update(dt);
        // Vẽ khung hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        game.render(renderer);
        SDL_RenderPresent(renderer);
    }
}

void App::cleanup() {
    // Hủy scene và game
    game.cleanup();
    // Giải phóng SDL_mixer
    Mix_HaltMusic();
    Mix_CloseAudio();
    Mix_Quit();
    // Giải phóng SDL_ttf
    TTF_Quit();
    // Giải phóng SDL_image
    IMG_Quit();
    // Hủy renderer và window
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
