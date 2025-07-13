#include <stdio.h>
#include <android/log.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "imgui.h" // Đã fix, cần đặt imgui.h trong thư mục

// Base (giả định, thay bằng base thực từ `pm map com.dts.freefiremax`)
void* lib_base = (void*)0x10000000;
void* local_player_base = lib_base + 0x385E098;
void* target_list = lib_base + 0x385E0B0;
void* camera_manager = lib_base + 0x2B3A9AC;
void* view_matrix = lib_base + 0x2B17C8C;

// Offsets
int target_size = 0x8;
int my_team_offset = 0x3C;
int is_alive_offset = 0xE0;
int team_id_offset = 0x3C;
int position_offset = 0x34;
int player_name_offset = 0x158;
int bone_base_offset = 0x200;
int head_bone_index = 0x70;
int health_offset = 0xF4;
int is_knocked_offset = 0x2D8;
int is_enemy_visible_offset = 0x11C;
int is_behind_wall_offset = 0x118;
int weapon_offset = 0x1B0; // Giả định
int input_trigger_offset = 0x158;
int camera_offset = 0x10;
int anti_scan_offset = 0x400; // Giả định
int volume_offset = 0x200; // Giả định
int record_offset = 0x300; // Giả định

// Structs
struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };

// State
bool enableESP = true, showBox = true, showName = true, showDistance = true, showLine = true, showHP = true;
bool showStatus = true, showSkeleton = true, wallhack = true, showWeapon = true, showRadar = true;
bool enableColor = false, hideOnRecord = false, enableAimbot = true;
float boxSize = 50.0f, radarSize = 200.0f, aimFOV = 30.0f;
ImVec4 boxColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
bool menuVisible = false, isRecording = false;
int volumePressCount = 0;

bool FirePressed() { return *(bool*)((char*)local_player_base + input_trigger_offset); }
struct Vector3 GetBonePosition(uintptr_t entity, int index) {
    uintptr_t bone_base = *(uintptr_t*)((char*)entity + bone_base_offset);
    return *(struct Vector3*)((char*)bone_base + index * 0x30);
}
float GetDistance(struct Vector3 a, struct Vector3 b) {
    return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2) + powf(a.z - b.z, 2));
}
bool WorldToScreen(struct Vector3 world, struct Vector2* screen) {
    float* matrix = (float*)view_matrix; // Giả định 16 float
    screen->x = world.x * matrix[0] + world.y * matrix[4] + world.z * matrix[8] + matrix[12];
    screen->y = world.x * matrix[1] + world.y * matrix[5] + world.z * matrix[9] + matrix[13];
    return (screen->z = world.x * matrix[3] + world.y * matrix[7] + world.z * matrix[11] + matrix[15]) > 0;
}

void bypass_scan() {
    if (*(int*)((char*)local_player_base + anti_scan_offset) == 1) {
        usleep(rand() % 100000 + 50000);
        __android_log_print(ANDROID_LOG_INFO, "Hack", "Bypass Scan Detected");
    }
}

void handle_volume_key() {
    if (*(int*)((char*)local_player_base + volume_offset) == 1) {
        volumePressCount++;
        if (volumePressCount == 2) {
            menuVisible = !menuVisible;
            volumePressCount = 0;
        }
        usleep(500000);
    }
}

void check_recording() {
    if (*(int*)((char*)local_player_base + record_offset) == 1) isRecording = true;
    else isRecording = false;
}

void RenderESP(uintptr_t enemy, struct Vector3 my_pos) {
    if (!enableESP || (hideOnRecord && isRecording)) return;
    int my_team = *(int*)((char*)local_player_base + my_team_offset);
    int team_id = *(int*)((char*)enemy + team_id_offset);
    if (team_id == my_team && !wallhack) return;
    struct Vector3 pos = *(struct Vector3*)((char*)enemy + position_offset);
    struct Vector2 screen;
    if (!WorldToScreen(pos, &screen)) return;
    float dist = GetDistance(my_pos, pos);
    float size = boxSize / (dist / 10.0f + 1.0f);

    bypass_scan();
    if (showBox) ImGui::GetOverlayDrawList()->AddRect(ImVec2(screen.x - size/2, screen.y - size), ImVec2(screen.x + size/2, screen.y), ImGui::GetColorU32(boxColor));
    if (showName) ImGui::GetOverlayDrawList()->AddText(ImVec2(screen.x, screen.y - size - 10), 0xFFFFFFFF, (char*)((char*)enemy + player_name_offset));
    if (showDistance) {
        char distStr[16]; snprintf(distStr, sizeof(distStr), "%.1fm", dist);
        ImGui::GetOverlayDrawList()->AddText(ImVec2(screen.x, screen.y + size), 0xFFFFFFFF, distStr);
    }
    if (showLine) ImGui::GetOverlayDrawList()->AddLine(ImVec2(960, 1080), screen, ImGui::GetColorU32(boxColor));
    if (showHP) {
        float hp = *(float*)((char*)enemy + health_offset) / 100.0f;
        ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(screen.x - 25, screen.y - size - 5), ImVec2(screen.x - 25 + 50 * hp, screen.y - size), 0xFF00FF00);
    }
    if (showStatus) {
        bool knocked = *(int*)((char*)enemy + is_knocked_offset);
        ImGui::GetOverlayDrawList()->AddText(ImVec2(screen.x, screen.y + size + 10), knocked ? 0xFFFFFF00 : 0xFFFFFFFF, knocked ? "Knocked" : "Alive");
    }
    if (showSkeleton) {
        struct Vector3 head = GetBonePosition(enemy, head_bone_index);
        struct Vector2 headScreen;
        if (WorldToScreen(head, &headScreen)) ImGui::GetOverlayDrawList()->AddLine(screen, headScreen, 0xFFFFFFFF);
    }
    if (showWeapon) ImGui::GetOverlayDrawList()->AddText(ImVec2(screen.x, screen.y + size + 20), 0xFFFFFFFF, (char*)((char*)enemy + weapon_offset));
    if (showRadar) {
        float radarX = 50 + (pos.x / 1000.0f) * radarSize;
        float radarY = 50 + (pos.z / 1000.0f) * radarSize;
        ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(radarX, radarY), 5, 0xFF0000FF);
    }
}

