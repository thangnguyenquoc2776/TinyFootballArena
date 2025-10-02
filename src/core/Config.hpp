#pragma once
#include <string>

// Cấu trúc cấu hình game, chứa các tham số đọc từ file JSON
struct Config {
    // Thông số cửa sổ
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;
    // Tỉ lệ đơn vị thế giới (mỗi pixel = bao nhiêu mét)
    float meters_per_px = 0.025f;
    float pixel_per_meter = 40.0f; // 1/0.025 = 40
    // Kích thước sân (tính bằng pixel)
    int fieldWidth = 1280;
    int fieldHeight = 720;
    // Camera
    float cameraDeadzoneRatio = 0.3f;
    float cameraLerp = 0.1f;
    // Thuộc tính vật lý của bóng
    float ballRadius = 0.0f;    // px
    float ballMass = 0.0f;
    float ballDrag = 0.0f;
    float ballElasticityWall = 0.5f;
    // Thuộc tính cầu thủ (người chơi)
    float playerRadius = 0.0f;  // px
    float playerMass = 0.0f;
    float playerMaxSpeed = 0.0f;  // px/giây
    float playerAccel = 0.0f;     // px/giây^2
    float playerDrag = 0.0f;
    float playerElasticityWall = 0.05f;
    // Thuộc tính thủ môn (GK)
    float gkRadius = 0.0f;
    float gkMass = 0.0f;
    float gkMaxSpeed = 0.0f;
    float gkAccel = 0.0f;
    float gkDrag = 0.0f;
    float gkElasticityWall = 0.0f;
    float gkFrontOffset = 0.0f; // khoảng cách đứng trước vạch gôn (px)
    // Sút bóng
    float kickImpulseMin = 0.0f;
    float kickImpulseMax = 0.0f;
    float kickCooldown = 0.0f;
    // Xoạc bóng (tackle)
    float tackleDashSpeed = 0.0f;
    float tackleDuration = 0.0f;
    float tackleCooldown = 0.0f;
    float tackleInterceptSlack = 0.0f;
    float tackleDislodgeSpeed = 0.0f;
    // Thông số trận đấu
    int matchHalves = 2;
    int halfTimeSeconds = 120;
    float goalFreezeTime = 2.0f;
    float kickoffLockTime = 1.0f;

    // Cấu trúc phím điều khiển cho 2 người chơi
    struct KeyMap { std::string up, down, left, right, shoot, slide; } keysP1, keysP2;

    // Hàm đọc file JSON (game.json và input.json) để thiết lập các giá trị trên
    bool loadFromFile(const std::string& gameConfigFile, const std::string& inputConfigFile);
};
