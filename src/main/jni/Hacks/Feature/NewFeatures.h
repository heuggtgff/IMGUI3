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
//  TRIGGERBOT  (A-Fire) - game-thread auto-fire.  Ported from the known-working
//               reference oonalang/V3 @3dd0151 (System/Hooks/Feats.h:
//               hook_Weapon_Tick + A_FireFindTarget), with this repo's menu.
//  ---------------------------------------------------------------------------
//   Driver:  Weapon::Tick     RVA 0x5114C34  (Slot 33)  <- game-thread tick
//            Weapon::StartFire   RVA 0x5123C84  (Slot 55)  called via raw ptr
//            Weapon::StopFire    RVA 0x5124EAC  (Slot 73)  called via raw ptr
//            (StartFire/StopFire are NEVER inline-hooked - exactly like V3.)
//
//   Enemy scan - only accessors this repo already proved in-game (DrawESP.h):
//     Gameplay::get_MatchGame()   RVA 0x8569B10   -> BaseGame / MatchGame*
//     Gameplay::get_LocalPawn()   RVA 0x856AA08   -> Pawn*
//     BaseGame::EnemyPawns        offset 0x178    -> List<Pawn*>*
//       -> List<Pawn*>::getItems() / getSize()   (the ELEMENT array)
//     Pawn::m_IsAlive             offset 0x548    (field read, as in DrawESP)
//     Pawn::m_HeadBone            offset 0x308    Transform* -> get_position()
//     Pawn::m_Mesh                offset 0x628    Transform* -> get_position()
//     Camera::get_main()          RVA 0x5024010
//     Camera::WorldToScreenPoint  RVA 0x5023AB8   (StructSDK, == api8)
//     Pawn::IsAiming()            RVA 0x519BB10   ("ADS only" mode; note
//                                 0x516699C is Weapon::get_IsAiming, and
//                                 calling that on a Pawn is what the old
//                                 build did)
//     Pawn::get_IsFiring()        RVA 0x51A8A38   ("Firing only" mode)
//
//     !!  NEVER index the List object itself (*(uintptr_t**)(list + 0x10)).
//     !!  That yields the il2cpp ARRAY HEADER, whose first slots are the
//     !!  klass / bounds pointers - 8-byte aligned and mapped, so they pass an
//     !!  alignment + IsPtrValid() check - and handing one of those to a Pawn
//     !!  virtual getter is what crashed the client as soon as a game (Training
//     !!  Ground) was joined with the triggerbot enabled.
//
//   The old CheckTarget @0xC9C1B50 / HasEnemyInPort @0xC9C13F8 /
//   WeaponFireComponent::StartFire @0xC14C29C hooks are gone: they only run on
//   the game's own aim-assist path (too late for an auto-fire).
//
//   Pawn (TypeDefIndex 16146)
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
        constexpr uintptr_t Pawn_IsAiming                   = 0x519BB10; // bool() - Pawn.IsAiming() (V3 api15; 0x516699C is Weapon::get_IsAiming, NOT callable on a Pawn)
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

        constexpr uintptr_t CollectWeaponShakeOffsets       = 0x9B774C0; // WeaponShakeOffsets(float,float)
        constexpr uintptr_t UpdateFirstPersonCameraOffset   = 0x51C489C; // void(float)
        constexpr uintptr_t OnJumpHitGround                 = 0x8599460; // void(float,bool)

        constexpr uintptr_t Pawn_GetTeamId                  = 0x51B89CC; // uint()
        constexpr uintptr_t Pawn_IsSameCamp                 = 0x51EF174; // bool(Pawn)
        constexpr uintptr_t Pawn_IsLocalPlayer              = 0x518872C; // bool()
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
        // Field read (Pawn::m_IsAlive @0x548) - the same accessor DrawESP uses.
        // This used to call RVA 0xC29C330, which the dump resolves to
        // BRTankDepot::get_IsAlive() - a *virtual* getter invoked with a Pawn
        // `this`. The offset read below cannot be wrong for a Pawn.
        if (!IsReadable(pawn)) return false;
        return *(bool *)((uintptr_t)pawn + Class_Pawn_m_IsAlive);
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
    //  TRIGGERBOT  (A-Fire)  -  V3-compatible game-thread implementation
    //
    //  Ported from the known-working reference oonalang/V3 @3dd0151
    //  (System/Hooks/Feats.h: hook_Weapon_Tick + A_FireFindTarget), keeping
    //  this repo's menu/config names.
    //
    //  Driver: Weapon::Tick (RVA 0x5114C34). Weapon.StartFire / StopFire are
    //  called through *raw* pointers - they are never inline-hooked, exactly
    //  like V3.
    //
    //  Enemy scanning uses ONLY the accessors this repo already proved to work
    //  (same ones DrawESP.h uses):
    //      GamePlay::get_MatchGame() -> MatchGame::EnemyPawns()
    //        -> List<Pawn*>::getItems() / getSize()
    //      Pawn::m_IsAlive()  field @0x548
    //      Pawn::m_HeadBone @0x308 / m_Mesh @0x628 transforms + get_position()
    //      Camera::get_main()->WorldToScreenPoint()
    //
    //  !! CRASH FIX: the previous version indexed the List object itself
    //  !! (*(uintptr_t**)(list + 0x10)) and then treated element 0/2 of the
    //  !! il2cpp ARRAY HEADER (the klass / bounds pointers) as Pawns. Those
    //  !! values are 8-byte aligned and mapped, so they pass an alignment +
    //  !! IsPtrValid() check, and the first enemy list of a match then sent
    //  !! Pawn virtual getters a non-Pawn `this` - the client died as soon as
    //  !! a game/Training Ground was joined with the triggerbot enabled.
    //  !! getItems() returns the real element array (&array->m_Items[0]).
    // =====================================================================

    // Raw function pointers (StartFire/StopFire are NOT hooked).
    inline void (*orig_Weapon_Tick)(void *, float) = nullptr;
    inline void (*fn_Weapon_StartFire)(void *) = nullptr;
    inline void (*fn_Weapon_StopFire)(void *, bool) = nullptr;

    // State - only ever touched on the game thread.
    inline bool     g_afireHoldingFire   = false;
    inline void    *g_afireFiringOn      = nullptr;
    inline double   g_afireLastPressTime = -1000.0;
    inline double   g_afireLastEvalTime  = -1000.0;
    // Re-entrancy guard. It is raised BEFORE StartFire is called so a Tick
    // raised from inside StartFire can never re-enter the fire path.
    inline bool     g_afireInTick        = false;

    // Debug state for the menu: 0 = no target, 1 = target under crosshair,
    // 2 = firing (tab 3 shows it).
    inline int s_dbgTriggerState = 0;

    inline double A_FireNow()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Crosshair radius in pixels: 2.5% of the smaller screen dimension (V3's
    // tight crosshair zone), optionally widened by the aim-assist size slider.
    inline float A_FireCrosshairRadiusPx()
    {
        const float w = (float)get_width();
        const float h = (float)get_height();
        if (w <= 0.0f || h <= 0.0f) return 64.0f;

        float radius = ImMin(w, h) * 0.025f;
        if (Config.Aim.AimAssistSize > 0.0f) {
            const float fromSlider = w * (Config.Aim.AimAssistSize / 100.0f) * 0.5f;
            if (fromSlider > radius) radius = fromSlider;
        }
        if (radius < 8.0f) radius = 8.0f;
        return radius;
    }

    // Nearest live enemy whose head bone or mesh root projects onto the
    // crosshair. Returns 0 when there is no match / no valid target.
    inline uintptr_t A_FireFindTarget()
    {
        MatchGame *matchGame = GamePlay::get_MatchGame();
        if (!Tools::IsPtrValid((void *)matchGame)) return 0;

        if (!Tools::IsPtrValid((void *)GamePlay::get_LocalPawn())) return 0;

        Camera *cam = Camera::get_main();
        if (!Tools::IsPtrValid((void *)cam)) return 0;

        List<Pawn *> *enemyPawns = matchGame->EnemyPawns();
        if (!Tools::IsPtrValid((void *)enemyPawns)) return 0;

        Pawn **items = (Pawn **)enemyPawns->getItems();
        const int count = enemyPawns->getSize();
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024) return 0;

        const float cx = (float)get_width()  * 0.5f;
        const float cy = (float)get_height() * 0.5f;
        const float radius = A_FireCrosshairRadiusPx();
        const float radiusSq = radius * radius;

        uintptr_t best = 0;
        float bestDist = radiusSq + 1.0f;   // strictly-inside test

        for (int i = 0; i < count; i++) {
            Pawn *pawn = items[i];
            if (!Tools::IsPtrValid((void *)pawn)) continue;
            if (!pawn->m_IsAlive()) continue;

            // Head first ...
            Transform *headBone = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_HeadBone);
            if (Tools::IsPtrValid((void *)headBone)) {
                const Vector3 headSc = cam->WorldToScreenPoint(headBone->get_position());
                if (headSc.z > 0.0f) {  // in front of the camera
                    const float dx = headSc.x - cx;
                    const float dy = headSc.y - cy;
                    const float d2 = dx * dx + dy * dy;
                    if (d2 <= radiusSq && d2 < bestDist) {
                        best = (uintptr_t)pawn;
                        bestDist = d2;
                        continue;       // head covered - body cannot be closer
                    }
                }
            }

            // ... then the mesh root, in case the crosshair is on the body.
            Transform *mesh = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_Mesh);
            if (Tools::IsPtrValid((void *)mesh)) {
                const Vector3 bodySc = cam->WorldToScreenPoint(mesh->get_position());
                if (bodySc.z > 0.0f) {
                    const float dx = bodySc.x - cx;
                    const float dy = bodySc.y - cy;
                    const float d2 = dx * dx + dy * dy;
                    if (d2 <= radiusSq && d2 < bestDist) {
                        best = (uintptr_t)pawn;
                        bestDist = d2;
                    }
                }
            }
        }
        return best;
    }

    // `instance` must be the local player's own, currently held weapon.
    inline bool A_FireIsLocalHeldWeapon(void *instance)
    {
        if (instance == nullptr) return false;
        Pawn *local = GamePlay::get_LocalPawn();
        if (!Tools::IsPtrValid((void *)local) || !local->m_IsAlive()) return false;
        Weapon *held = local->get_CurrentWeapon();
        if (!Tools::IsPtrValid((void *)held)) return false;
        return (void *)held == instance;
    }

    // Let go of the trigger we pressed.
    inline void A_FireReleaseTrigger(void *instance)
    {
        if (!g_afireHoldingFire) return;
        if (instance == nullptr || instance == g_afireFiringOn) {
            if (fn_Weapon_StopFire != nullptr && g_afireFiringOn != nullptr)
                fn_Weapon_StopFire(g_afireFiringOn, false);
            g_afireHoldingFire = false;
            g_afireFiringOn = nullptr;
        }
    }

    // Pawn::IsAiming() @0x519BB10 - the local player is scoping / ADS.
    inline bool A_FireLocalPlayerIsAiming(Pawn *local)
    {
        auto isAiming = (bool (*)(void *))UnityRva(RVA::Pawn_IsAiming);
        return isAiming != nullptr && isAiming(local);
    }

    // Pawn::get_IsFiring() - the local player is holding the fire button.
    inline bool A_FireLocalPlayerIsFiring(Pawn *local)
    {
        auto getIsFiring = (bool (*)(void *))UnityRva(RVA::Pawn_get_IsFiring);
        return getIsFiring != nullptr && getIsFiring(local);
    }

    // Weapon::Tick hook - the game-thread heartbeat (same driver as V3).
    inline void Hook_Weapon_Tick(void *instance, float deltaTime)
    {
        if (orig_Weapon_Tick != nullptr)
            orig_Weapon_Tick(instance, deltaTime);

        if (instance == nullptr) return;

        const double now = A_FireNow();
        if (now - g_afireLastEvalTime < 0.010) return;   // ~10 ms, like V3
        g_afireLastEvalTime = now;

        if (g_afireInTick) return;                       // re-entrancy guard

        if (!Config.NewFeaturesMenu.Triggerbot) {
            A_FireReleaseTrigger(instance);
            s_dbgTriggerState = 0;
            return;
        }

        // Only ever act for the local player's own, currently held weapon.
        if (!A_FireIsLocalHeldWeapon(instance)) {
            A_FireReleaseTrigger(instance);
            s_dbgTriggerState = 0;
            return;
        }

        Pawn *local = GamePlay::get_LocalPawn();

        bool ready = A_FireFindTarget() != 0;

        // Trigger modes: 0 = Always, 1 = ADS only, 2 = Firing only.
        if (ready && Config.NewFeaturesMenu.TriggerbotMode == 1) {
            ready = A_FireLocalPlayerIsAiming(local);
        } else if (ready && Config.NewFeaturesMenu.TriggerbotMode == 2) {
            ready = A_FireLocalPlayerIsFiring(local);
        }

        if (ready && Config.NewFeaturesMenu.TriggerbotDelay > 0) {
            if (now - g_afireLastPressTime <
                (double)Config.NewFeaturesMenu.TriggerbotDelay / 1000.0)
                ready = false;
        }

        if (!ready) {
            A_FireReleaseTrigger(instance);
            s_dbgTriggerState = 0;
            return;
        }

        s_dbgTriggerState = 1;                           // target under crosshair

        if (g_afireHoldingFire && g_afireFiringOn == instance) {
            g_afireLastPressTime = now;                  // already firing on it
            s_dbgTriggerState = 2;
            return;
        }

        g_afireInTick        = true;                     // guard raised first
        g_afireHoldingFire   = true;
        g_afireFiringOn      = instance;

        if (fn_Weapon_StartFire != nullptr)
            fn_Weapon_StartFire(instance);

        g_afireLastPressTime = now;
        g_afireInTick        = false;
        s_dbgTriggerState    = 2;                        // firing
    }

    // Installation: exactly ONE inline hook (Weapon::Tick) plus the two raw
    // StartFire/StopFire pointers. No second heartbeat hook: the reference
    // build does not hook TickLocalPlayer, and an extra trampoline on a
    // function that used to be unused there is one more way to break the
    // game while it builds pawn/weapon state on match join.
    inline void InstallTriggerbotHooks()
    {
        auto startFireAddr = UnityRva(RVA::Weapon_StartFire);
        if (Tools::IsPtrValid(startFireAddr))
            fn_Weapon_StartFire = (void (*)(void *))startFireAddr;

        auto stopFireAddr = UnityRva(RVA::Weapon_StopFire);
        if (Tools::IsPtrValid(stopFireAddr))
            fn_Weapon_StopFire = (void (*)(void *, bool))stopFireAddr;

        Tools::Hook(UnityRva(RVA::Weapon_Tick),
                    (void *)&Hook_Weapon_Tick,
                    (void **)&orig_Weapon_Tick);

        LOGI("[NewFeatures] triggerbot: Weapon_Tick=%p StartFire=%p StopFire=%p",
             (void *)orig_Weapon_Tick, (void *)fn_Weapon_StartFire, (void *)fn_Weapon_StopFire);
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

        LOGI("[NewFeatures] hooks installed: Weapon_Tick=%p Weapon_StartFire=%p Weapon_StopFire=%p "
             "BreatheTremblingPos=%p CollectWeaponShakeOffsets=%p UpdateFirstPersonCameraOffset=%p OnJumpHitGround=%p "
             "(NoHipfireDelay: 2 x 8-byte patch, no hook)",
             (void *)orig_Weapon_Tick,
             (void *)fn_Weapon_StartFire,
             (void *)fn_Weapon_StopFire,
             (void *)oWeaponFireComponent_Instant_GetBreatheTremblingPos,
             (void *)oWeaponShakeController_CollectWeaponShakeOffsets,
             (void *)oPawn_UpdateFirstPersonCameraOffset,
             (void *)oHitGroundComponent_OnJumpHitGround);
    }
}
