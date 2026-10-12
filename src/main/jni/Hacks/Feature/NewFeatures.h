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
        constexpr uintptr_t StartFire                       = 0xC9C9364; // void() Slot 39 - WeaponFireComponent_Instant.StartFire

        // Weapon::Tick(float) - runs every frame, used for tick-based triggerbot
        constexpr uintptr_t Weapon_Tick                     = 0xC9C0B64; // void(float) Slot 51 - WeaponFireComponent_Instant.Tick

        constexpr uintptr_t CollectWeaponShakeOffsets       = 0x9B774C0; // WeaponShakeOffsets(float,float)
        constexpr uintptr_t UpdateFirstPersonCameraOffset   = 0x51C489C; // void(float)
        constexpr uintptr_t OnJumpHitGround                 = 0x8599460; // void(float,bool)

        constexpr uintptr_t Pawn_GetTeamId                  = 0x51B89CC; // uint()
        constexpr uintptr_t Pawn_IsSameCamp                 = 0x51EF174; // bool(Pawn)
        constexpr uintptr_t Pawn_IsAiming                   = 0x519BB10; // bool()
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

    // Humanized delay.  It is armed only AFTER the game's own target check has
    // confirmed an enemy (the old code called TriggerbotDelay() on every
    // CreateBulletProjectile/CreateBulletLine event, i.e. even with no target).
    // Implemented as a millisecond timer instead of this_thread::sleep_for
    // because we are executing inside the game's aim-assist / fire path -
    // sleeping there would visibly stall the render thread.
    static void *s_delayTarget = nullptr;
    static std::chrono::steady_clock::time_point s_delayStart;
    static int s_delayMs = 0;
    static bool s_triggerFiring = false; // re-entrancy guard (StartFire -> ... -> CheckTarget)

    inline int TriggerbotDelay()
    {
        const int lo = Config.NewFeaturesMenu.TriggerbotDelayMin;
        const int hi = Config.NewFeaturesMenu.TriggerbotDelayMax;
        if (hi <= lo) return lo > 0 ? lo : 0;
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    }

    inline bool TriggerbotDelayElapsed(void *targetPawn)
    {
        const auto now = std::chrono::steady_clock::now();
        if (targetPawn != s_delayTarget) {
            s_delayTarget = targetPawn;
            s_delayStart = now;
            s_delayMs = TriggerbotDelay();
            LOGI("[Triggerbot] DelayElapsed: new target, delay=%dms", s_delayMs);
            return s_delayMs <= 0;
        }
        if (s_delayMs <= 0) return true;
        const auto held = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_delayStart).count();
        bool elapsed = held >= (long long)s_delayMs;
        if (elapsed) {
            LOGI("[Triggerbot] DelayElapsed: delay complete, held=%lldms", held);
        }
        return elapsed;
    }

    inline void TriggerbotClearTarget() { s_delayTarget = nullptr; }

    // "Weapon ready" - the game's own check plus an ammo sanity guard.
    inline bool TriggerbotWeaponReady(void *fireComponent)
    {
        if (!IsReadable(fireComponent)) return false;
        auto ammoCanFire = (bool (*)(void *))UnityRva(RVA::AmmoCanFire);
        if (ammoCanFire != nullptr && !ammoCanFire(fireComponent)) return false;
        int clipAmmo = *(int *)((uintptr_t)fireComponent + Offsets::WeaponFire_ClipAmmoCount);
        if (clipAmmo <= 0) return false;
        return true;
    }

    // Loose backstop built from the dump's aim-assist table.  m_AimAssistDis
    // (0x628) is the weapon's aim-assist radius table and m_TargetPawnDist
    // (0x690) the distance the game measured to the locked pawn.  We only use
    // it to reject absurd ranges; the real validation is done by CheckTarget.
    inline bool TriggerbotInAssistRange(void *fireComponent)
    {
        return true; // unlimited range
    }

    inline void TriggerbotFire(void *fireComponent)
    {
        if (!TriggerbotWeaponReady(fireComponent)) return;
        auto startFire = (void (*)(void *))UnityRva(RVA::StartFire);
        if (startFire == nullptr) {
            LOGI("[Triggerbot] Fire: StartFire is null!");
            return;
        }
        s_triggerFiring = true;
        startFire(fireComponent);
        s_triggerFiring = false;
    }
