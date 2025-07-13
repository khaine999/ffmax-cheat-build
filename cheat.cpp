// cheat.cpp
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

// ImGui includes (bạn phải để thư mục imgui vào cùng project hoặc đường dẫn include đúng)
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#include <cmath>
#include <vector>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FF_CHEAT", __VA_ARGS__)

// == Config
bool enableAimbot = true;
bool enableESP = true;
float aimFOV = 90.0f;
bool showMenu = true;
ImVec4 espColorEnemy = ImVec4(1.f, 0.f, 0.f, 1.f);

// == Vector3 struct
struct Vector3 {
    float x, y, z;
};

// == Matrix4x4 struct
struct Matrix4x4 {
    float m[16];
};

// == Offsets (OB49 64-bit example)
uintptr_t TargetList = 0x385E0B0;
uintptr_t CameraManager = 0x2B3A9AC;
uintptr_t LocalPlayerBase = 0x385E098;
uintptr_t ViewMatrix = 0x2B17C8C;

// == WorldToScreen function
bool WorldToScreen(const Vector3& world, Vector3& screen, float width, float height) {
    Matrix4x4 matrix = *(Matrix4x4*)(ViewMatrix);

    float w = matrix.m[3]*world.x + matrix.m[7]*world.y + matrix.m[11]*world.z + matrix.m[15];
    if (w < 0.1f) return false;

    float x = matrix.m[0]*world.x + matrix.m[4]*world.y + matrix.m[8]*world.z + matrix.m[12];
    float y = matrix.m[1]*world.x + matrix.m[5]*world.y + matrix.m[9]*world.z + matrix.m[13];

    screen.x = (width / 2.f) + (x / w) * (width / 2.f);
    screen.y = (height / 2.f) - (y / w) * (height / 2.f);
    screen.z = 0.f;

    return true;
}

float GetDistance(Vector3 a, Vector3 b) {
    return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2) + powf(a.z - b.z, 2));
}

bool FirePressed() {
    uintptr_t local = *(uintptr_t*)LocalPlayerBase;
    if (!local) return false;
    return *(bool*)(local + 0x158);
}

Vector3 GetBonePosition(uintptr_t enemy, int boneOffset) {
    uintptr_t boneBase = *(uintptr_t*)(enemy + 0x200);
    uintptr_t bone = *(uintptr_t*)(boneBase + boneOffset);
    return *(Vector3*)(bone + 0x34);
}

// == Aimbot thread
void* AimbotThread(void*) {
    while(true) {
        if (!enableAimbot) {
            usleep(50000);
            continue;
        }

        Vector3 localCam = *(Vector3*)(CameraManager + 0x10);
        float closestDist = 9999.f;
        Vector3 targetHead = {};

        for (int i = 0; i < 30; i++) {
            uintptr_t enemy = *(uintptr_t*)(TargetList + i*0x8);
            if (!enemy) continue;

            bool isDead = *(bool*)(enemy + 0xE0);
            bool isKnocked = *(bool*)(enemy + 0x2D8);
            bool isVisible = *(bool*)(enemy + 0x11C);
            bool isBehindWall = *(bool*)(enemy + 0x118);
            int teamID = *(int*)(enemy + 0x3C);

            if (isDead || isKnocked || !isVisible || isBehindWall || teamID == *(int*)(LocalPlayerBase + 0x3C))
                continue;

            Vector3 head = GetBonePosition(enemy, 0x70);
            float dist = GetDistance(localCam, head);

            if (dist < closestDist && dist < aimFOV) {
                closestDist = dist;
                targetHead = head;
            }
        }

        if (closestDist < 9999.f && FirePressed()) {
            *(Vector3*)(CameraManager + 0x10) = targetHead;
        }

        usleep(5000);
    }
    return nullptr;
}

// == Draw ESP Box
void DrawESP(ImDrawList* drawList, float width, float height) {
    for (int i=0; i<30; i++) {
        uintptr_t enemy = *(uintptr_t*)(TargetList + i*0x8);
        if (!enemy) continue;

        bool isDead = *(bool*)(enemy + 0xE0);
        int teamID = *(int*)(enemy + 0x3C);

        if (isDead || teamID == *(int*)(LocalPlayerBase + 0x3C))
            continue;

        Vector3 pos = *(Vector3*)(enemy + 0x34);
        Vector3 screenPos;

        if (!WorldToScreen(pos, screenPos, width, height))
            continue;

        float boxHeight = 150.f / (pos.z + 1.f);
        float boxWidth = boxHeight / 2.f;

        ImVec2 topLeft(screenPos.x - boxWidth/2.f, screenPos.y - boxHeight);
        ImVec2 bottomRight(screenPos.x + boxWidth/2.f, screenPos.y);

        drawList->AddRect(topLeft, bottomRight, ImColor(espColorEnemy), 0, 0, 2.f);
        drawList->AddText(ImVec2(screenPos.x - 30.f, screenPos.y - boxHeight - 15.f), IM_COL32(255,255,255,255), "Enemy");
    }
}

// == Hook render OpenGL (ví dụ hook eglSwapBuffers)
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
        ImGui::SliderFloat("Aim FOV", &aimFOV, 0.f, 360.f);
        ImGui::Checkbox("Enable ESP", &enableESP);
        ImGui::ColorEdit4("ESP Color", (float*)&espColorEnemy);
        ImGui::Checkbox("Show Menu", &showMenu);
        ImGui::End();
    }

    if (enableESP)
        DrawESP(ImGui::GetBackgroundDrawList(), 1080.f, 1920.f);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// == Constructor để tạo thread aimbot
__attribute__((constructor))
void init() {
    pthread_t aimThread;
    pthread_create(&aimThread, nullptr, AimbotThread, nullptr);
    LOGI("[+] FF MAX cheat loaded");
