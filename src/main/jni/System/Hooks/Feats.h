#pragma once

#include <cstdint>
#include <string>

extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern float SlideRange;
extern bool UnlimitedAmmo;
extern bool UnlimitedFly;   // <-- Unlimited Fly toggle
extern std::chrono::steady_clock::time_point lastAmmoRefillTime;

//-- Smart Reload
namespace SmartReloadCfg
{
    constexpr uintptr_t CarriedAmmoCountRva = 0x50EC6E8;
    constexpr uintptr_t PawnTryChangeClipRva = 0x5262594;
}

inline bool g_SmartReloadPending = false;

inline int SmartReloadCarriedAmmo(void *weapon)
{
    using GetAmmoFn = int (*)(void *);
    static GetAmmoFn getCarriedAmmo = reinterpret_cast<GetAmmoFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::CarriedAmmoCountRva));
    return getCarriedAmmo != nullptr ? getCarriedAmmo(weapon) : 0;
}

inline void SmartReloadRequestChangeClip(Pawn *local)
{
    using TryChangeClipFn = void (*)(Pawn *, bool);
    static TryChangeClipFn tryChangeClip = reinterpret_cast<TryChangeClipFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::PawnTryChangeClipRva));

    if (tryChangeClip != nullptr)
        tryChangeClip(local, true);
}

inline void SmartReloadMarkShotFired()
{
    if (!Config.ExtraMenu.SmartReload)
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive())
        return;

    g_SmartReloadPending = true;
}

inline bool SmartReloadCanRequestForPawn(Pawn *local)
{
    if (!Config.ExtraMenu.SmartReload || !g_SmartReloadPending)
        return false;

    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_SmartReloadPending = false;
        return false;
    }

    Pawn *currentLocal = GamePlay::get_LocalPawn();
    if (currentLocal != local) {
        g_SmartReloadPending = false;
        return false;
    }

    Weapon *weapon = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(weapon)) {
        g_SmartReloadPending = false;
        return false;
    }

    if (SmartReloadCarriedAmmo(weapon) <= 0) {
        g_SmartReloadPending = false;
        return false;
    }

    return true;
}

inline void SmartReloadRequestAfterStopFire(Pawn *local)
{
    if (!SmartReloadCanRequestForPawn(local))
        return;

    g_SmartReloadPending = false;
    SmartReloadRequestChangeClip(local);
}

inline void (*orig_Pawn_StopFire)(Pawn *instance, bool isImmidiately) = nullptr;
inline void hook_Pawn_StopFire(Pawn *instance, bool isImmidiately)
{
    orig_Pawn_StopFire(instance, isImmidiately);
    SmartReloadRequestAfterStopFire(instance);
}


//-- Clear Terrain
inline void (*orig_SetGrassShowState)(bool showGrass) = nullptr;
inline void hook_SetGrassShowState(bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain)
        showGrass = false;
    return orig_SetGrassShowState(showGrass);
}

inline void (*orig_SetGrassLODBias)(float lodBias, float midLodBias, bool showGrass) = nullptr;
inline void hook_SetGrassLODBias(float lodBias, float midLodBias, bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain) {
        lodBias = 0.0f;
        midLodBias = 0.0f;
        showGrass = false;
    }
    return orig_SetGrassLODBias(lodBias, midLodBias, showGrass);
}

inline void ApplyWorldVisualsRuntime()
{
    static bool lastClearTerrain = false;
    if (Config.ExtraMenu.ClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(false);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(0.0f, 0.0f, false);
    } else if (lastClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(true);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(1.0f, 1.0f, true);
    }
    lastClearTerrain = Config.ExtraMenu.ClearTerrain;
}

//-- Aim Assist
inline float (*orig_GetAssitAimSpeed)(void *, Vector3, float, float, float, bool, bool) = nullptr;
inline float GetAssitAimSpeed(void * instance, Vector3 assistCentorPos, float assistDis, float dis, float angle, bool isPVE, bool gamepadInput) {
    if (instance != NULL) {
        if (Config.Aim.AimAssistSize > 0.0f) {
            return (float)Config.Aim.AimAssistSize;
        }
    }
    return orig_GetAssitAimSpeed(instance, assistCentorPos, assistDis, dis, angle, isPVE, gamepadInput);
}