// Shared trigger pipeline. `targetPawn` must be a real Pawn (we only ever
    // pass either m_TargetPawn or the already-confirmed AttackableTarget that
    // the game itself equated with m_TargetPawn).

    // Debug state for UI
    inline int s_dbgTriggerState = 0;

    inline void TriggerbotTryFire(void *fireComponent, void *targetPawn)
    {
        if (!Config.NewFeaturesMenu.Triggerbot) return;
        if (s_triggerFiring) return;
        if (!IsLocalFireComponent(fireComponent)) return;

        void *localPawn = LocalPlayer();

        if (!IsReadable(targetPawn) || targetPawn == localPawn) { TriggerbotClearTarget(); return; }
        if (IsSameTeam(localPawn, targetPawn)) { TriggerbotClearTarget(); return; }
        if (!IsPawnAlive(targetPawn)) { TriggerbotClearTarget(); return; }
        if (!TriggerbotInAssistRange(fireComponent)) { TriggerbotClearTarget(); return; }

        if (!TriggerbotDelayElapsed(targetPawn)) {
            s_dbgTriggerState = 1; // target locked, waiting for delay
            return;
        }

        s_dbgTriggerState = 2; // firing
        LOGI("[Triggerbot] TryFire: FIRING! targetPawn=%p", targetPawn);
        TriggerbotFire(fireComponent);
    }

    inline int GetTriggerbotDebugState() { return s_dbgTriggerState; }

    // =====================================================================
    //  TICK-BASED TRIGGERBOT (alternative, more reliable)
    //  Hooks Weapon::Tick(float) which runs every frame. Checks crosshair
    //  for enemy and calls StartFire directly. Does NOT depend on
    //  CheckTarget/HasEnemyInPort virtual methods being invoked by game.
    // =====================================================================
    void (*oWeapon_Tick)(void *, float) = nullptr;

    inline void Hook_Weapon_Tick(void *thiz, float deltaTime)
    {
        // Call original first
        if (oWeapon_Tick) oWeapon_Tick(thiz, deltaTime);

        // Only run tick-based triggerbot when mode = 1
        if (Config.NewFeaturesMenu.TriggerbotMode != 1) return;
        if (!Config.NewFeaturesMenu.Triggerbot) return;
        if (s_triggerFiring) return;
        if (!IsLocalFireComponent(thiz)) return;

        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn)) return;
        if (!IsLocalPlayerScoping()) return; // only while ADS

        // Get current target from weapon's m_TargetPawn (set by game's aim-assist)
        void *targetPawn = *(void **)((uintptr_t)thiz + Offsets::WeaponFire_TargetPawn);
        if (!IsReadable(targetPawn) || targetPawn == localPawn) return;
        if (IsSameTeam(localPawn, targetPawn)) return;
        if (!IsPawnAlive(targetPawn)) return;

        // Delay
        if (!TriggerbotDelayElapsed(targetPawn)) return;

        s_dbgTriggerState = 2;
        TriggerbotFire(thiz);
    }

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
        constexpr uintptr_t LocalPawn_get_CurrentWeapon = 0x518BEA8; // Pawn::get_CurrentWeapon()
    }

    // Is `weaponInstance` the weapon the local player is currently holding?
    // (keeps every timing override local-player-only)
    inline bool IsLocalWeapon(void *weaponInstance)
    {
        if (!IsReadable(weaponInstance)) return false;
        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn)) return false;
        auto getCurrentWeapon = (void *(*)(void *))UnityRva(FiringTiming::LocalPawn_get_CurrentWeapon);
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

    // ---- Triggerbot ----
    bool (*oWeaponFireComponent_Instant_CheckTarget)(void *, void *, Vector3, Vector3, int, float) = nullptr;
    bool (*oWeaponFireComponent_Instant_HasEnemyInPort)(void *, bool) = nullptr;

    inline bool Hook_WeaponFireComponent_Instant_CheckTarget(void *thiz, void *target,
                                                             Vector3 startPos, Vector3 rayDir,
                                                             int flag, float range)
    {
        // Only run CheckTarget-based triggerbot when mode = 0
        if (Config.NewFeaturesMenu.TriggerbotMode != 0) return oWeaponFireComponent_Instant_CheckTarget ? oWeaponFireComponent_Instant_CheckTarget(thiz, target, startPos, rayDir, flag, range) : false;

        // Override range to unlimited for triggerbot
        const float unlimitedRange = 999999.0f;
        const bool result = oWeaponFireComponent_Instant_CheckTarget
                          ? oWeaponFireComponent_Instant_CheckTarget(thiz, target, startPos, rayDir, flag, unlimitedRange)
                          : false;

        if (result) {
            s_dbgTriggerState = 1; // target detected
        } else if (s_dbgTriggerState != 0) {
            s_dbgTriggerState = 0; // lost target
        }

        if (result && Config.NewFeaturesMenu.Triggerbot && !s_triggerFiring) {
            void *targetPawn = *(void **)((uintptr_t)thiz + Offsets::WeaponFire_TargetPawn);
            void *fireTarget = (targetPawn != nullptr && targetPawn == target) ? targetPawn : target;
            if (fireTarget != nullptr) {
                TriggerbotTryFire(thiz, fireTarget);
            }
        }
        return result;
    }

    inline bool Hook_WeaponFireComponent_Instant_HasEnemyInPort(void *thiz, bool traceAssist)
    {
        // Only run CheckTarget-based triggerbot when mode = 0
        if (Config.NewFeaturesMenu.TriggerbotMode != 0) return oWeaponFireComponent_Instant_HasEnemyInPort ? oWeaponFireComponent_Instant_HasEnemyInPort(thiz, traceAssist) : false;

        const bool result = oWeaponFireComponent_Instant_HasEnemyInPort
                          ? oWeaponFireComponent_Instant_HasEnemyInPort(thiz, traceAssist)
                          : false;

        if (result) {
            s_dbgTriggerState = 1;
        } else if (s_dbgTriggerState == 1) {
            s_dbgTriggerState = 0;
        }

        if (result && Config.NewFeaturesMenu.Triggerbot && !s_triggerFiring) {
            void *targetPawn = *(void **)((uintptr_t)thiz + Offsets::WeaponFire_TargetPawn);
            if (targetPawn != nullptr) {
                TriggerbotTryFire(thiz, targetPawn);
            }
        }
        return result;
    }

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
        // ---- Triggerbot -------------------------------------------------
        Tools::Hook(UnityRva(RVA::CheckTarget),
                    (void *)&Hook_WeaponFireComponent_Instant_CheckTarget,
                    (void **)&oWeaponFireComponent_Instant_CheckTarget);
        Tools::Hook(UnityRva(RVA::HasEnemyInPort),
                    (void *)&Hook_WeaponFireComponent_Instant_HasEnemyInPort,
                    (void **)&oWeaponFireComponent_Instant_HasEnemyInPort);

        // Tick-based triggerbot (alternative, more reliable)
        if (RVA::Weapon_Tick != 0) {
            Tools::Hook(UnityRva(RVA::Weapon_Tick),
                        (void *)&Hook_Weapon_Tick,
                        (void **)&oWeapon_Tick);
        }

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

        LOGI("[NewFeatures] hooks installed: CheckTarget=%p HasEnemyInPort=%p BreatheTremblingPos=%p "
             "CollectWeaponShakeOffsets=%p UpdateFirstPersonCameraOffset=%p OnJumpHitGround=%p "
             "(NoHipfireDelay: 2 x 8-byte patch, no hook)",
             (void *)oWeaponFireComponent_Instant_CheckTarget,
             (void *)oWeaponFireComponent_Instant_HasEnemyInPort,
             (void *)oWeaponFireComponent_Instant_GetBreatheTremblingPos,
             (void *)oWeaponShakeController_CollectWeaponShakeOffsets,
             (void *)oPawn_UpdateFirstPersonCameraOffset,
             (void *)oHitGroundComponent_OnJumpHitGround);
    }
}
