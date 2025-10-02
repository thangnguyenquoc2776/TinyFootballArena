#pragma once
#include <SDL.h>
#include <vector>

struct Animation {
    std::vector<SDL_Texture*> frames;
    int current = 0;
    float frameTime = 0.15f;  // thời gian đổi frame
    float timer = 0.0f;

    void update(float dt) {
        if (frames.size() <= 1) return; // idle: 1 frame
        timer += dt;
        if (timer >= frameTime) {
            timer -= frameTime;
            current = (current + 1) % frames.size();
        }
    }

    SDL_Texture* getFrame() {
        if (frames.empty()) return nullptr;
        return frames[current];
    }
};