//-- Anti Flashbang
inline void (*orig_OnFlashBangExplode)(void *, int, float, float, float) = nullptr;
inline void hook_OnFlashBangExplode(void *instance, int weaponItemID, float whiteTime, float whiteAlphaTime, float initIntensity) {
    if (instance != nullptr && Config.ExtraMenu.Flash) {
        whiteTime = 0.1f;
        whiteAlphaTime = 0.1f;
        initIntensity = 0.1f;
    }
    orig_OnFlashBangExplode(instance, weaponItemID, whiteTime, whiteAlphaTime, initIntensity);
}

//-- Firerate Speed  (+ Unlimited Fly reuses these hooks)
//
// These three Weapon getters are also the hook points for the NewFeatures
// firing-timing toggles (see NewFeatures.h, Task 3).  There is exactly ONE
// hook per address - the new toggles are folded into these bodies instead of
// installing a second trampoline on the same prologue:
//   get_FireBoltTime    @0x51237F4 (No Fire Delay feature removed per request)
//   get_FireInterval    @0x5105C74 (No Fire Delay feature removed per request)
//   get_DelaySprintFire @0x513CBC4 -> No Sprint Fire Delay (sprint-out timer)
// NewFeatures::NoSprintFireDelayActive() answers "is this the local player's held
// weapon?" from a CACHE refreshed by the local pawn's own tick, because this body
// runs inside game code on every weapon of the world: it must not resolve the local
// pawn or call Pawn::get_CurrentWeapon() itself. That native call from inside the
// getter is what crashed the option (a pawn the game already destroyed passes a plain
// readability check), so the body is now a pure pointer compare - nothing is changed
// for remote players either way.
inline float (*orig_get_FireBoltTime)(void *) = nullptr;
inline float get_FireBoltTime(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.05f;
        }
    }
    return orig_get_FireBoltTime(instance);
}

inline float (*orig_get_FireInterval)(void *) = nullptr;
inline float get_FireInterval(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.05f;
        }
    }
    return orig_get_FireInterval(instance);
}

inline float (*orig_get_DelaySprintFire)(void *) = nullptr;
inline float get_DelaySprintFire(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.00001f;
        }
        if (NewFeatures::NoSprintFireDelayActive(instance)) {
            return 0.00001f;
        }
    }
    return orig_get_DelaySprintFire(instance);
}

typedef void (*SetUltraFrameRateDeviceInfo_t)(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate);
SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 144;
        ultraFrameRateBR = 144;
        ultraFrameRateQualityLimit = 144;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
}

//-- High Jump
inline float (*orig_GetMaxJumpHeight)(void*) = nullptr;
inline float hook_GetMaxJumpHeight(void* instance) {
    float orig_ = orig_GetMaxJumpHeight(instance);
    return (jumpHeightMultiplier > 1.0f) ? orig_ * jumpHeightMultiplier : orig_;
}

//-- Increase Damage
// ---------------------------------------------------------------------------
// Bullet Track (Config.Aim.AimSilent) -- the HIT path, not just the tracer.
//
// The DrawESP.h hooks (WeaponFireComponent_Instant::CreateBulletLine /
// CreateBulletProjectile) only bend the *visible* bullet line towards the
// enemy. Whether a shot REGISTERS is decided by the game's own sweep,
// WeaponFireComponent_Instant::SingleLineCheckPhysics @0xC9B2A9C (Slot 90,
// dump.cs 769628) -- the override a real gun actually dispatches to (the base
// WeaponFireComponent::SingleLineCheckPhysics @0xC1514C0 is the one hooked
// above for Hitbox). Redirecting `dir` on THAT ray is what turns a tracer that
// hits nothing into shots that land.
//
// Guards, so nothing but the local player's own shot is ever touched:
//   * the feature has to be on (AimSilent / Aimbot360), including the Trigger
//     (None / Shooting / Scoping) setting -- exactly the gate the DrawESP
//     tracer handler uses, so ray and tracer always agree;
//   * the fire component must belong to the weapon the local player is holding
//     (WeaponFireComponent::m_WeaponEntity @0x10, dump.cs 764177), so a bot's
//     shot is never redirected;
//   * without a live enemy the original `dir` is passed through untouched and
//     the shot resolves exactly as the game intended.
// ---------------------------------------------------------------------------
inline bool A_BulletTrackIsLocalShot(void *fireComponent)
{
    if (!Tools::IsPtrValid(fireComponent)) return false;

    // WeaponFireComponent::m_WeaponEntity @0x10 (dump.cs 764173/764177).
    // NOTE: there is no owner-pawn field at +0x28 - m_Owner there is a const
    // PropertyName, which is why the weapon pointer is used instead.
    void *weapon = *(void **)((uintptr_t)fireComponent + 0x10);
    if (!Tools::IsPtrValid(weapon)) return false;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local)) return false;

    return (void *)local->get_CurrentWeapon() == weapon;
}

