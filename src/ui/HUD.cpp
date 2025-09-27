#include "ui/HUD.hpp"
#include "core/Config.hpp"
#include <SDL.h>
#include <SDL_ttf.h>

HUD::HUD(SDL_Renderer* renderer_, const Config& /*config*/) : renderer(renderer_) {
    // Nạp font Roboto từ thư mục assets/fonts
    fontSmall = TTF_OpenFont("assets/fonts/Roboto-Regular.ttf", 32);
    fontLarge = TTF_OpenFont("assets/fonts/Roboto-Regular.ttf", 72);
}

HUD::~HUD() {
    if (fontSmall) { TTF_CloseFont(fontSmall); fontSmall = nullptr; }
    if (fontLarge) { TTF_CloseFont(fontLarge); fontLarge = nullptr; }
}

static void renderText(SDL_Renderer* renderer, TTF_Font* font,
                       const std::string& text, SDL_Color color,
                       int y, bool centerX, bool centerY) {
    if (!font || text.empty()) return;

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    int w = surf->w, h = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return;

    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);

    SDL_Rect dst{0, y, w, h};
    if (centerX) dst.x = (screenW - w) / 2;
    if (centerY) dst.y = (screenH - h) / 2;

    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void HUD::render(const std::string& scoreText,
                 const std::string& timeText,
                 const std::string& bannerText) {
    // Score (trên cùng)
    renderText(renderer, fontSmall, scoreText, colorWhite, 5,  true, false);
    // Time (ngay dưới score)
    renderText(renderer, fontSmall, timeText,  colorWhite, 45, true, false);
    // Banner (giữa màn hình)
    renderText(renderer, fontLarge, bannerText, colorWhite, 0, true, true);
}