void AimbotThread() {
    srand(time(NULL));
    while (true) {
        usleep(16000 + rand() % 1000);
        bypass_scan();
        if (enableAimbot && FirePressed()) {
            struct Vector3 my_pos = *(struct Vector3*)((char*)local_player_base + position_offset);
            int my_team = *(int*)((char*)local_player_base + my_team_offset);
            uintptr_t closest_enemy = 0;
            float min_fov = aimFOV;
            for (int i = 0; i < 30; i++) {
                uintptr_t enemy = *(uintptr_t*)((char*)target_list + i * target_size);
                if (!enemy) continue;
                bool is_dead = !*(int*)((char*)enemy + is_alive_offset);
                bool is_knocked = *(int*)((char*)enemy + is_knocked_offset);
                bool is_visible = *(int*)((char*)enemy + is_enemy_visible_offset);
                bool is_behind_wall = *(int*)((char*)enemy + is_behind_wall_offset);
                int team_id = *(int*)((char*)enemy + team_id_offset);
                if (is_dead || is_knocked || !is_visible || is_behind_wall || team_id == my_team) continue;
                struct Vector3 head_pos = GetBonePosition(enemy, head_bone_index);
                struct Vector2 screen;
                if (WorldToScreen(head_pos, &screen)) {
                    float fov = sqrtf(powf(screen.x - 960, 2) + powf(screen.y - 540, 2));
                    if (fov < min_fov) {
                        min_fov = fov;
                        closest_enemy = enemy;
                    }
                }
            }
            if (closest_enemy) {
                struct Vector3 target_head = GetBonePosition(closest_enemy, head_bone_index);
                *(struct Vector3*)((char*)camera_manager + camera_offset) = target_head;
                __android_log_print(ANDROID_LOG_INFO, "Hack", "Aimbot Aiming at FOV %.1f", min_fov);
            }
        }
    }
}

void ESPThread() {
    srand(time(NULL));
    while (true) {
        usleep(16000 + rand() % 1000);
        check_recording();
        struct Vector3 my_pos = *(struct Vector3*)((char*)local_player_base + position_offset);
        for (int i = 0; i < 30; i++) {
            uintptr_t enemy = *(uintptr_t*)((char*)target_list + i * target_size);
            if (!enemy) continue;
            RenderESP(enemy, my_pos);
        }
    }
}

void RenderMenu() {
    if (menuVisible) {
        ImGui::Begin("Hack Menu", &menuVisible);
        ImGui::Checkbox("Enable ESP", &enableESP);
        ImGui::Checkbox("Show Box", &showBox);
        ImGui::Checkbox("Show Name", &showName);
        ImGui::Checkbox("Show Distance", &showDistance);
        ImGui::Checkbox("Show Line", &showLine);
        ImGui::Checkbox("Show HP", &showHP);
        ImGui::Checkbox("Show Status", &showStatus);
        ImGui::Checkbox("Show Skeleton", &showSkeleton);
        ImGui::Checkbox("Wallhack", &wallhack);
        ImGui::Checkbox("Show Weapon", &showWeapon);
        ImGui::Checkbox("Show Radar", &showRadar);
        ImGui::Checkbox("Custom Color", &enableColor);
        if (enableColor) ImGui::ColorEdit4("Box Color", (float*)&boxColor);
        ImGui::Checkbox("Hide on Record", &hideOnRecord);
        ImGui::Separator();
        ImGui::Checkbox("Enable Aimbot", &enableAimbot);
        ImGui::SliderFloat("Aim FOV", &aimFOV, 10.0f, 100.0f);
        ImGui::End();
    }
}

void HackThread() {
    while (true) {
        usleep(16000);
        handle_volume_key();
        RenderMenu();
    }
}

void hack_loop() {
    pthread_t esp_thread, aimbot_thread, hack_thread;
    pthread_create(&esp_thread, NULL, (void*)ESPThread, NULL);
    pthread_create(&aimbot_thread, NULL, (void*)AimbotThread, NULL);
    pthread_create(&hack_thread, NULL, (void*)HackThread, NULL);
    pthread_join(esp_thread, NULL);
    pthread_join(aimbot_thread, NULL);
    pthread_join(hack_thread, NULL);
}

extern "C" int main() {
    ImGui::CreateContext();
    srand(time(NULL));
    hack_loop();
    return 0;