// Same Trigger gate DrawESP.h's WeaponHandlerByAstral uses.
inline bool A_BulletTrackTriggerReady()
{
    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local)) return false;

    if (Config.Aim.Trigger == EAimTrigger::Shooting)
        return ((bool (*)(uintptr_t))(Class_Pawn_get_IsFiring))((uintptr_t)local);
    if (Config.Aim.Trigger == EAimTrigger::Scoping)
        return ((bool (*)(uintptr_t))(Class_Pawn_IsAiming))((uintptr_t)local);
    return true;                                    // EAimTrigger::None
}

// Target head to aim the ray at. Distance mode (the default) takes the enemy
// nearest the shot origin - the same choice GetClosestTarget() makes in
// DrawESP.h; FOV mode takes the enemy whose head lies closest to the ray.
inline bool A_BulletTrackTargetHead(const Vector3 &startPos, const Vector3 &dir, Vector3 &outHead)
{
    auto getMatchGame = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    uintptr_t matchGame = (getMatchGame != nullptr) ? getMatchGame() : 0;
    if (!Tools::IsPtrValid((void *) matchGame)) return false;

    auto EnemyPawns = *(List<uintptr_t> **) (matchGame + Class_BaseGame_EnemyPawns);
    if (!Tools::IsPtrValid((void *) EnemyPawns)) return false;
    auto Items = EnemyPawns->getItems();
    if (!Items) return false;

    const int count = EnemyPawns->getSize();
    if (count <= 0 || count > 1024) return false;

    const bool byFov = (Config.Aim.By == EAim::Crosshair);
    const Vector3 rayAxis = Vector3::Normalized(dir);

    bool found = false;
    float best = 0.0f;
    for (int i = 0; i < count; i++) {
        uintptr_t pawnU = Items[i];
        if (!Tools::IsPtrValid((void *) pawnU)) continue;

        Pawn *pawn = (Pawn *) pawnU;
        if (!pawn->m_IsAlive()) continue;

        const Vector3 head = pawn->get_HeadPosition();
        const Vector3 toHead = head - startPos;

        float metric;
        if (byFov) {
            const float along = Vector3::Dot(toHead, rayAxis);
            if (along <= 0.0f) continue;             // behind the shot
            metric = Vector3::Dot(toHead, toHead) - along * along;  // perpendicular^2
        } else {
            metric = Vector3::SqrMagnitude(toHead);  // nearest to the shooter
        }

        if (!found || metric < best) {
            best = metric;
            outHead = head;
            found = true;
        }
    }
    return found;
}

// Rewrites the shot ray. No-op (dir passed through untouched) unless every
// guard above lines up.
inline void A_BulletTrackAimRay(void *fireComponent, const Vector3 &startPos, Vector3 &dir)
{
    if (!Config.Aim.AimSilent && !Config.Aim.Aimbot360) return;
    if (!A_BulletTrackTriggerReady()) return;
    if (!A_BulletTrackIsLocalShot(fireComponent)) return;

    Vector3 head;
    if (!A_BulletTrackTargetHead(startPos, dir, head)) return;

    const Vector3 toTarget = head - startPos;
    if (!(Vector3::SqrMagnitude(toTarget) > 0.0001f)) return;   // also rejects NaN

    dir = toTarget;
}

inline bool (*orig_SingleLineCheckPhysics_Instant)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;
inline bool SingleLineCheckPhysics_Instant_Hook(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    A_BulletTrackAimRay(instance, startPos, dir);
    if (orig_SingleLineCheckPhysics_Instant != nullptr)
        return orig_SingleLineCheckPhysics_Instant(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
    return false;
}

inline bool (*orig_SingleLineCheckPhysics)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;
inline bool SingleLineCheckPhysics(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Hit) {
            return true;
        }
    }
    return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

