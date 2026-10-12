#pragma once

// ============================================================================
//  NewFeatures.h  -  combat (Triggerbot) + sniper (Stable Scope) features
//
//  EVERY address/offset below was resolved from "dump (GR .57).cs"
//  (huggingface.co/datasets/jeudhehs/dump).  The libanogs(.57).so.lst file in
//  the same dataset is an IDA listing of the *anti-cheat* library and contains
//  no game symbols (no CheckTarget / Breathe / Shake / Scope entries), so it is
//  not usable as a source for gameplay functions - dump.cs is.
//
//  ---------------------------------------------------------------------------
//  TRIGGERBOT  (hook the game's own "enemy under crosshair" check - NOT
//               CreateBulletProjectile, which only runs on an already firing
//               weapon, i.e. too late)
//  ---------------------------------------------------------------------------
//   WeaponFireComponent_Instant  (TypeDefIndex 16555, dump line 851046)
//     CheckTarget        RVA 0xC9C1B50  dump 851720
//        protected bool CheckTarget(AttackableTarget target, Vector3 startPos,
//                                   Vector3 rayDir, int flag, float range)
//     HasEnemyInPort     RVA 0xC9C13F8  dump 851716   (Slot 122)
//        public virtual bool HasEnemyInPort(bool traceAssist = false)
//        -> second, independent "enemy inside the aim port / crosshair"
//           signal, so the trigger still works on builds where aim assist
//           short-circuits CheckTarget.
//     m_TargetPawn        0x668  (protected Pawn,        dump 851166)
//     m_AimAssistDis      0x628  (protected float[],     dump 851132)
//     m_TargetPawnDist    0x690  (protected float,       dump 851169)
//     get_IsAiming()      RVA 0xC9AF4E0  dump 851428 (weapon level ADS flag)
//
//     !!  The task brief said "TargetPawn at 0x688".  That is stale: in this
//     !!  build 0x684 is m_HitAimAssistancePos (Vector3) and 0x690 is
//     !!  m_TargetPawnDist, so 0x688 is the Z float in the middle of that
//     !!  Vector3 - reading a Pawn there would dereference garbage.  The real
//     !!  field is 0x668 and that is what this file uses.
//
//   WeaponFireComponent (base class, TypeDefIndex 16477, dump 846121)
//     m_Owner              0x28  (protected Pawn)
//     m_ClipAmmoCount      0xA8
//     AmmoCanFire()        RVA 0xC14DF24  (Slot 58)  <- game's own "can fire"
//     StartFire()          RVA 0xC14C29C  (Slot 39)  <- firing entry point
//                            (WeaponFireComponent_Instant does not override
//                             StartFire, so the base address is exactly what
//                             the instant weapon's vtable slot 39 dispatches to)
//
//   Pawn (TypeDefIndex 16146, dump 793452)
//     get_TeamId()         RVA 0x51B89CC  (Slot 216, dump 796719)
//     IsSameCamp(Pawn)     RVA 0x51EF174  (Slot  84, dump 798513)
//     m_IsAlive            0x548
//
//  ---------------------------------------------------------------------------
//  STABLE SCOPE  (the old "No Drift" only pinned m_AimmingBreatheCountDown
//                0x774, which is *breathing*.  Movement sway and the landing
//                punch are different systems.)
//  ---------------------------------------------------------------------------
//   Breathing
//     WeaponFireComponent_Instant::GetBreatheTremblingPos  RVA 0xC9BD880
//        protected virtual Vector2 GetBreatheTremblingPos()  (Slot 116)
//     m_AimmingBreatheCountDown  0x774
//     m_BreatheTremblingPos      0x5A0
//
//   Movement / locomotion sway + landing shake mode (the system that moves the
//   reticle while strafing and dips it on landing):
//     WeaponShakeController::CollectWeaponShakeOffsets  RVA 0x9B774C0
//        (TypeDefIndex 20710, dump 1066212)
//        private WeaponShakeOffsets CollectWeaponShakeOffsets(float dt, float weight)
//        WeaponShakeOffsets is the struct that carries
//          rotationOffset (0x00)  positionOffset (0x0C)
//          AimRotationOffset (0x18)  gunKickAimRotationOffset (0x24)
//          crosshairOffset (0x30)    <-- the crosshair/scope reticle offset
//          ikWeaponRootRotationOffset (0x3C) ikWeaponRootPositionOffset (0x48)
//          pawnRootRotationOffset (0x54)     pawnRootPositionOffset (0x60)
//        WeaponShakeController::m_Pawn = 0x78  (owner pawn)
//        (the controller also tracks m_PrePawnPhysicsState / m_CurPawnPhysicsState
//         at 0x150/0x154, i.e. walk/jump/land shake modes)
//
//   Lateral 1P camera shift while strafing:
//     Pawn::UpdateFirstPersonCameraOffset(float)  RVA 0x51C489C  (dump 797151)
//        writes m_FirstPersonCameraCurrentOffsetX (0x6FC)
//               m_FirstPersonCameraDestOffsetX    (0x700)
//
//   Landing impact ("crosshair dips down and springs back"):
//     HitGroundComponent::OnJumpHitGround(float fLandHeight, bool bIsUltLanding)
//        RVA 0x8599460  (TypeDefIndex 20394, dump 1050352)
//     HitGroundComponent : PawnComponent -> PawnComponent::m_Pawn = 0x20
//
//   Pawn::IsAiming() RVA 0x519BB10  (dump 803813, == Defines.h api15)
//        -> "the local player is scoping / hardscoping"
//
//  HOOK SAFETY: the repo already learned (DrawESP.h comment block) that Dobby
//  inline trampolines on tiny shared callers corrupt game state.  Every target
//  used here is a large function, and every write is gated on
//  "feature ON + local player + scoping", so with the features off the hooks
//  are pass-through and NOTHING is written (same principle as the
//  edge-triggered NoHitFlinch / NoDrift patches in Main.cpp).
// ============================================================================

