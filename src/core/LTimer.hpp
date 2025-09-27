#pragma once
#include <SDL.h>

// Timer đơn giản để tính delta time và quản lý frame
class LTimer {
public:
    LTimer() : startTicks(0), pausedTicks(0), paused(false), started(false), vsync(false), lastTicks(0) {}

    void start() {
        started = true;
        paused = false;
        startTicks = SDL_GetTicks();
        pausedTicks = 0;
        lastTicks = startTicks;
    }

    void stop() {
        started = false;
        paused = false;
        startTicks = 0;
        pausedTicks = 0;
        lastTicks = 0;
    }

    void pause() {
        if (started && !paused) {
            paused = true;
            pausedTicks = SDL_GetTicks() - startTicks;
            startTicks = 0;
        }
    }

    void resume() {
        if (started && paused) {
            paused = false;
            startTicks = SDL_GetTicks() - pausedTicks;
            pausedTicks = 0;
        }
    }

    Uint32 getTicks() const {
        if (started) {
            if (paused) return pausedTicks;
            else return SDL_GetTicks() - startTicks;
        }
        return 0;
    }

    float getDeltaSeconds() {
        Uint32 current = SDL_GetTicks();
        float dt = (current - lastTicks) / 1000.0f;
        lastTicks = current;
        return dt;
    }

    void setVSync(bool enabled) { vsync = enabled; }
    bool isVSync() const { return vsync; }

private:
    Uint32 startTicks;
    Uint32 pausedTicks;
    Uint32 lastTicks;
    bool paused;
    bool started;
    bool vsync;
};