//-- Long Slide
inline float (*o_get_SlideTackleAcclerationSpeed)(void*) = nullptr;
inline float h_get_SlideTackleAcclerationSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 1;
    }
    return o_get_SlideTackleAcclerationSpeed(ins);
}

inline float (*o_PawnGetMaxSpeed)(void*) = nullptr;
inline float h_PawnGetMaxSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_PawnGetMaxSpeed(ins);
}

inline float (*o_get_SlideTackleSpeed)(void*) = nullptr;
inline float h_get_SlideTackleSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange;
    }
    return o_get_SlideTackleSpeed(ins);
}

inline float (*o_GetSuperSlideRate)(void*) = nullptr;
inline float h_GetSuperSlideRate(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_GetSuperSlideRate(ins);
}

inline void (*o_TickLocalPlayer)(void*, float) = nullptr;
inline void h_TickLocalPlayer(void* ins, float deltaTime) {
    if (SlideRange <= 0.0f) {
        o_TickLocalPlayer(ins, deltaTime);
    }
}

//-- No Overheat
inline float (*orig_get_AddHotTime)(void* instance) = nullptr;
inline float get_AddHotTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Rpd) {
            return 0.00001f;
        }
    }
    return orig_get_AddHotTime(instance);
}

//-- No Parachute
inline void (*orig_OpenParachute)(void* instance, bool isAuto) = nullptr;
inline void OpenParachute(void* instance, bool isAuto) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Parachute) {
            return;
        }
    }
    return orig_OpenParachute(instance, isAuto);
}

//-- Quick Reload
inline float (*orig_get_ChangeClipTime)(void* instance) = nullptr;
inline float get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipTime(instance);
}

inline float (*orig_get_ChangeClipLoopTime)(void* instance) = nullptr;
inline float get_ChangeClipLoopTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipLoopTime(instance);
}

//-- Quick Scope
inline float (*orig_get_AimingTime)(void* instance) = nullptr;
inline float get_AimingTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Scope) {
            return 0.0001f;
        }
    }
    return orig_get_AimingTime(instance);
}

//-- Quick Switch
inline float (*orig_get_EquipTime)(void* instance) = nullptr;
inline float get_EquipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_EquipTime(instance);
}

inline float (*orig_get_UnequipTime)(void* instance) = nullptr;
inline float get_UnequipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_UnequipTime(instance);
}

//-- Red Wallhack
// get_IsInEM3Eye() @0x9677554 is only 8 bytes long (ldrb w0,[x0,#..]; ret) and
// the very next function in libunity.so is set_IsInEM3Eye() @0x967755C. A64
// inline hooking (hook()/HOOK_LIB/And64InlineHook) overwrites the target with a
// 16-byte LDR/BR stub whenever the replacement sits further than +/-128MB away,
// which wipes out that neighbour. The flag is forced with Patches.Em3Eye
// (mov x0,#1 ; ret) from Main.cpp instead - exactly the 8 bytes of the getter.

float (*orig_GetAccDistance)(void *instance);
float GetAccDistance(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return 50.0f;
        }
    }
    return orig_GetAccDistance(instance);
}

//-- Skip Tutorial
inline bool IsTutorialEnabled() {
    return false;
}

//-- Sky Diving Speed
inline float (*orig_get_AccelerationForwardSpeedUp)(void* instance) = nullptr;
inline float get_AccelerationForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_AccelerationForwardSpeedUp(instance);
}

inline float (*orig_get_MaxVelocityForwardSpeedUp)(void* instance) = nullptr;
inline float get_MaxVelocityForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_MaxVelocityForwardSpeedUp(instance);
}

//-- Snowboard Boost
inline float (*get_m_PhysSkisMaxSpeed)(void*) = nullptr;
inline float hooked_get_m_PhysSkisMaxSpeed(void* instance) {
    if (SnowBsize > 0.0f) {
        return SnowBsize;
    }
    return get_m_PhysSkisMaxSpeed(instance);
}

