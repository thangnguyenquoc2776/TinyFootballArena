#pragma once
#include <string>

// Cấu trúc cấu hình game, chứa các tham số đọc từ file JSON
struct Config {
    // Thông số cửa sổ
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;

    // Tỉ lệ đơn vị thế giới
    float meters_per_px = 0.025f;
    float pixel_per_meter = 40.0f;

    // Kích thước sân
    int fieldWidth = 1280;
    int fieldHeight = 720;

    // Camera
    float cameraDeadzoneRatio = 0.3f;
    float cameraLerp = 0.1f;

    // Bóng
    float ballRadius = 0.0f;    // px
    float ballMass = 0.0f;
    float ballDrag = 0.0f;
    float ballElasticityWall = 0.5f;

    // Cầu thủ
    float playerRadius = 0.0f;  // px
    float playerMass = 0.0f;
    float playerMaxSpeed = 0.0f;  // px/s
    float playerAccel = 0.0f;     // px/s^2
    float playerDrag = 0.0f;
    float playerElasticityWall = 0.05f;

    // Thủ môn
    float gkRadius = 0.0f;
    float gkMass = 0.0f;
    float gkMaxSpeed = 0.0f;
    float gkAccel = 0.0f;
    float gkDrag = 0.0f;
    float gkElasticityWall = 0.0f;
    float gkFrontOffset = 0.0f;

    // Sút
    float kickImpulseMin = 0.0f;
    float kickImpulseMax = 0.0f;
    float kickCooldown = 0.0f;

    // Xoạc
    float tackleDashSpeed = 0.0f;
    float tackleDuration = 0.0f;
    float tackleCooldown = 0.0f;
    float tackleInterceptSlack = 0.0f;
    float tackleDislodgeSpeed = 0.0f;

    // Trận đấu
    int matchHalves = 2;
    int halfTimeSeconds = 120;
    float goalFreezeTime = 2.0f;
    float kickoffLockTime = 1.0f;

    // Phím điều khiển
    struct KeyMap {
        std::string up, down, left, right, shoot, slide;
        std::string switchGK; // <<< THÊM DÒNG NÀY
    } keysP1, keysP2;

    bool loadFromFile(const std::string& gameConfigFile, const std::string& inputConfigFile);
};