//  ---------------------------------------------------------------------------
//  FIRING TIMING (Task 3) - the .57 dump has NO m_HipfireDelay / m_NextFireTime
//  for guns (NextFireTime only exists on WeaponFireComponent_Melee @0x408), so
//  the three delays resolve to these dump-verified symbols:
//
//   3.1 No Sprint Fire Delay
//        Weapon::get_DelaySprintFire()  RVA 0x513CBC4
//        Weapon.m_DelaySprintFire @0x9F8 / WeaponData.DelaySprintFire @0x2C8
//        -> sprint-out timer that blocks firing.
//
//   3.3 No Fire Delay (re-fire cooldown)
//        Weapon::get_FireInterval()     RVA 0x5105C74  (shot-to-shot cooldown)
//        Weapon::get_FireBoltTime()     RVA 0x51237F4  (pump/bolt/chamber time,
//                                       WeaponFireComponent.m_FireBoltTime @0xEC)
//        -> only the interval is shortened; ammo/reload/fire mode are untouched
//           because CanFire()/AmmoCanFire() are NEVER forced here.
//
//   3.2 No Hipfire Delay
//        Weapon::get_UseGetReadyTime()    RVA 0x50E9F60  (m_UseGetReadyTime @0xDF)
//        Weapon::get_SpringFireGetReady() RVA 0x50E9F28  (m_SpringFireGetReady @0xDC)
//        Weapon::get_GetReadyTime()       RVA 0x514D2C8  (m_GetReadyTime @0xBA0)
//        -> this build implements the hipfire "wind-up / readiness" as the
//           GetReady / SpringFireGetReady gate on Weapon.  There is no
//           dedicated hipfire timer in the dump, so these are the gate.
//
//        0x513CBC4 / 0x5105C74 / 0x51237F4 are ALREADY hooked once by
//        System/Hooks/Feats.h (Firerate).  We do not double-hook the same
//        prologue (two trampolines on one address is what crashed the game
//        before) - those three hooks call NoSprintFireDelayActive() /
//        NoFireDelayActive() from this file instead.
//
//        The two hipfire getters are NOT hooked: get_UseGetReadyTime @0x50E9F60
//        and get_SpringFireGetReady @0x50E9F28 are 8-byte leaf getters
//        (ldrb w0,[x0,#..] ; ret) whose *immediate* neighbours are
//        set_UseGetReadyTime @0x50E9F68 and set_SpringFireGetReady @0x50E9F30.
//        A64 inline hooking (hook()/HOOK_LIB) writes a 16-byte LDR/BR stub in
//        place whenever the replacement is more than +/-128MB away, so it
//        overwrote those setters - and the game calls them while it copies
//        weapon data, i.e. exactly when a match is joined.  They are forced to
//        false with 8-byte patches from Main.cpp instead (Patches.
//        NoHipfireUseGetReadyTime / NoHipfireSpringFireGetReady), the same
//        mechanism the repo already uses for the other tiny functions.
//
#include <random>
#include <chrono>
#include <cstdint>
#include <cstring>

#include "../../IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "../../IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "../../IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "../../IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "../../foxcheats/include/ScanEngine.hpp"
#include "../../Includes/Utils.h"
#include "../../Includes/Macros.h"      // hook()/HOOK_LIB = A64HookFunction (safe for tiny getters)
#include "../StructGame/StructGame.h"   // Pawn / GamePlay + Defines.h globals

namespace NewFeatures
{
    // ---------------------------------------------------------------------
    // Field offsets (dump-verified)
    // ---------------------------------------------------------------------
    namespace Offsets
    {
        constexpr uintptr_t WeaponFire_TargetPawn           = 0x668; // m_TargetPawn (0x688 is WRONG, see header)
        constexpr uintptr_t WeaponFire_AimAssistDis         = 0x628; // m_AimAssistDis (float[])
        constexpr uintptr_t WeaponFire_TargetPawnDist       = 0x690; // m_TargetPawnDist
        constexpr uintptr_t WeaponFire_AimmingBreatheCD     = 0x774; // m_AimmingBreatheCountDown
        constexpr uintptr_t WeaponFire_BreatheTremblingPos  = 0x5A0; // m_BreatheTremblingPos (Vector2)
        constexpr uintptr_t WeaponFire_Owner                = 0x28;  // WeaponFireComponent::m_Owner
        constexpr uintptr_t WeaponFire_ClipAmmoCount        = 0xA8;  // WeaponFireComponent::m_ClipAmmoCount