//-- Speed Hack
inline float (*original_CalcFinalMoveScale)(void*) = nullptr;
inline float hooked_CalcFinalMoveScale(void* instance) {
    if (instance == nullptr) {
        return original_CalcFinalMoveScale(instance);
    }
    if (speedHackMultiplier > 1.0f && speedHackMultiplier <= 100.0f) {
        return speedHackMultiplier;
    }
    return original_CalcFinalMoveScale(instance);
}

//-- Weapon Kinetic
// get_IsKineticArmor() @0x51B3D64 is only 8 bytes long and the very next
// function is the *virtual* Pawn::get_CurrentSeatMode() @0x51B3D6C (0xAC bytes),
// so a 16-byte inline stub here would break every seat query in the game. The
// flag is forced with Patches.KineticArmor (mov x0,#1 ; ret) from Main.cpp.

//-- Zero Recoil
inline float (*orig_GetScaleRecoil)(void* instance) = nullptr;
inline float GetScaleRecoil(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Recoil) {
            return 0.00001f;
        }
    }
    return orig_GetScaleRecoil(instance);
}

//-- Zero Spread
inline float (*orig_MinInaccuracy)(void *) = nullptr;
inline float MinInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MinInaccuracy(instance);
}

inline float (*orig_MaxInaccuracy)(void *) = nullptr;
inline float MaxInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MaxInaccuracy(instance);
}

inline float (*orig_DisperseBase)(void *) = nullptr;
inline float DisperseBase(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_DisperseBase(instance);
} 

// Unlock Blueprints (SKINS tab switch). BlueprintWeaponInfo.IsUnlocked()
// @0x901F988 takes no parameters, so this pointer's shape is the method's own.
inline bool (*orig_IsUnlocked)(void *instance);
inline bool IsUnlocked(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Blueprints) {
            return true;
        }
    }
    return orig_IsUnlocked(instance);
}

// Mythic Attachments (unlock-as-if-owned) lives in its own header: the four
// client-side gates that decide whether an applied skin and the weapon's
// attachments count as unlocked - WeaponPromotionInfo.IsWeaponAttachmentUnlock
// 0xA61AA00, WeaponPromotionInfo.IsWeaponSkinUnlock 0xA61AD54,
// LoadoutWeaponsUIHelper.IsThisAttachmentUnlock 0x5D40528 and
// WeaponAttr.IsAttachUnlock 0xA188634. See System/Hooks/MythicAttachments.h.
#include "MythicAttachments.h"

// No Spectate Deelay
inline bool (*o_NeedDelayProcess)(void *instance, int a, int b);
inline bool h_NeedDelayProcess(void *instance, int a, int b) {
    bool orig_val = o_NeedDelayProcess(instance, a, b);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

inline bool (*o_ProcessForDelay)(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex);
inline bool h_ProcessForDelay(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex) {
    bool orig_val = o_ProcessForDelay(instance, packData, PackDataCount, Sequence, timeQueued, aiIndex);

    if (Config.ExtraMenu.Spectatex) {
        return false;
    }

    return orig_val;
}

inline bool (*o_get_IsObserver)(void *);
inline bool h_get_IsObserver(void *instance) {
    bool orig_val = o_get_IsObserver(instance);

    if (Config.ExtraMenu.Spectatex) {
        return true;
    }

    return orig_val;
}

inline float (*o_GetDelayCountDown)(void *);
inline float h_GetDelayCountDown(void *instance) {
    float orig_val = o_GetDelayCountDown(instance);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

int (*orig_get_DeviceCapacityLevel)(void *instance);
int get_DeviceCapacityLevel(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Grap) {
            return 6;
        }
    }
    return orig_get_DeviceCapacityLevel(instance);
}

// Hook for CheckTargetIsValid_DyingInAzurGameMode
inline bool (*orig_CheckTargetIsValid_DyingInAzurGameMode)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_DyingInAzurGameMode_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_DyingInAzurGameMode(instance, targetPawn);
}

// Hook for CheckTargetIsValid_PhysState
inline bool (*orig_CheckTargetIsValid_PhysState)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_PhysState_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_PhysState(instance, targetPawn);
}

