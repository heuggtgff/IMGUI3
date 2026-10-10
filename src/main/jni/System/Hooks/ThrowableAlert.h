#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ThrowAlertCfg {
    constexpr uintptr_t PosRva = 0xB1F087C;       // WeaponProjectile.Position
    constexpr uintptr_t TickRva = 0xB200314;      // WeaponProjectile.Tick(float)
    constexpr uintptr_t Exploded = 0xA1;          // WeaponProjectile.bHasExplode
    constexpr uintptr_t DamageRadius = 0xA8;      // WeaponProjectile.m_DamageRadius
    constexpr uintptr_t ItemId = 0xE4;            // WeaponProjectile.m_WeaponItemID
    constexpr uintptr_t LethalRadius = 0x200;     // WeaponProjectile.m_LethalRadius
    constexpr float MaxDist = 70.0f;
    constexpr float WarnDist = 35.0f;
}

struct ThrowInfo {
    std::vector<Vector3> path;
    Vector3 pos;
    float radius;
    int itemId;
    uint64_t seen;
};

inline void (*orig_WeaponProjectileTick)(void*, float) = nullptr;
inline std::unordered_map<uintptr_t, ThrowInfo> g_Throws;
inline uint64_t g_ThrowSeen = 0;

inline bool ThrowAlertOn()
{
    return Config.ESPMenu.GrenadeWarn;
}

inline const char* ThrowName(int itemId)
{
    return itemId > 0 ? "THROWABLE" : "PROJECTILE";
}

inline Vector3 GetThrowPos(void* proj)
{
    using Fn = Vector3 (*)(void*);
    static Fn fn = (Fn)getAbsoluteAddress("libunity.so", ThrowAlertCfg::PosRva);
    return fn ? fn(proj) : Vector3::zero();
}

inline void TrackThrowable(void* proj)
{
    if (!Tools::IsPtrValid(proj))
        return;

    bool exploded = *(bool*)((uintptr_t)proj + ThrowAlertCfg::Exploded);
    if (exploded) {
        g_Throws.erase((uintptr_t)proj);
        return;
    }

    float radius = *(float*)((uintptr_t)proj + ThrowAlertCfg::LethalRadius);
    if (radius <= 0.25f)
        radius = *(float*)((uintptr_t)proj + ThrowAlertCfg::DamageRadius);
    if (radius <= 0.25f || radius > 80.0f)
        return;

    Vector3 pos = GetThrowPos(proj);
    if (pos == Vector3::zero())
        return;

    ThrowInfo& info = g_Throws[(uintptr_t)proj];
    info.pos = pos;
    info.radius = radius;
    info.itemId = *(int*)((uintptr_t)proj + ThrowAlertCfg::ItemId);
    info.seen = ++g_ThrowSeen;

    if (info.path.empty() || Vector3::Distance(info.path.back(), pos) > 0.25f) {
        if (info.path.size() >= 80)
            info.path.erase(info.path.begin());
        info.path.push_back(pos);
    }
}

inline bool ThrowToScreen(Camera* cam, const Vector3& world, int height, ImVec2& out)
{
    Vector3 screen = cam->WorldToScreenPoint(world);
    if (screen.z <= 0.01f)
        return false;
    out = ImVec2(screen.x, height - screen.y);
    return true;
}

inline void DrawThrowRadius(ImDrawList* draw, Camera* cam, const Vector3& center, float radius, int height)
{
    constexpr int segs = 56;
    std::vector<ImVec2> ring;
    ring.reserve(segs);

    float r = ImClamp(radius, 1.0f, 18.0f);
    for (int i = 0; i < segs; ++i) {
        float a = (2.0f * IM_PI * i) / segs;
        ImVec2 p;
        if (!ThrowToScreen(cam, Vector3(center.x + cosf(a) * r, center.y + 0.04f, center.z + sinf(a) * r), height, p))
            return;
        ring.push_back(p);
    }

    draw->AddConvexPolyFilled(ring.data(), (int)ring.size(), IM_COL32(255, 72, 56, 28));
    draw->AddPolyline(ring.data(), (int)ring.size(), IM_COL32(255, 72, 56, 210), true, 2.0f);
}

inline void DrawThrowableAlert(ImDrawList* draw, int width, int height, const Vector3& localPos)
{
    if (!ThrowAlertOn())
        return;

    Camera* cam = Camera::get_main();
    if (!cam)
        return;

    for (auto it = g_Throws.begin(); it != g_Throws.end();) {
        ThrowInfo& info = it->second;
        if (g_ThrowSeen > info.seen + 360) {
            it = g_Throws.erase(it);
            continue;
        }

        float dist = Vector3::Distance(localPos, info.pos);
        if (dist > ThrowAlertCfg::MaxDist) {
            ++it;
            continue;
        }

        for (size_t i = 1; i < info.path.size(); ++i) {
            ImVec2 a, b;
            if (ThrowToScreen(cam, info.path[i - 1], height, a) && ThrowToScreen(cam, info.path[i], height, b))
                draw->AddLine(a, b, IM_COL32(255, 255, 255, 210), 2.0f);
        }

        ImVec2 pos;
        if (ThrowToScreen(cam, info.pos, height, pos)) {
            DrawThrowRadius(draw, cam, info.pos, info.radius, height);
            char text[64];
            snprintf(text, sizeof(text), "%s | %dm", ThrowName(info.itemId), (int)dist);
            DrawTextWithBorder1(draw, text, {pos.x - 36.0f, pos.y - 18.0f}, IM_COL32(255, 226, 64, 255), IM_COL32(0, 0, 0, 255), 18.0f);

            if (dist <= ThrowAlertCfg::WarnDist) {
                DrawTextWithBorder1(draw, "THROWABLE WARNING..!", {(float)width * 0.5f - 120.0f, 125.0f}, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255), 22.0f);
            }
        }

        ++it;
    }
}

inline void hook_WeaponProjectileTick(void* instance, float deltaTime)
{
    if (orig_WeaponProjectileTick)
        orig_WeaponProjectileTick(instance, deltaTime);
    TrackThrowable(instance);
}