        constexpr uintptr_t Pawn_IsAlive                    = 0xC29C330; // bool() Pawn::get_IsAlive() Slot 49
        constexpr uintptr_t Pawn_FirstPersonCamOffsetX      = 0x6FC; // m_FirstPersonCameraCurrentOffsetX
        constexpr uintptr_t Pawn_FirstPersonCamDestX        = 0x700; // m_FirstPersonCameraDestOffsetX

        constexpr uintptr_t PawnComponent_Owner             = 0x20;  // PawnComponent::m_Pawn
        constexpr uintptr_t WeaponShake_Owner               = 0x78;  // WeaponShakeController::m_Pawn
    }

    // ---------------------------------------------------------------------
    // Function RVAs (relative to libunity.so)
// ---------------------------------------------------------------------
    namespace RVA
    {
        constexpr uintptr_t CheckTarget                     = 0xC9C1B50; // bool(AttackableTarget,Vector3,Vector3,int,float)
        constexpr uintptr_t HasEnemyInPort                  = 0xC9C13F8; // bool(bool)
        constexpr uintptr_t GetBreatheTremblingPos          = 0xC9BD880; // Vector2()
        constexpr uintptr_t GetIsAiming                     = 0xC9AF4E0; // bool()  (fire component ADS flag)
        constexpr uintptr_t AmmoCanFire                     = 0xC160CEC; // bool() Slot 58 - WeaponFireComponent_Instant.AmmoCanFire

        // V3 working triggerbot RVAs (from IL2CPP dump)
        constexpr uintptr_t Weapon_StartFire                = 0x5123C84; // void() Slot 55 - Weapon.StartFire
        constexpr uintptr_t Weapon_StopFire                 = 0x5124EAC; // void(bool) Slot 73 - Weapon.StopFire
        constexpr uintptr_t Weapon_Tick                     = 0x5114C34; // void(float) Slot 33 - Weapon.Tick (game thread heartbeat)
        constexpr uintptr_t Pawn_GetCurrentWeapon           = 0x518BEA8; // Weapon*() - Pawn.get_CurrentWeapon
        constexpr uintptr_t Pawn_IsAiming                   = 0x516699C; // bool() - Pawn.IsAiming (ADS) Slot 427
        constexpr uintptr_t Pawn_get_IsFiring               = 0x51A8A38; // bool() - Pawn.get_IsFiring
        constexpr uintptr_t Gameplay_get_MatchGame          = 0x8569B10; // BaseGame*() - Gameplay.get_MatchGame
        constexpr uintptr_t Gameplay_get_LocalPawn          = 0x856AA08; // Pawn*() - Gameplay.get_LocalPawn
        constexpr uintptr_t BaseGame_GetEnemyPawns          = 0x9CAFE38; // PawnList*() - BaseGame.GetEnemyPawns
        constexpr uintptr_t Pawn_get_HeadPosition           = 0x51B86EC; // ref object() - Pawn.get_HeadPosition Slot 215
        constexpr uintptr_t Pawn_get_LastPawnPos            = 0x51B8D30; // ref object() - Pawn.get_LastPawnPos
        constexpr uintptr_t Camera_get_main                 = 0x5024010; // Camera*() - Camera.get_main
        constexpr uintptr_t Camera_WorldToScreenPoint       = 0x5023B54; // ref object(Vector3) - Camera.WorldToScreenPoint
        constexpr uintptr_t Screen_get_width                = 0x50504BC; // int() - Screen.get_width
        constexpr uintptr_t Screen_get_height               = 0x5050520; // int() - Screen.get_height
        constexpr uintptr_t TickLocalPlayer                 = 0x8F141B8; // void(float) - PlayerSwimmingComponent.TickLocalPlayer (secondary heartbeat)

        constexpr uintptr_t CollectWeaponShakeOffsets       = 0x9B774C0; // WeaponShakeOffsets(float,float)
        constexpr uintptr_t UpdateFirstPersonCameraOffset   = 0x51C489C; // void(float)
        constexpr uintptr_t OnJumpHitGround                 = 0x8599460; // void(float,bool)

        constexpr uintptr_t Pawn_GetTeamId                  = 0x51B89CC; // uint()
        constexpr uintptr_t Pawn_IsSameCamp                 = 0x51EF174; // bool(Pawn)
        constexpr uintptr_t Pawn_IsLocalPlayer              = 0x518872C; // bool()
        constexpr uintptr_t Pawn_IsAlive                    = 0xC29C330; // bool() Pawn::get_IsAlive() Slot 49
    }

    inline std::mt19937 rng(std::random_device{}());

    // =====================================================================
    //  Small helpers
    // =====================================================================
    inline void* UnityRva(uintptr_t rva)
    {
        return (void *)getAbsoluteAddress("libunity.so", (DWORD)rva);
    }

    inline void* LocalPlayer()
    {
        return (void *)GamePlay::get_LocalPawn();
    }

    inline bool IsReadable(void *p)
    {
        return p != nullptr && Tools::IsPtrValid(p);
    }