// Hook for CheckTargetIsValid_ManualParameter
inline bool (*orig_CheckTargetIsValid_ManualParameter)(void* instance, void* tempPawn, float* distance);
inline bool CheckTargetIsValid_ManualParameter_Hook(void* instance, void* tempPawn, float* distance) {
    if (instance != nullptr && tempPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_ManualParameter(instance, tempPawn, distance);
}

// Hook for CheckTargetIsValid_Ult
inline bool (*orig_CheckTargetIsValid_Ult)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_Ult_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_Ult(instance, targetPawn);
}

inline bool (*orig_CalculateObstacleSide)(void* instance, float* backDis, float* beforeDis);
inline bool CalculateObstacleSide_Hook(void* instance, float* backDis, float* beforeDis) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleSide(instance, backDis, beforeDis);
}

inline bool (*orig_CalculateObstacleTop)(void* instance, Vector3 pawnPos);
inline bool CalculateObstacleTop_Hook(void* instance, Vector3 pawnPos) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleTop(instance, pawnPos);
}

int (*oCheckTargetIsValid)(void *ins, void* tempPawn, float distance);
int CheckTargetIsValid(void *ins, void* tempPawn, float distance) {
    if ( ins != nullptr ) {
        if (isExecute) {
            return (int)true;
        }
    }
    return oCheckTargetIsValid(ins, tempPawn, distance);
}

inline bool (*oCheckCanExcution_Extra)(void *ins);
inline bool CheckCanExcution_Extra(void *ins) {
    if ( ins != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return oCheckCanExcution_Extra(ins);
}


inline bool (*o_CheckExecution_ObstacleAround)(void *ins);
inline bool CheckExecution_ObstacleAround(void *ins) {
    if (ins!=nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return o_CheckExecution_ObstacleAround(ins);
}

/// Walk Under Water
inline bool (*oPawn_GetCurrentDistToWaterSurface)(void *instance);
inline bool Pawn_GetCurrentDistToWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPawn_GetCurrentDistToWaterSurface(instance);
}

inline bool (*oPlayerPawn_IsUnderWaterSurface)(void *instance);
inline bool PlayerPawn_IsUnderWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPlayerPawn_IsUnderWaterSurface(instance);
}

inline bool (*oCheckInWaterComponent_get_CurrentWaterSurfaceHeight)(void *instance);
inline bool CheckInWaterComponent_get_CurrentWaterSurfaceHeight(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return true;
        }
    }
    return oCheckInWaterComponent_get_CurrentWaterSurfaceHeight(instance);
}

///Camera Pov
float (*oInputSettingConfig_Instant_GetMainCameraFov_3p)(void *instance);
float InputSettingConfig_Instant_GetMainCameraFov_3p(void *instance) {
    if (instance && Config.ExtraMenu.CameraPov) {
        return (float)Config.ExtraMenu.CameraPovSize;
    }
    return oInputSettingConfig_Instant_GetMainCameraFov_3p(instance);
}

//-- No Smoke
inline bool NameHasSmokeToken(const char *name)
{
    if (!name)
        return false;

    std::string s(name);
    for (char &c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }

    return s.find("smoke") != std::string::npos ||
           s.find("smog") != std::string::npos ||
           s.find("smk") != std::string::npos;
}

inline String *(*UnityObject_get_name)(void *instance) = nullptr;
inline void (*orig_GameObjectSetActive)(void *instance, bool value) = nullptr;
inline void hook_GameObjectSetActive(void *instance, bool value)
{
    if (Config.ExtraMenu.NoSmoke && value && instance && UnityObject_get_name) {
        String *name = UnityObject_get_name(instance);
        if (name && NameHasSmokeToken(name->CString()))
            value = false;
    }
    return orig_GameObjectSetActive(instance, value);
}

