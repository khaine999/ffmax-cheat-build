#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FF_CHEAT", __VA_ARGS__)

// Một số biến cấu hình cheat
bool enableAimbot = true;
bool enableESP = true;
bool showMenu = true;
ImVec4 espColorEnemy = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

// Các offset (ví dụ, bạn cần chỉnh theo game thực tế)
uintptr_t TargetList = 0x385E0B0;
uintptr_t CameraManager = 0x2B3A9AC;
uintptr_t LocalPlayerBase = 0x385E098;
uintptr_t ViewMatrix = 0x2B17C8C;

// Vector 3D
struct Vector3 {
    float x, y, z;
};

// Ma trận 4x4
struct Matrix4x4 {
    float m[16];
};

// Chuyển từ tọa độ thế giới sang màn hình
bool WorldToScreen(const Vector3& world, Vector3& screen, float width, float height) {
    Matrix4x4 matrix = *(Matrix4x4*)(ViewMatrix);

    float w = matrix.m[3] * world.x + matrix.m[7] * world.y + matrix.m[11] * world.z + matrix.m[15];
    if (w < 0.1f) return false;

    float x = matrix.m[0] * world.x + matrix.m[4] * world.y + matrix.m[8] * world.z + matrix.m[12];
    float y = matrix.m[1] * world.x + matrix.m[5] * world.y + matrix.m[9] * world.z + matrix.m[13];

    screen.x = (width / 2.0f) + (x / w) * (width / 2.0f);
    screen.y = (height / 2.0f) - (y / w) * (height / 2.0f);
    screen.z = 0;

    return true;
}

// Vẽ ESP box đơn giản bằng ImGui
void DrawESP(ImDrawList* drawList, float width, float height) {
    for (int i = 0; i < 30; i++) {
        uintptr_t enemy = *(uintptr_t*)(TargetList + i * 0x8);
        if (!enemy) continue;

        bool isDead = *(bool*)(enemy + 0xE0);
        int teamID = *(int*)(enemy + 0x3C);
        if (isDead || teamID == *(int*)(LocalPlayerBase + 0x3C)) continue;

        Vector3 pos = *(Vector3*)(enemy + 0x34);
        Vector3 screenPos;
        if (!WorldToScreen(pos, screenPos, width, height)) continue;

        float boxHeight = 150.0f / (pos.z + 1.0f);
        float boxWidth = boxHeight / 2.0f;

        ImVec2 topLeft(screenPos.x - boxWidth / 2, screenPos.y - boxHeight);
        ImVec2 bottomRight(screenPos.x + boxWidth / 2, screenPos.y);

        drawList->AddRect(topLeft, bottomRight, ImColor(espColorEnemy), 0, 0, 2.0f);
        drawList->AddText(ImVec2(screenPos.x - 30, screenPos.y - boxHeight - 15), IM_COL32(255,255,255,255), "Enemy");
    }
}

// Hook render OpenGL để vẽ ImGui menu + ESP
bool imgui_initialized = false;
void HookRender() {
    if (!imgui_initialized) {
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 100");
        ImGui_ImplAndroid_Init(nullptr);
        imgui_initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (showMenu) {
        ImGui::Begin("FF MAX Cheat Menu");
        ImGui::Checkbox("Enable Aimbot", &enableAimbot);
        ImGui::Checkbox("Enable ESP", &enableESP);
        ImGui::ColorEdit4("ESP Color", (float*)&espColorEnemy);
        ImGui::Checkbox("Show Menu", &showMenu);
        ImGui::End();
    }

    if (enableESP)
        DrawESP(ImGui::GetBackgroundDrawList(), 1080.0f, 1920.0f);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Thread aimbot (giả lập đơn giản)
void* AimbotThread(void*) {
    while (true) {
        if (!enableAimbot) {
            usleep(50000);
            continue;
        }
        // TODO: thêm logic aimbot ở đây
        usleep(5000);
    }
    return nullptr;
}

__attribute__((constructor))
void init() {
    pthread_t aimThread;
    pthread_create(&aimThread, nullptr, AimbotThread, nullptr);
    LOGI("[+] FF MAX cheat ESP + Aimbot loaded");
