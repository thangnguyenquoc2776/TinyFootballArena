#include "core/Config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end   = s.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Hàm trợ giúp chuyển chuỗi phím (W, LEFT, RCTRL, ...) thành SDL_Scancode
// (Lưu ý: Hàm này được dùng trong InputSystem; không sử dụng trực tiếp ở đây)
static inline int mapKeyName(const std::string& name);

// Hàm load config từ file JSON
bool Config::loadFromFile(const std::string& gameConfigFile, const std::string& inputConfigFile) {
    std::ifstream fin(gameConfigFile);
    if (!fin.is_open()) return false;
    std::string line;
    std::string section;
    while (std::getline(fin, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line.back() == '{') {
            // Bắt đầu một section, ví dụ "window": {
            size_t keyEnd = line.find(':');
            if (keyEnd != std::string::npos) {
                std::string name = trim(line.substr(0, keyEnd));
                if (!name.empty() && name.front() == '"' && name.back() == '"') {
                    name = name.substr(1, name.size() - 2);
                }
                section = name;
            }
            continue;
        }
        if (line.front() == '}' || line.front() == ']') {
            // Kết thúc section
            section.clear();
            continue;
        }
        // Xử lý một dòng "key": value
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        std::string key = trim(line.substr(0, colonPos));
        if (!key.empty() && key.front() == '"') {
            key = key.substr(1, key.find_last_of('"') - 1);
        }
        std::string value = trim(line.substr(colonPos + 1));
        // Xóa dấu phẩy cuối dòng nếu có
        if (!value.empty() && value.back() == ',') {
            value.pop_back();
            value = trim(value);
        }
        // Xử lý giá trị tùy kiểu
        if (section == "window") {
            if (key == "w") windowWidth = std::stoi(value);
            else if (key == "h") windowHeight = std::stoi(value);
            else if (key == "vsync") vsync = (value == "true");
        } else if (section == "world") {
            if (key == "meters_per_px") {
                meters_per_px = std::stof(value);
                pixel_per_meter = (meters_per_px != 0.0f ? 1.0f / meters_per_px : 40.0f);
            } else if (key == "pitch_m") {
                // Giá trị dạng mảng [width, height]
                // Loại bỏ dấu [ và ] rồi tách các số
                size_t lb = value.find('[');
                size_t rb = value.find(']');
                if (lb != std::string::npos && rb != std::string::npos) {
                    std::string arr = value.substr(lb + 1, rb - lb - 1);
                    std::replace(arr.begin(), arr.end(), ',', ' ');
                    std::istringstream iss(arr);
                    float pitchWm, pitchHm;
                    iss >> pitchWm >> pitchHm;
                    fieldWidth = static_cast<int>(pitchWm / meters_per_px + 0.5f);
                    fieldHeight = static_cast<int>(pitchHm / meters_per_px + 0.5f);
                }
            }
        } else if (section == "camera") {
            if (key == "deadzone_ratio") cameraDeadzoneRatio = std::stof(value);
            else if (key == "lerp") cameraLerp = std::stof(value);
        } else if (section == "ball") {
            if (key == "r_m") {
                float r_m = std::stof(value);
                ballRadius = r_m / meters_per_px;
            } else if (key == "m") ballMass = std::stof(value);
            else if (key == "drag") ballDrag = std::stof(value);
            else if (key == "e_wall") ballElasticityWall = std::stof(value);
        } else if (section == "player") {
            if (key == "r_m") {
                float r_m = std::stof(value);
                playerRadius = r_m / meters_per_px;
            } else if (key == "m") playerMass = std::stof(value);
            else if (key == "vmax") playerMaxSpeed = std::stof(value) * pixel_per_meter;
            else if (key == "accel") playerAccel = std::stof(value) * pixel_per_meter;
            else if (key == "drag") playerDrag = std::stof(value);
            else if (key == "e_wall") playerElasticityWall = std::stof(value);
        } else if (section == "gk") {
            if (key == "m") gkMass = std::stof(value);
            else if (key == "vmax") gkMaxSpeed = std::stof(value) * pixel_per_meter;
            else if (key == "accel") gkAccel = std::stof(value) * pixel_per_meter;
            else if (key == "front_offset_m") {
                float off_m = std::stof(value);
                gkFrontOffset = off_m / meters_per_px;
            }
            // GK dùng chung một số thuộc tính với player
            gkRadius = playerRadius;
            gkDrag = playerDrag;
            gkElasticityWall = playerElasticityWall;
        } else if (section == "kick") {
            if (key == "impulse_min") kickImpulseMin = std::stof(value);
            else if (key == "impulse_max") kickImpulseMax = std::stof(value);
            else if (key == "cooldown") kickCooldown = std::stof(value);
        } else if (section == "tackle") {
            if (key == "dash_speed") tackleDashSpeed = std::stof(value) * pixel_per_meter;
            else if (key == "duration") tackleDuration = std::stof(value);
            else if (key == "cooldown") tackleCooldown = std::stof(value);
            else if (key == "intercept_slack_m") tackleInterceptSlack = std::stof(value) / meters_per_px;
            else if (key == "dislodge_speed") tackleDislodgeSpeed = std::stof(value) * pixel_per_meter;
        } else if (section == "match") {
            if (key == "halves") matchHalves = std::stoi(value);
            else if (key == "half_seconds") halfTimeSeconds = std::stoi(value);
            else if (key == "goal_freeze") goalFreezeTime = std::stof(value);
            else if (key == "kickoff_lock") kickoffLockTime = std::stof(value);
        }
    }
    fin.close();

    // Đọc file cấu hình phím
    std::ifstream fin2(inputConfigFile);
    if (!fin2.is_open()) return false;
    section.clear();
    while (std::getline(fin2, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line.back() == '{') {
            size_t keyEnd = line.find(':');
            if (keyEnd != std::string::npos) {
                std::string name = trim(line.substr(0, keyEnd));
                if (!name.empty() && name.front() == '"' && name.back() == '"') {
                    name = name.substr(1, name.size() - 2);
                }
                section = name;
            }
            continue;
        }
        if (line.front() == '}' || line.front() == ']') {
            section.clear();
            continue;
        }
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        std::string key = trim(line.substr(0, colonPos));
        if (!key.empty() && key.front() == '"') {
            key = key.substr(1, key.find_last_of('"') - 1);
        }
        std::string val = trim(line.substr(colonPos + 1));
        if (!val.empty() && val.back() == ',') {
            val.pop_back();
            val = trim(val);
        }
        if (!val.empty() && val.front() == '"') {
            val = val.substr(1, val.find_last_of('"') - 1);
        }
        if (section == "p1") {
            if (key == "up") keysP1.up = val;
            else if (key == "down") keysP1.down = val;
            else if (key == "left") keysP1.left = val;
            else if (key == "right") keysP1.right = val;
            else if (key == "shoot") keysP1.shoot = val;
            else if (key == "slide") keysP1.slide = val;
        } else if (section == "p2") {
            if (key == "up") keysP2.up = val;
            else if (key == "down") keysP2.down = val;
            else if (key == "left") keysP2.left = val;
            else if (key == "right") keysP2.right = val;
            else if (key == "shoot") keysP2.shoot = val;
            else if (key == "slide") keysP2.slide = val;
        }
    }
    fin2.close();
    return true;
}