    // Pawn::IsAiming() @0x519BB10 - "the local player is scoping / ADS"
    inline bool IsLocalPlayerScoping()
    {
        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn)) return false;
        auto isAiming = (bool (*)(void *))UnityRva(RVA::Pawn_IsAiming);
        return isAiming != nullptr && isAiming(localPawn);
    }

    inline bool IsPawnAlive(void *pawn)
    {
        if (!IsReadable(pawn)) return false;
        auto getIsAlive = (bool (*)(void *))UnityRva(RVA::Pawn_IsAlive);
        return getIsAlive ? getIsAlive(pawn) : false;
    }

    // A hook only ever touches the local player's own weapon / components.
    inline bool IsLocalFireComponent(void *fireComponent)
    {
        if (!IsReadable(fireComponent)) return false;
        void *owner = *(void **)((uintptr_t)fireComponent + Offsets::WeaponFire_Owner);
        return owner != nullptr && owner == LocalPlayer();
    }

    inline bool IsLocalWeaponShakeController(void *controller)
    {
        if (!IsReadable(controller)) return false;
        void *owner = *(void **)((uintptr_t)controller + Offsets::WeaponShake_Owner);
        return owner != nullptr && owner == LocalPlayer();
    }

    inline bool IsLocalPawnComponent(void *component)
    {
        if (!IsReadable(component)) return false;
        void *owner = *(void **)((uintptr_t)component + Offsets::PawnComponent_Owner);
        return owner != nullptr && owner == LocalPlayer();
    }

    // Fire-component level "is scoping" (WeaponFireComponent_Instant::get_IsAiming).
    inline bool IsFireComponentScoping(void *fireComponent)
    {
        auto getIsAiming = (bool (*)(void *))UnityRva(RVA::GetIsAiming);
        return getIsAiming != nullptr && getIsAiming(fireComponent);
    }

    // Team check - pawn level (dump-verified getter + IsSameCamp).
    // Returns true when localPawn and targetPawn are on the SAME team.
    inline bool IsSameTeam(void *localPawn, void *targetPawn)
    {
        if (!IsReadable(localPawn) || !IsReadable(targetPawn)) return false;
        if (localPawn == targetPawn) return true;

        auto isSameCamp = (bool (*)(void *, void *))UnityRva(RVA::Pawn_IsSameCamp); // Slot 84
        if (isSameCamp != nullptr && isSameCamp(localPawn, targetPawn)) return true;

        auto getTeamId = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);       // Slot 216
        if (getTeamId == nullptr) return false;
        const uint32_t localTeam  = getTeamId(localPawn);
        const uint32_t targetTeam = getTeamId(targetPawn);
        return localTeam != 0 && localTeam == targetTeam;
    }

