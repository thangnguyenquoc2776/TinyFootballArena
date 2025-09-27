#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

// Lớp HUD quản lý hiển thị điểm số, thời gian và banner thông báo
class HUD {
public:
    HUD(SDL_Renderer* renderer, const struct Config& config);
    ~HUD();

    // Vẽ HUD: tỉ số, đồng hồ, banner (nếu có)
    void render(const std::string& scoreText,
                const std::string& timeText,
                const std::string& bannerText);

private:
    SDL_Renderer* renderer = nullptr;
    TTF_Font* fontSmall = nullptr;
    TTF_Font* fontLarge = nullptr;
    SDL_Color colorWhite{255, 255, 255, 255};
};
