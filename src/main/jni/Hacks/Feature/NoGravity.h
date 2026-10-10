#pragma once

namespace NoGravityCfg
{
    constexpr uintptr_t Velocity = 0x32C;
    constexpr uintptr_t VerticalSpeed = 0x1304;
    constexpr uintptr_t AirControl = 0x132C;
    constexpr uintptr_t LocalGravity = 0x1348;
    constexpr float MinGravity = 0.05f;
    constexpr float MinSpeed = 0.35f;
    constexpr float MaxSpeed = 4.0f;
    constexpr float MinAirCtrl = 0.08f;
}

inline bool NoGravityOn()
{
    return Config.ExtraMenu.NoGravity;
}

inline float NoGravityScale()
{
    float scale = Config.ExtraMenu.NoGravityScale;
    if (scale < 0.0f)
        return 0.0f;
    if (scale > 1.0f)
        return 1.0f;
    return scale;
}

inline void ApplyNoGravityRuntime()
{
    static Pawn* lastPawn = nullptr;
    static bool wasOn = false;

    if (!NoGravityOn()) {
        lastPawn = nullptr;
        wasOn = false;
        return;
    }

    Pawn* pawn = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(pawn) || !pawn->m_IsAlive()) {
        lastPawn = nullptr;
        return;
    }

    if (pawn != lastPawn || !wasOn) {
        lastPawn = pawn;
    }

    const float gravity = NoGravityScale();
    if (gravity >= 0.995f) {
        wasOn = true;
        return;
    }

    const float grav = NoGravityCfg::MinGravity + gravity * 0.16f;
    const float maxSpeed = NoGravityCfg::MinSpeed + gravity * (NoGravityCfg::MaxSpeed - NoGravityCfg::MinSpeed);
    const float airCtrl = NoGravityCfg::MinAirCtrl + gravity * (1.0f - NoGravityCfg::MinAirCtrl);

    Vector3* velocity = (Vector3*)((uintptr_t)pawn + NoGravityCfg::Velocity);
    float* verticalSpeed = (float*)((uintptr_t)pawn + NoGravityCfg::VerticalSpeed);
    float vy = verticalSpeed != nullptr ? *verticalSpeed : (velocity != nullptr ? velocity->y : 0.0f);

    if (vy > maxSpeed)
        vy = maxSpeed;
    else if (vy < -maxSpeed)
        vy = -maxSpeed;

    if (vy > 0.0f) {
        vy -= grav;
        if (vy < 0.0f)
            vy = 0.0f;
    } else if (vy < 0.0f) {
        vy -= grav;
        if (vy < -maxSpeed)
            vy = -maxSpeed;
    }

    if (velocity != nullptr)
        velocity->y = vy;
    if (verticalSpeed != nullptr)
        *verticalSpeed = vy;

    *(float*)((uintptr_t)pawn + NoGravityCfg::AirControl) = airCtrl;
    *(float*)((uintptr_t)pawn + NoGravityCfg::LocalGravity) = gravity;
    wasOn = true;
}
