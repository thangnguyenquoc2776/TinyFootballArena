#include "core/Game.hpp"

void Game::init(const Config& config, SDL_Renderer* renderer) {
    // Khởi tạo HUD với renderer
    hud = new HUD(renderer, config);

    // Khởi tạo Scene trận đấu (truyền thêm renderer)
    currentScene = new MatchScene();
    currentScene->init(config, renderer, hud);
}

void Game::update(float dt) {
    if (!paused && currentScene) {
        currentScene->update(dt);
    }
}

void Game::render(SDL_Renderer* renderer) {
    if (currentScene) {
        currentScene->render(renderer, paused);
    }
}

void Game::togglePause() { paused = !paused; }

void Game::cleanup() {
    if (currentScene) { delete currentScene; currentScene = nullptr; }
    if (hud) { delete hud; hud = nullptr; }
}

Player* Game::getPlayer1() { return currentScene ? currentScene->getPlayer1() : nullptr; }
Player* Game::getPlayer2() { return currentScene ? currentScene->getPlayer2() : nullptr; }