// =====================================================================
    //  TRIGGERBOT
    // =====================================================================

    // Re-entrancy guard (StartFire -> ... -> Tick)
    static bool s_triggerFiring = false;

    // =====================================================================
    //  V3-STYLE GAME-THREAD TRIGGERBOT (A-Fire)
    //  Runs on game thread via Weapon::Tick hook, scans EnemyPawns list,
    //  projects head/body to screen, fires if under crosshair.
    //  Throttled to ~10ms. Uses Weapon.StartFire/StopFire on current weapon.
    // =====================================================================
    
    // Forward declarations
    inline void (*orig_Weapon_Tick)(void *, float) = nullptr;
    inline void (*orig_Weapon_StartFire)(void *) = nullptr;
    inline void (*orig_Weapon_StopFire)(void *, bool) = nullptr;
    inline void (*orig_TickLocalPlayer)(void *, float) = nullptr;

    // State (only accessed from game thread)
    inline bool     g_afireHoldingFire   = false;
    inline void    *g_afireFiringOn      = nullptr;
    inline double   g_afireLastPressTime = -1000.0;
    inline bool     g_afireReleaseRequested = false;
    inline double   g_afireLastEvalTime     = -1000.0;

    // Debug state for UI
    inline int s_dbgTriggerState = 0;

    inline double A_FireNow()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Crosshair radius in pixels (matches V3)
    inline float A_FireCrosshairRadiusPx()
    {
        auto getWidth = (int (*)())UnityRva(RVA::Screen_get_width);
        auto getHeight = (int (*)())UnityRva(RVA::Screen_get_height);
        if (!getWidth || !getHeight) return 64.0f;
        
        const float w = (float)getWidth();
        const float h = (float)getHeight();
        float radius = ImMin(w, h) * 0.06f;
        if (Config.Aim.AimAssistSize > 0.0f)
        {
            const float fromSlider = w * (Config.Aim.AimAssistSize / 100.0f) * 0.5f;
            if (fromSlider > radius)
                radius = fromSlider;
        }
        if (radius < 8.0f)
            radius = 8.0f;
        return radius;
    }

    // Check if pawn pointer is valid and aligned (V3 method)
    inline bool A_FireIsReadablePawn(uintptr_t pawnU)
    {
        if (pawnU == 0 || (pawnU & 0x7) != 0)
            return false;
        return Tools::IsPtrValid((void *) pawnU);
    }

    // Find nearest enemy under crosshair (V3 method)
    inline uintptr_t A_FireFindTarget()
    {
        auto getMatchGame = (uintptr_t (*)())UnityRva(RVA::Gameplay_get_MatchGame);
        auto getLocalPawn = (uintptr_t (*)())UnityRva(RVA::Gameplay_get_LocalPawn);
        auto getEnemyPawns = (void* (*)(void*))UnityRva(RVA::BaseGame_GetEnemyPawns);
        auto getMainCamera = (void* (*)())UnityRva(RVA::Camera_get_main);
        auto worldToScreen = (void* (*)(void*, Vector3))UnityRva(RVA::Camera_WorldToScreenPoint);
        auto getHeadPos = (Vector3 (*)(void*))UnityRva(RVA::Pawn_get_HeadPosition);
        auto getLastPos = (Vector3 (*)(void*))UnityRva(RVA::Pawn_get_LastPawnPos);
        auto getWidth = (int (*)())UnityRva(RVA::Screen_get_width);
        auto getHeight = (int (*)())UnityRva(RVA::Screen_get_height);

        if (!getMatchGame || !getLocalPawn || !getEnemyPawns || !getMainCamera || !worldToScreen || !getHeadPos || !getLastPos || !getWidth || !getHeight)
            return 0;

        const uintptr_t matchGame = getMatchGame();
        if (!Tools::IsPtrValid((void *)matchGame))
            return 0;

        const uintptr_t localPawnU = getLocalPawn();
        if (localPawnU == 0)
            return 0;

        void *enemyPawns = getEnemyPawns((void *)matchGame);
        if (!Tools::IsPtrValid(enemyPawns))
            return 0;

        // Get List items (IL2CPP List<T> layout: items array at offset 0x10, size at 0x18)
        uintptr_t *items = *(uintptr_t **)((uintptr_t)enemyPawns + 0x10);
        int count = *(int *)((uintptr_t)enemyPawns + 0x18);
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024)
            return 0;

        void *cam = getMainCamera();
        if (!Tools::IsPtrValid(cam))
            return 0;

        const float cx = (float)getWidth() * 0.5f;
        const float cy = (float)getHeight() * 0.5f;
        const float radius = A_FireCrosshairRadiusPx();
        const float radiusSq = radius * radius;

        uintptr_t best = 0;
        float bestDist = radiusSq + 1.0f;

        for (int i = 0; i < count; i++)
        {
            const uintptr_t pawnU = items[i];
            if (!A_FireIsReadablePawn(pawnU))
                continue;

            // Check IsAlive via getter (field offset corrupted in dump)
            auto getIsAlive = (bool (*)(void*))UnityRva(RVA::Pawn_IsAlive);
            if (getIsAlive && !getIsAlive((void *)pawnU))
                continue;

            Pawn *pawn = (Pawn *)pawnU;
            const Vector3 headPos = getHeadPos(pawn);
            const Vector3 bodyPos = getLastPos(pawn);

            // World to screen for head
            Vector3 headSc = ((Vector3 (*)(void*, Vector3))worldToScreen)(cam, headPos);
            if (headSc.z > 0.0f)
            {
                const float dx = headSc.x - cx;
                const float dy = headSc.y - cy;
                const float d2 = dx * dx + dy * dy;
                if (d2 <= radiusSq && d2 < bestDist)
                {
                    best = pawnU;
                    bestDist = d2;
                    continue; // head covered - don't test body
                }
            }

            // World to screen for body
            Vector3 bodySc = ((Vector3 (*)(void*, Vector3))worldToScreen)(cam, bodyPos);
            if (bodySc.z > 0.0f)
            {
                const float dx = bodySc.x - cx;
                const float dy = bodySc.y - cy;
                const float d2 = dx * dx + dy * dy;
                if (d2 <= radiusSq && d2 < bestDist)
                {
                    best = pawnU;
                    bestDist = d2;
                }
            }
        }
        return best;
    }

    // Release trigger
    inline void A_FireReleaseTrigger(void *instance)
    {
        if (!g_afireHoldingFire)
            return;
        if (instance == nullptr || instance == g_afireFiringOn) {
            if (orig_Weapon_StopFire != nullptr && g_afireFiringOn != nullptr)
                orig_Weapon_StopFire(g_afireFiringOn, false);
            g_afireHoldingFire = false;
            g_afireFiringOn = nullptr;
        }
    }

    // Render-thread-safe release request
    inline void A_FireRequestRelease(void)
    {
        g_afireReleaseRequested = true;
    }

    // Triggerbot driver (called from game thread only)
    inline void A_FireTick(void)
    {
        const double now = A_FireNow();
        if (now - g_afireLastEvalTime < 0.010) // ~10ms throttle
            return;
        g_afireLastEvalTime = now;

        if (!Config.NewFeaturesMenu.Triggerbot || g_afireReleaseRequested) {
            g_afireReleaseRequested = false;
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        auto getLocalPawn = (uintptr_t (*)())UnityRva(RVA::Gameplay_get_LocalPawn);
        auto getCurrentWeapon = (void* (*)(void*))UnityRva(RVA::Pawn_GetCurrentWeapon);
        if (!getLocalPawn || !getCurrentWeapon) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        const uintptr_t localPawnU = getLocalPawn();
        if (localPawnU == 0) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        void *localPawn = (void *)localPawnU;
        if (!Tools::IsPtrValid(localPawn)) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        auto getIsAlive = (bool (*)(void*))UnityRva(RVA::Pawn_IsAlive);
        if (getIsAlive && !getIsAlive(localPawn)) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        void *held = getCurrentWeapon(localPawn);
        if (!Tools::IsPtrValid(held)) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        // Weapon swapped while trigger down - release old weapon first
        if (g_afireHoldingFire && g_afireFiringOn != held)
            A_FireReleaseTrigger(nullptr);

        bool ready = A_FireFindTarget() != 0;

        // Trigger modes: 0=Always, 1=ADS only, 2=Firing only
        if (ready && Config.NewFeaturesMenu.TriggerbotMode == 1) {
            auto isAiming = (bool (*)(void*))UnityRva(RVA::Pawn_IsAiming);
            ready = isAiming && isAiming(localPawn);
        }
        else if (ready && Config.NewFeaturesMenu.TriggerbotMode == 2) {
            auto getIsFiring = (bool (*)(void*))UnityRva(RVA::Pawn_get_IsFiring);
            ready = getIsFiring && getIsFiring(localPawn);
        }

        if (ready && Config.NewFeaturesMenu.TriggerbotDelay > 0) {
            if (now - g_afireLastPressTime < (double)Config.NewFeaturesMenu.TriggerbotDelay / 1000.0)
                ready = false;
        }

        if (ready) {
            if (!g_afireHoldingFire) {
                if (orig_Weapon_StartFire != nullptr)
                    orig_Weapon_StartFire(held);
                g_afireHoldingFire = true;
                g_afireFiringOn = held;
                s_dbgTriggerState = 2; // firing
            }
            g_afireLastPressTime = now;
        }
        else if (g_afireHoldingFire) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = ready ? 1 : 0; // target locked or no target
        }
        else {
            s_dbgTriggerState = ready ? 1 : 0;
        }
    }

    // Weapon::Tick hook (primary game-thread heartbeat)
    inline void Hook_Weapon_Tick(void *thiz, float deltaTime)
    {
        if (orig_Weapon_Tick != nullptr)
            orig_Weapon_Tick(thiz, deltaTime);
        A_FireTick();
    }

    // PlayerSwimmingComponent::TickLocalPlayer hook (secondary heartbeat)
    inline void Hook_TickLocalPlayer(void *thiz, float deltaTime)
    {
        if (orig_TickLocalPlayer != nullptr)
            orig_TickLocalPlayer(thiz, deltaTime);
        A_FireTick();
    }

    // Hook installation
    inline void InstallTriggerbotHooks()
    {
        // Weapon::Tick - primary game-thread heartbeat for triggerbot
        Tools::Hook(UnityRva(RVA::Weapon_Tick),
                    (void *)&Hook_Weapon_Tick,
                    (void **)&orig_Weapon_Tick);

        // Weapon::StartFire - get original for calling when trigger condition met
        auto startFireAddr = UnityRva(RVA::Weapon_StartFire);
        if (Tools::IsPtrValid(startFireAddr)) {
            orig_Weapon_StartFire = (void (*)(void *))startFireAddr;
        }

        // Weapon::StopFire - get original for releasing the trigger
        auto stopFireAddr = UnityRva(RVA::Weapon_StopFire);
        if (Tools::IsPtrValid(stopFireAddr)) {
            orig_Weapon_StopFire = (void (*)(void *, bool))stopFireAddr;
        }

        // TickLocalPlayer (secondary heartbeat) - helps ensure triggerbot runs
        Tools::Hook(UnityRva(RVA::TickLocalPlayer),
                    (void *)&Hook_TickLocalPlayer,
                    (void **)&orig_TickLocalPlayer);
    }

