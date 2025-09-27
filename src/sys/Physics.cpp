#include "ecs/Player.hpp"
#include "sys/Physics.hpp"
#include <SDL_mixer.h>
#include <cmath>
#include <algorithm>

void PhysicsSystem::step(float dt, std::vector<Entity*>& entities, Goals& goals, int fieldWidth, int fieldHeight) {
    // Tích hợp vị trí cho tất cả thực thể dựa trên vận tốc hiện tại
    for (Entity* ent : entities) {
        // Áp dụng ma sát cho bóng (các cầu thủ đã áp dụng khi applyInput)
        if (ent->mass < 1.0f) { // giả định mass <1 nghĩa là bóng
            ent->tf.vel.x *= std::exp(-ent->drag * dt);
            ent->tf.vel.y *= std::exp(-ent->drag * dt);
        }
        ent->tf.pos.x += ent->tf.vel.x * dt;
        ent->tf.pos.y += ent->tf.vel.y * dt;
    }
    // Xử lý va chạm tròn-tròn giữa các thực thể động
    size_t n = entities.size();
    for (size_t i = 0; i < n; ++i) {
        Entity* e1 = entities[i];
        for (size_t j = i + 1; j < n; ++j) {
            Entity* e2 = entities[j];
            // Kiểm tra trùng lặp (không xét cặp GK cùng đội? – ở đây vẫn xét vì họ có thể va chạm)
            float dx = e2->tf.pos.x - e1->tf.pos.x;
            float dy = e2->tf.pos.y - e1->tf.pos.y;
            float dist2 = dx*dx + dy*dy;
            float rsum = e1->radius + e2->radius;
            if (dist2 < rsum * rsum && dist2 > 0.0f) {
                float dist = std::sqrt(dist2);
                // Vector pháp tuyến đơn vị từ e1 -> e2
                float nx = dx / dist;
                float ny = dy / dist;
                // Vận tốc tương đối theo pháp tuyến
                float relVx = e1->tf.vel.x - e2->tf.vel.x;
                float relVy = e1->tf.vel.y - e2->tf.vel.y;
                float relDotN = relVx * nx + relVy * ny;
                // Nếu vận tốc hướng vào nhau (đang va chạm)
                if (relDotN < 0) {
                    // Hệ số đàn hồi e tùy cặp va chạm
                    float elast;
                    if (e1->mass < 1.0f || e2->mass < 1.0f) {
                        // Nếu một trong hai là bóng
                        elast = 0.3f;
                    } else {
                        // Cầu thủ vs cầu thủ
                        elast = 0.2f;
                    }
                    // Tính xung (impulse) phản hồi va chạm
                    float invMass1 = 1.0f / e1->mass;
                    float invMass2 = 1.0f / e2->mass;
                    float J = -(1.0f + elast) * relDotN / (invMass1 + invMass2);
                    // Cập nhật vận tốc sau va chạm
                    e1->tf.vel.x += J * nx * invMass1;
                    e1->tf.vel.y += J * ny * invMass1;
                    e2->tf.vel.x -= J * nx * invMass2;
                    e2->tf.vel.y -= J * ny * invMass2;
                }
                // Xử lý tách xuyên (đẩy các thực thể ra khỏi nhau nếu overlap)
                float overlap = rsum - dist;
                // Tính phần dịch chuyển cho mỗi thực thể theo khối lượng (vật nhẹ di chuyển nhiều hơn)
                float invMass1 = (e1->mass > 0 ? 1.0f / e1->mass : 0.0f);
                float invMass2 = (e2->mass > 0 ? 1.0f / e2->mass : 0.0f);
                float sumInvMass = invMass1 + invMass2;
                if (sumInvMass == 0) sumInvMass = 1.0f;
                float move1 = overlap * invMass1 / sumInvMass;
                float move2 = overlap * invMass2 / sumInvMass;
                // Dịch chuyển e1 ngược hướng pháp tuyến, e2 theo hướng pháp tuyến để tách chúng
                e1->tf.pos.x -= move1 * nx;
                e1->tf.pos.y -= move1 * ny;
                e2->tf.pos.x += move2 * nx;
                e2->tf.pos.y += move2 * ny;
            }
        }
    }
    // Va chạm bóng với tường (các cạnh sân) và cột gôn
    for (Entity* ent : entities) {
        // Nếu ent là bóng
        if (ent->mass < 1.0f) {
            Ball* ball = static_cast<Ball*>(ent);
            // Tường trên/dưới
            if (ball->tf.pos.y - ball->radius < 0) {
                ball->tf.pos.y = ball->radius;
                ball->tf.vel.y = -ball->tf.vel.y * ball->e_wall;
                Mix_Chunk* wallSfx = Mix_LoadWAV("assets/audio/wall.wav");
                if (wallSfx) Mix_PlayChannel(-1, wallSfx, 0);
            }
            if (ball->tf.pos.y + ball->radius > fieldHeight) {
                ball->tf.pos.y = fieldHeight - ball->radius;
                ball->tf.vel.y = -ball->tf.vel.y * ball->e_wall;
                Mix_Chunk* wallSfx = Mix_LoadWAV("assets/audio/wall.wav");
                if (wallSfx) Mix_PlayChannel(-1, wallSfx, 0);
            }
            // Tường trái/phải (trừ khu vực khung thành)
            if (ball->tf.pos.x - ball->radius < 0) {
                // Nếu bóng không lọt vào giữa 2 cột (ngoài khu cầu môn)
                if (!(ball->tf.pos.y > goals.goalY1 && ball->tf.pos.y < goals.goalY2)) {
                    ball->tf.pos.x = ball->radius;
                    ball->tf.vel.x = -ball->tf.vel.x * ball->e_wall;
                    Mix_Chunk* wallSfx = Mix_LoadWAV("assets/audio/wall.wav");
                    if (wallSfx) Mix_PlayChannel(-1, wallSfx, 0);
                }
            }
            if (ball->tf.pos.x + ball->radius > fieldWidth) {
                if (!(ball->tf.pos.y > goals.goalY1 && ball->tf.pos.y < goals.goalY2)) {
                    ball->tf.pos.x = fieldWidth - ball->radius;
                    ball->tf.vel.x = -ball->tf.vel.x * ball->e_wall;
                    Mix_Chunk* wallSfx = Mix_LoadWAV("assets/audio/wall.wav");
                    if (wallSfx) Mix_PlayChannel(-1, wallSfx, 0);
                }
            }
            // Va chạm bóng với cột gôn (trụ cầu môn)
            Post posts[4] = { goals.leftPosts[0], goals.leftPosts[1], goals.rightPosts[0], goals.rightPosts[1] };
            for (int p = 0; p < 4; ++p) {
                float dx = ball->tf.pos.x - posts[p].pos.x;
                float dy = ball->tf.pos.y - posts[p].pos.y;
                float dist2 = dx*dx + dy*dy;
                float sumRad = ball->radius + posts[p].radius;
                if (dist2 < sumRad * sumRad && dist2 > 0.0f) {
                    float dist = std::sqrt(dist2);
                    float nx = dx / dist;
                    float ny = dy / dist;
                    // Đẩy bóng ra khỏi cột
                    float overlap = sumRad - dist;
                    ball->tf.pos.x += nx * overlap;
                    ball->tf.pos.y += ny * overlap;
                    // Phản xạ vận tốc bóng quanh pháp tuyến cột
                    float vDotN = ball->tf.vel.x * nx + ball->tf.vel.y * ny;
                    if (vDotN < 0) {
                        ball->tf.vel.x -= (1.0f + ball->e_wall) * vDotN * nx;
                        ball->tf.vel.y -= (1.0f + ball->e_wall) * vDotN * ny;
                    }
                    Mix_Chunk* postSfx = Mix_LoadWAV("assets/audio/post.wav");
                    if (postSfx) Mix_PlayChannel(-1, postSfx, 0);
                }
            }
        } else {
            // Nếu ent là cầu thủ (va chạm tường)
            Player* player = dynamic_cast<Player*>(ent);
            if (!player) continue;
            // Tường trên/dưới
            if (player->tf.pos.y - player->radius < 0) {
                player->tf.pos.y = player->radius;
                if (player->tf.vel.y < 0) player->tf.vel.y = 0;
            }
            if (player->tf.pos.y + player->radius > fieldHeight) {
                player->tf.pos.y = fieldHeight - player->radius;
                if (player->tf.vel.y > 0) player->tf.vel.y = 0;
            }
            // Tường trái/phải (kể cả vùng cầu môn để không lọt ra ngoài)
            if (player->tf.pos.x - player->radius < 0) {
                player->tf.pos.x = player->radius;
                if (player->tf.vel.x < 0) player->tf.vel.x = 0;
            }
            if (player->tf.pos.x + player->radius > fieldWidth) {
                player->tf.pos.x = fieldWidth - player->radius;
                if (player->tf.vel.x > 0) player->tf.vel.x = 0;
            }
            // Va chạm cầu thủ với cột gôn (tránh kẹt vào cột)
            Post posts[4] = { goals.leftPosts[0], goals.leftPosts[1], goals.rightPosts[0], goals.rightPosts[1] };
            for (int p = 0; p < 4; ++p) {
                float dx = player->tf.pos.x - posts[p].pos.x;
                float dy = player->tf.pos.y - posts[p].pos.y;
                float dist2 = dx*dx + dy*dy;
                float sumRad = player->radius + posts[p].radius;
                if (dist2 < sumRad * sumRad && dist2 > 0.0f) {
                    float dist = std::sqrt(dist2);
                    float nx = dx / dist;
                    float ny = dy / dist;
                    // Đẩy cầu thủ ra khỏi cột
                    float overlap = sumRad - dist;
                    player->tf.pos.x += nx * overlap;
                    player->tf.pos.y += ny * overlap;
                    // Giảm vận tốc hướng vào cột (không nẩy lại để tránh rung)
                    float vDotN = player->tf.vel.x * nx + player->tf.vel.y * ny;
                    if (vDotN < 0) {
                        player->tf.vel.x -= vDotN * nx;
                        player->tf.vel.y -= vDotN * ny;
                    }
                }
            }
        }
    }
}