inline void InitializeAllHooks() {

    //-- Smart Reload
    HOOK_LIB("libunity.so", "0x5260B78", hook_Pawn_StopFire, orig_Pawn_StopFire);

    //-- Aim Assist
    HOOK_LIB("libunity.so", "0x666FD90", GetAssitAimSpeed, orig_GetAssitAimSpeed);

    //-- Anti Flashbang
    HOOK_LIB("libunity.so", "0x51E977C", hook_OnFlashBangExplode, orig_OnFlashBangExplode);
    
    //-- Firerate Speed  (+ Unlimited Fly reuses these hooks)
    HOOK_LIB("libunity.so", "0x51237F4", get_FireBoltTime, orig_get_FireBoltTime);
    HOOK_LIB("libunity.so", "0x5105C74", get_FireInterval, orig_get_FireInterval);
    HOOK_LIB("libunity.so", "0x513CBC4", get_DelaySprintFire, orig_get_DelaySprintFire);

    //-- High Jump
    HOOK_LIB("libunity.so", "0x5221D00", hook_GetMaxJumpHeight, orig_GetMaxJumpHeight);

    //-- Increase Damage
    HOOK_LIB("libunity.so", "0xC1514C0", SingleLineCheckPhysics, orig_SingleLineCheckPhysics);

    //-- Bullet Track (silent aim): the hit ray a real gun dispatches to.
    //   See the A_BulletTrack* block above - this is what makes the redirected
    //   shot actually register instead of only bending the tracer.
    HOOK_LIB("libunity.so", "0xC9B2A9C", SingleLineCheckPhysics_Instant_Hook, orig_SingleLineCheckPhysics_Instant);

    //-- No Parachute
    HOOK_LIB("libunity.so", "0x68823FC", OpenParachute, orig_OpenParachute);

    //-- Quick Reload
    HOOK_LIB("libunity.so", "0x50ECADC", get_ChangeClipTime, orig_get_ChangeClipTime);

    //-- Quick Scope
    HOOK_LIB("libunity.so", "0x50EB944", get_AimingTime, orig_get_AimingTime);

    //-- Quick Switch
    HOOK_LIB("libunity.so", "0x50ED8D4", get_EquipTime, orig_get_EquipTime);

    //-- Red Wallhack
    //   get_IsInEM3Eye @0x9677554 is an 8-byte getter: never inline-hook it,
    //   it stays a patch (Patches.Em3Eye, Main.cpp).
    HOOK_LIB("libunity.so", "0xAD1DD78", GetAccDistance, orig_GetAccDistance);

    //-- Skip Tutorial
    HOOK_LIB_NO_ORIG("libunity.so", "0x9DE0E58", IsTutorialEnabled);
    
    //-- Sky Diving Speed
    HOOK_LIB("libunity.so", "0x5DE981C", get_AccelerationForwardSpeedUp, orig_get_AccelerationForwardSpeedUp);
    HOOK_LIB("libunity.so", "0x5DE9880", get_MaxVelocityForwardSpeedUp, orig_get_MaxVelocityForwardSpeedUp);

    //-- Snowboard Boost
    HOOK_LIB("libunity.so", "0x522860C", hooked_get_m_PhysSkisMaxSpeed, get_m_PhysSkisMaxSpeed);

    //-- Speed Hack
    HOOK_LIB("libunity.so", "0x51D2EB8", hooked_CalcFinalMoveScale, original_CalcFinalMoveScale);

    //-- Weapon Kinetic
    //   get_IsKineticArmor @0x51B3D64 is an 8-byte getter: never inline-hook it,
    //   it stays a patch (Patches.KineticArmor, Main.cpp).

    //-- Zero Recoil
    HOOK_LIB("libunity.so", "0xC9BAFF8", GetScaleRecoil, orig_GetScaleRecoil);

    //-- Zero Spread
    HOOK_LIB("libunity.so", "0xC9B8F78", MinInaccuracy, orig_MinInaccuracy);
    HOOK_LIB("libunity.so", "0xC159288", MaxInaccuracy, orig_MaxInaccuracy);
    HOOK_LIB("libunity.so", "0xC9C76C4", DisperseBase, orig_DisperseBase);
    
    //-- Spectate
    // NOTE: ObserverDelayManager::ProcessForDelay @0x519676C must be hooked
    // exactly ONCE. It used to be hooked twice into the same orig pointer; the
    // second A64 hook built its trampoline from the *already patched* prologue,
    // so o_ProcessForDelay ended up branching back into h_ProcessForDelay, which
    // calls o_ProcessForDelay - unbounded recursion (stack overflow / SIGSEGV)
    // the first time the game ran ProcessForDelay.
    HOOK_LIB("libunity.so", "0x519676C", h_ProcessForDelay, o_ProcessForDelay);
    HOOK_LIB("libunity.so", "0x51966D8", h_NeedDelayProcess, o_NeedDelayProcess);

    //-- No smoke
    UnityObject_get_name = reinterpret_cast<String *(*)(void *)>(
          getAbsoluteAddress("libunity.so", 0x5052F10));

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x5036EB8),
          (void *)&hook_GameObjectSetActive,
          (void **)&orig_GameObjectSetActive);
          
    //-- Throwable Alert      
    DobbyHook((void *)getAbsoluteAddress("libunity.so", ThrowAlertCfg::TickRva),
          (void *)&hook_WeaponProjectileTick,
          (void **)&orig_WeaponProjectileTick);
          
    //-- Clear Terrain
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xAE2E1B0),
          (void *)&hook_SetGrassShowState,
          (void **)&orig_SetGrassShowState);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x7B4E290),
          (void *)&hook_SetGrassLODBias,
          (void **)&orig_SetGrassLODBias);         
          
    DobbyHook((void*)getAbsoluteAddress("libunity.so", 0x9FDEE28), (void*)hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

    ///Walk Under Water
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x51D31BC), (void *)  &Pawn_GetCurrentDistToWaterSurface, (void **) &oPawn_GetCurrentDistToWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x54BC450), (void *)  &PlayerPawn_IsUnderWaterSurface, (void **) &oPlayerPawn_IsUnderWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x757162C), (void *)  &CheckInWaterComponent_get_CurrentWaterSurfaceHeight, (void **) &oCheckInWaterComponent_get_CurrentWaterSurfaceHeight); 

  ///Camera Pov
  DobbyHook((void *) getAbsoluteAddress("libunity.so", 0xC347950), (void *)  &InputSettingConfig_Instant_GetMainCameraFov_3p, (void **) &oInputSettingConfig_Instant_GetMainCameraFov_3p); 

    //-- Unlock Blueprints (SKINS tab) at BlueprintWeaponInfo.IsUnlocked()
    //   @0x901F988 - no parameters.
    //-- Mythic attachments (HOME tab switch, ON by default) installs its four
    //   gates itself: see MythicAttachments.h. WeaponPromotionInfo.
    //   IsWeaponAttachmentUnlock(int attachment) @0xA61AA00 is the first of them.
    HOOK_LIB("libunity.so", "0x901F988", IsUnlocked, orig_IsUnlocked);

    //-- Unlock Attachments / Mythic Attachments (see MythicAttachments.h). The
    //   RVAs are the dump's, same convention as every hook above.
    HOOK_LIB("libunity.so", "0xA61AA00", IsWeaponAttachmentUnlock, orig_IsWeaponAttachmentUnlock);
    HOOK_LIB("libunity.so", "0xA61AD54", IsWeaponSkinUnlock, orig_IsWeaponSkinUnlock);
    HOOK_LIB("libunity.so", "0x5D40528", IsThisAttachmentUnlock, orig_IsThisAttachmentUnlock);
    HOOK_LIB("libunity.so", "0xA188634", IsAttachUnlock, orig_IsAttachUnlock);

    //-- Mythic Attachments, LAYER 2: the client-side DATA writes that make an
    //   applied skin behave like really owned (the four answers above were
    //   never asked on device - they are small enough to be inlined into their
    //   callers). These three are the builders / SetData of the very screen
    //   that draws the lock, so the lock is rewritten instead of answered.
    HOOK_LIB("libunity.so", "0xB6323C0", GetAttachmentList, orig_GetAttachmentList);
    HOOK_LIB("libunity.so", "0xB83F1C0", AttachmentItemSetData, orig_AttachmentItemSetData);
    HOOK_LIB("libunity.so", "0xB83D5E0", AttachmentCellSetData, orig_AttachmentCellSetData);

    // Which of those seven addresses actually took the hook? The hooker only
    // fills the original pointer on success, and the SKINS tab prints these -
    // a 0 there is the reason that gate's asked/granted counter stays at 0/0.
    mythic_attachments::NoteLanded(mythic_attachments::GateAttachment, orig_IsWeaponAttachmentUnlock != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateSkin,       orig_IsWeaponSkinUnlock != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateLoadout,    orig_IsThisAttachmentUnlock != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateConfirm,    orig_IsAttachUnlock != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateList,       orig_GetAttachmentList != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateItem,       orig_AttachmentItemSetData != nullptr);
    mythic_attachments::NoteLanded(mythic_attachments::GateCell,       orig_AttachmentCellSetData != nullptr);
}