inline int GetTriggerbotDebugState() { return s_dbgTriggerState; }

    // Layout of WeaponShakeController.WeaponShakeOffsets (private nested struct).
    struct WeaponShakeOffsets
    {
        Vector3 rotationOffset;              // 0x00
        Vector3 positionOffset;              // 0x0C
        Vector3 aimRotationOffset;           // 0x18
        Vector3 gunKickAimRotationOffset;    // 0x24
        Vector3 crosshairOffset;             // 0x30  <- reticle / scope offset
        Vector3 ikWeaponRootRotationOffset;  // 0x3C
        Vector3 ikWeaponRootPositionOffset;  // 0x48
        Vector3 pawnRootRotationOffset;      // 0x54
        Vector3 pawnRootPositionOffset;      // 0x60
        bool    isRotationEnd;               // 0x6C
        bool    isPositionEnd;               // 0x6D
    };

    inline void ZeroScopeShakeOffsets(WeaponShakeOffsets &o)
    {
        // Clears exactly the 9 offset Vector3s (0x00..0x6B); the two state
        // bytes at 0x6C/0x6D are left alone so the controller still knows its
        // shake modes finished.
        std::memset(&o, 0, 9 * sizeof(Vector3));
    }

    inline void ZeroIfNonZero(float *p)
    {
        if (p != nullptr && *p != 0.0f) *p = 0.0f;
    }

    // =====================================================================
    //  FIRING TIMING (Task 3)
    // =====================================================================
    namespace FiringTiming
    {
        // Weapon instance fields (dump 819580..826686)
        constexpr uintptr_t Weapon_SpringFireGetReady = 0xDC;   // protected bool
        constexpr uintptr_t Weapon_UseGetReadyTime    = 0xDF;   // protected bool
        constexpr uintptr_t Weapon_DelaySprintFire    = 0x9F8;  // protected float
        constexpr uintptr_t Weapon_GetReadyTime       = 0xBA0;  // protected float

        // Weapon RVAs inside libunity.so
        constexpr uintptr_t get_DelaySprintFire    = 0x513CBC4;
        constexpr uintptr_t get_FireInterval       = 0x5105C74;
        constexpr uintptr_t get_FireBoltTime       = 0x51237F4;
        constexpr uintptr_t get_UseGetReadyTime    = 0x50E9F60;
        constexpr uintptr_t get_SpringFireGetReady = 0x50E9F28;
        constexpr uintptr_t get_GetReadyTime       = 0x514D2C8;
        constexpr uintptr_t LocalPawn_GetCurrentWeapon = 0x518BEA8; // Pawn::get_CurrentWeapon()
    }

    // Is `weaponInstance` the weapon the local player is currently holding?
    // (keeps every timing override local-player-only)
    inline bool IsLocalWeapon(void *weaponInstance)
    {
        if (!IsReadable(weaponInstance)) return false;
        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn)) return false;
        auto getCurrentWeapon = (void *(*)(void *))UnityRva(FiringTiming::LocalPawn_GetCurrentWeapon);
        if (getCurrentWeapon == nullptr) return false;
        return getCurrentWeapon(localPawn) == weaponInstance;
    }

    // Hipfiring == local player alive and NOT scoping / ADS.
    inline bool LocalPlayerIsHipfiring()
    {
        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn) || !IsPawnAlive(localPawn)) return false;
        return !IsLocalPlayerScoping();
    }

    // Consumed by the existing Feats.h hooks (single hook per address).
    inline bool NoSprintFireDelayActive(void *weaponInstance)
    {
        return Config.NewFeaturesMenu.NoSprintToFireDelay && IsLocalWeapon(weaponInstance);
    }

    inline bool NoFireDelayActive(void *weaponInstance)
    {
        return Config.NewFeaturesMenu.NoFireDelay && IsLocalWeapon(weaponInstance);
    }

    // ---- 3.2 No Hipfire Delay -------------------------------------------
    // Implemented as two 8-byte code patches in Main.cpp (hook install time),
    // NOT as hooks - see the note in the file header.  Each getter is exactly 8
    // bytes long, so "mov x0,#0 ; ret" rewrites the whole function and cannot
    // touch the setter that follows it:
    //   Weapon::get_UseGetReadyTime()    @0x50E9F60 -> false
    //   Weapon::get_SpringFireGetReady() @0x50E9F28 -> false
    // A code patch cannot be gated per instance, so (unlike the old hook, which
    // only skipped the delay for the local player's own weapon while hipfiring)
    // the gate reports "no get-ready time" for every weapon instance.  That is
    // exactly what the option asks for and it matches how the repo's other
    // tiny-function/global patches (NoDrift, NoHitFlinch, NoCrouch, ...) work.
    // IsLocalWeapon() is still used by the Feats.h timing hooks above;
    // LocalPlayerIsHipfiring() is now only a helper (kept, no longer on any
    // hot path since the two getters are patched, not hooked).

    // =====================================================================
    //  Original function pointers + hook bodies
    // =====================================================================

    // ---- Stable Scope : breathing ----
    Vector2 (*oWeaponFireComponent_Instant_GetBreatheTremblingPos)(void *) = nullptr;

    inline Vector2 Hook_WeaponFireComponent_Instant_GetBreatheTremblingPos(void *thiz)
    {
        // Only override when the feature is on, this is the local player's weapon,
        // and the local player is actually hardscoping. Outside that state return the
        // game's value unchanged. Do NOT write to m_AimmingBreatheCountDown /
        // m_BreatheTremblingPos here - the existing NoDrift MemoryPatch
        // (UpdateBreatheTrembling @0xC9BE4C0 -> ret) already freezes the breathing
        // tick, so a per-frame zero-write would fight the game and feel like lag.
        if (!Config.NewFeaturesMenu.StableScope) return Vector2::Zero();
        if (!IsLocalFireComponent(thiz)) return Vector2::Zero();
        if (!IsLocalPlayerScoping()) return Vector2::Zero();
        return Vector2::Zero();
    }

    // ---- Stable Scope : movement sway / locomotion + landing shake ----
    WeaponShakeOffsets (*oWeaponShakeController_CollectWeaponShakeOffsets)(void *, float, float) = nullptr;

    inline WeaponShakeOffsets Hook_WeaponShakeController_CollectWeaponShakeOffsets(void *thiz, float deltaTime, float weight)
    {
        WeaponShakeOffsets offsets{};
        if (oWeaponShakeController_CollectWeaponShakeOffsets)
            offsets = oWeaponShakeController_CollectWeaponShakeOffsets(thiz, deltaTime, weight);

        if (!Config.NewFeaturesMenu.StableScope) return offsets;
        if (!IsLocalWeaponShakeController(thiz)) return offsets;   // local player only
        if (!IsLocalPlayerScoping()) return offsets;               // only while hardscoping

        // Zero the visual offsets so strafing / walking / sliding / the land
        // shake mode cannot move the reticle away from screen center.
        ZeroScopeShakeOffsets(offsets);
        return offsets;
    }

    // ---- Stable Scope : lateral 1P camera shift while strafing ----
    void (*oPawn_UpdateFirstPersonCameraOffset)(void *, float) = nullptr;

    inline void Hook_Pawn_UpdateFirstPersonCameraOffset(void *thiz, float deltaTime)
    {
        if (oPawn_UpdateFirstPersonCameraOffset)
            oPawn_UpdateFirstPersonCameraOffset(thiz, deltaTime);

        if (!Config.NewFeaturesMenu.StableScope) return;
        if (thiz == nullptr || thiz != LocalPlayer()) return;
        if (!IsLocalPlayerScoping()) return;

        ZeroIfNonZero((float *)((uintptr_t)thiz + Offsets::Pawn_FirstPersonCamOffsetX));
        ZeroIfNonZero((float *)((uintptr_t)thiz + Offsets::Pawn_FirstPersonCamDestX));
    }

    // ---- Stable Scope : landing impact ----
    void (*oHitGroundComponent_OnJumpHitGround)(void *, float, bool) = nullptr;

    inline void Hook_HitGroundComponent_OnJumpHitGround(void *thiz, float fLandHeight, bool bIsUltLanding)
    {
        // Swallow the landing camera punch while hardscoping so the crosshair
        // does not dip and spring back.  Outside that state the game behaves
        // exactly as before.
        if (Config.NewFeaturesMenu.StableScope &&
            IsLocalPawnComponent(thiz) && IsLocalPlayerScoping())
            return;

        if (oHitGroundComponent_OnJumpHitGround)
            oHitGroundComponent_OnJumpHitGround(thiz, fLandHeight, bIsUltLanding);
    }

    // =====================================================================
    //  Hook installation (called from Main.cpp, Init_Thread)
    // =====================================================================
    inline void InstallHooks()
    {
        // ---- Triggerbot (V3-style game-thread) ---------------------------
        InstallTriggerbotHooks();

        // ---- Stable Scope ----------------------------------------------
        Tools::Hook(UnityRva(RVA::GetBreatheTremblingPos),
                    (void *)&Hook_WeaponFireComponent_Instant_GetBreatheTremblingPos,
                    (void **)&oWeaponFireComponent_Instant_GetBreatheTremblingPos);
        Tools::Hook(UnityRva(RVA::CollectWeaponShakeOffsets),
                    (void *)&Hook_WeaponShakeController_CollectWeaponShakeOffsets,
                    (void **)&oWeaponShakeController_CollectWeaponShakeOffsets);
        Tools::Hook(UnityRva(RVA::UpdateFirstPersonCameraOffset),
                    (void *)&Hook_Pawn_UpdateFirstPersonCameraOffset,
                    (void **)&oPawn_UpdateFirstPersonCameraOffset);
        Tools::Hook(UnityRva(RVA::OnJumpHitGround),
                    (void *)&Hook_HitGroundComponent_OnJumpHitGround,
                    (void **)&oHitGroundComponent_OnJumpHitGround);

        // ---- Firing timing : 3.2 No Hipfire Delay -----------------------
        // NoSprintFireDelay (3.1) and NoFireDelay (3.3) are gated inside the
        // existing Feats.h hooks for 0x513CBC4 / 0x5105C74 / 0x51237F4 - see
        // NoSprintFireDelayActive() / NoFireDelayActive() above.
        // NoHipfireDelay is a pair of 8-byte patches, created and toggled in
        // Main.cpp (Patches.NoHipfireUseGetReadyTime /
        // Patches.NoHipfireSpringFireGetReady) - the two getters are 8-byte leaf
        // functions and must never be inline-hooked (their setters are adjacent).

        LOGI("[NewFeatures] hooks installed: Weapon_Tick=%p Weapon_StartFire=%p Weapon_StopFire=%p TickLocalPlayer=%p "
             "BreatheTremblingPos=%p CollectWeaponShakeOffsets=%p UpdateFirstPersonCameraOffset=%p OnJumpHitGround=%p "
             "(NoHipfireDelay: 2 x 8-byte patch, no hook)",
             (void *)orig_Weapon_Tick,
             (void *)orig_Weapon_StartFire,
             (void *)orig_Weapon_StopFire,
             (void *)orig_TickLocalPlayer,
             (void *)oWeaponFireComponent_Instant_GetBreatheTremblingPos,
             (void *)oWeaponShakeController_CollectWeaponShakeOffsets,
             (void *)oPawn_UpdateFirstPersonCameraOffset,
             (void *)oHitGroundComponent_OnJumpHitGround);
    }
}
