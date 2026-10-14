#pragma once

// ============================================================================
//  NewFeatures.h  -  combat (Triggerbot) features
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
//     Camera::WorldToScreenPoint  RVA 0x5023B54   (the 1-arg overload; see
//                                 the note in A_FireFindTarget)
//     Pawn::IsAiming()            RVA 0x519BB10   ("ADS only" mode; note
//                                 0x516699C is Weapon::get_IsAiming, and
//                                 calling that on a Pawn is what the old
//                                 build did)
//     Pawn::get_IsFiring()        RVA 0x51A8A38   ("Firing only" mode)
//
//   The old CheckTarget @0xC9C1B50 / HasEnemyInPort @0xC9C13F8 /
//   WeaponFireComponent::StartFire @0xC14C29C hooks are gone: they only run on
//   the game's own aim-assist path (too late for an auto-fire).
//
//   Pawn (TypeDefIndex 16146)
//     m_IsAlive            0x548
//
//  ---------------------------------------------------------------------------
//  STABLE SCOPE was removed with its UI; its four hooks (GetBreatheTremblingPos
//  0xC9BD880, CollectWeaponShakeOffsets 0x9B774C0, UpdateFirstPersonCameraOffset
//  0x51C489C, OnJumpHitGround 0x8599460) are gone. NoDrift now = the
//  UpdateBreatheTrembling patch in Main.cpp (Patches.NoDrift).
//  ---------------------------------------------------------------------------
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

        constexpr uintptr_t WeaponShake_Owner               = 0x78;  // WeaponShakeController::m_Pawn
    }

    // ---------------------------------------------------------------------
// Function RVAs (relative to libunity.so)
 // ---------------------------------------------------------------------
    namespace RVA
    {
        constexpr uintptr_t CheckTarget                     = 0xC9C1B50; // bool(AttackableTarget,Vector3,Vector3,int,float)
        constexpr uintptr_t HasEnemyInPort                  = 0xC9C13F8; // bool(bool)
        constexpr uintptr_t GetIsAiming                     = 0xC9AF4E0; // bool()  (fire component ADS flag)
        // WAS 0xC160CEC = CarePackageGrenade's Slot-58 override (dump 766768);
        // Weapon's own AmmoCanFire is 0x50EB73C - every call passes a Weapon*.
        constexpr uintptr_t AmmoCanFire                     = 0x50EB73C; // bool() - Weapon.AmmoCanFire

        // V3 working triggerbot RVAs (from IL2CPP dump)
        constexpr uintptr_t Weapon_StartFire                = 0x5123C84; // void() Slot 55 - Weapon.StartFire
        constexpr uintptr_t Weapon_StopFire                 = 0x5124EAC; // void(bool) Slot 73 - Weapon.StopFire
        // Triggerbot heartbeat. Weapon::Tick (Slot 33, class Weapon) is what the
        // reference implementation hooks and what this build demonstrably calls
        // on the held weapon every frame -> primary heartbeat.
        // Pawn::Tick is the *base* implementation only: dump.cs shows PlayerPawn
        // (0x54816D8), MPPawn, BRPlayerPawn, BRGroundWarPlayerPawn, DMZPlayerPawn,
        // PVEPlayerPawn, ... each override Tick at their own RVA, so hooking
        // Pawn::Tick can never be the only heartbeat. Kept as a second chance.
        constexpr uintptr_t Weapon_Tick                     = 0x5114C34; // void(float) Slot 33 - Weapon.Tick  [HEARTBEAT]
        constexpr uintptr_t Pawn_Tick                       = 0x51C366C; // void(float) Slot 33 - Pawn.Tick (base only)
        constexpr uintptr_t Pawn_GetCurrentWeapon           = 0x518BEA8; // Weapon*() - Pawn.get_CurrentWeapon
        constexpr uintptr_t Pawn_IsAiming                   = 0x519BB10; // bool() - Pawn.IsAiming() (V3 api15; 0x516699C is Weapon::get_IsAiming, NOT callable on a Pawn)
        constexpr uintptr_t Pawn_get_IsFiring               = 0x51A8A38; // bool() - Pawn.get_IsFiring
        constexpr uintptr_t Gameplay_get_MatchGame          = 0x8569B10; // BaseGame*() - Gameplay.get_MatchGame
        constexpr uintptr_t Gameplay_get_LocalPawn          = 0x856AA08; // Pawn*() - Gameplay.get_LocalPawn
        constexpr uintptr_t BaseGame_GetEnemyPawns          = 0x9CAFE38; // PawnList*() - BaseGame.GetEnemyPawns
        constexpr uintptr_t Pawn_get_HeadPosition           = 0x51B86EC; // ref object() - Pawn.get_HeadPosition Slot 215
        constexpr uintptr_t Pawn_get_LastPawnPos            = 0x51B8D30; // ref object() - Pawn.get_LastPawnPos
        constexpr uintptr_t Camera_get_main                 = 0x5024010; // Camera*() - Camera.get_main
        constexpr uintptr_t Camera_WorldToScreenPoint       = 0x5023B54; // Vector3 Camera.WorldToScreenPoint(Vector3)  <- 1-arg overload (0x5023AB8 is the 2-arg one)
        constexpr uintptr_t Physics_RaycastLOS              = 0x506C1E4; // bool(Vector3, Vector3, out RaycastHit) - 2-arg overload (dump.cs 267239)
        // The game's OWN visibility answers - no raycast needed:
        //   RecentlyRendered: was this pawn actually rendered on screen recently
        //     (frustum + occlusion culling + render distance) - Slot 234
        //   LastOccVisible: last frame's occlusion-culling verdict - Slot 235
        constexpr uintptr_t Pawn_get_RecentlyRendered       = 0x51BA7E8; // bool() Slot 234 (dump.cs 796916)
        constexpr uintptr_t Pawn_get_LastOccVisible         = 0x51BA800; // bool() Slot 235 (dump.cs 796918)
        constexpr uintptr_t Screen_get_width                = 0x50504BC; // int() - Screen.get_width
        constexpr uintptr_t Screen_get_height               = 0x5050520; // int() - Screen.get_height

        constexpr uintptr_t Pawn_GetTeamId                  = 0x51B89CC; // uint()
        constexpr uintptr_t Pawn_IsSameCamp                 = 0x51EF174; // bool(Pawn)
        constexpr uintptr_t Pawn_IsLocalPlayer              = 0x518872C; // bool()

        // ===== ENHANCED TRIGGERBOT RVAs =====
        // All addresses verified against eliwoahzja/dump (same build: it gives
        // Weapon.StartFire 0x5123C84 Slot 55 and Weapon.Tick 0x5114C34 Slot 33).
        // NEVER add an unverified RVA here - a bogus address jumps into unrelated
        // code (the first cut of this block carried several that are not in the
        // dump: 0x510C0A0, 0x513F8A0, 0x510E000/100/200, 0x513E000/100).
        constexpr uintptr_t Weapon_get_CurrentFireComponent = 0x50EB1E4; // object() - Weapon.CurrentFireComponent (dump 739078)
        constexpr uintptr_t Weapon_get_IsAutoFire           = 0x513D704; // bool()   - Weapon.get_IsAutoFire (dump 742570)
        constexpr uintptr_t Weapon_get_IsBurstFire          = 0x50EBB28; // bool()   - Weapon.get_IsBurstFire (dump 739129)
        constexpr uintptr_t Weapon_get_ClipAmmoCount        = 0x50EC614; // int()    - Weapon.get_ClipAmmoCount (dump 739204)
        constexpr uintptr_t Weapon_get_ShotSpreadSize       = 0x5109B68; // float()  - Weapon.get_ShotSpreadSize (dump 739732)
        constexpr uintptr_t WeaponFireComponent_get_BulletInitSpeed = 0xC14A158; // float() - base WeaponFireComponent.get_BulletInitSpeed (dump 764888, m_BulletInitSpeed @0x220)
        
        // Pawn velocity for target leading.
        // WAS 0x713743 - that is dump.cs LINE 713743 (where get_Velocity is
        // declared), not an address; the real RVA is 0x51B38CC.
        constexpr uintptr_t Pawn_get_Velocity               = 0x51B38CC; // Vector3() - Pawn.get_Velocity (dump 713743)

        // Camera.get_transform 0x5023A00 / Transform.get_position 0x50206AC are
        // NOT in the dump (0x50206AC is set_position) and were never used -
        // transform access goes through cam->get_transform()->get_position().
        // Spread: the dump has no get_CurrentSpread / get_CurrentRecoil anywhere;
        // the real per-weapon answer is Weapon.get_ShotSpreadSize @0x5109B68.
        
        // Sprint-to-fire and hipfire delay
        constexpr uintptr_t Weapon_get_DelaySprintFire      = 0x513CBC4; // float()
        constexpr uintptr_t Weapon_get_UseGetReadyTime      = 0x50E9F60; // bool()
        constexpr uintptr_t Weapon_get_SpringFireGetReady   = 0x50E9F28; // bool()
        constexpr uintptr_t Weapon_get_GetReadyTime         = 0x514D2C8; // float()
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

    // A hook only ever touches the local player
    inline bool IsLocalFireComponent(void *fireComponent)
    {
        if (!IsReadable(fireComponent)) return false;
        void *owner = *(void **)((uintptr_t)fireComponent + Offsets::WeaponFire_Owner);
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

        // `static`: UnityRva -> getAbsoluteAddress() re-opens and regex-scans
        // /proc/self/maps on EVERY call, and this runs per enemy per scan.
        static auto isSameCamp = (bool (*)(void *, void *))UnityRva(RVA::Pawn_IsSameCamp); // Slot 84
        if (isSameCamp != nullptr && isSameCamp(localPawn, targetPawn)) return true;

        static auto getTeamId = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);       // Slot 216
        if (getTeamId == nullptr) return false;
        const uint32_t localTeam  = getTeamId(localPawn);
        const uint32_t targetTeam = getTeamId(targetPawn);
        return localTeam != 0 && localTeam == targetTeam;
    }

    // =====================================================================
    //  ENHANCED TRIGGERBOT HELPERS
    // =====================================================================

    // Hitbox priority enum
    enum class HitboxPriority : int
    {
        Head = 0,
        Chest = 1,
        Limbs = 2,
        Any = 3
    };

    // Fire mode enum
    enum class FireMode : int
    {
        Single = 0,
        Burst = 1,
        Auto = 2,
        Unknown = 3
    };

    // Target info structure with hitbox and leading data
    struct TriggerTarget
    {
        Pawn *pawn = nullptr;
        Vector3 aimPoint;           // World position to aim at (with leading)
        Vector3 screenPos;          // Screen position
        float dist2 = -1.0f;        // Screen distance squared from crosshair
        float worldDist = 0.0f;     // World distance to target
        HitboxPriority hitbox = HitboxPriority::Any;
        bool hasLOS = false;
        float travelTime = 0.0f;    // Bullet travel time
        Vector3 predictedPos;       // Predicted position with leading
    };

    // Weapon.CurrentFireComponent (dump 739078, RVA 0x50EB1E4). This dump prints
    // every object/struct return as "ref object" (get_MatchGame and
    // WorldToScreenPoint print identically and both are called by value here in
    // game), so it is called by value.
    inline void *GetCurrentFireComponent(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return nullptr;
        static auto fn = (void *(*)(void *))UnityRva(RVA::Weapon_get_CurrentFireComponent);
        if (fn == nullptr) return nullptr;
        void *comp = fn(weapon);
        return Tools::IsPtrValid(comp) ? comp : nullptr;
    }

    // Get bullet speed from weapon (meters/second)
    inline float GetWeaponBulletSpeed(void *weapon)
    {
        void *fireComp = GetCurrentFireComponent(weapon);
        if (Tools::IsPtrValid(fireComp)) {
            // WeaponFireComponent.get_BulletInitSpeed, base class @0xC14A158
            // (reads m_BulletInitSpeed @0x220) - correct for every subclass.
            static auto getSpeed = (float (*)(void *))UnityRva(RVA::WeaponFireComponent_get_BulletInitSpeed);
            if (getSpeed != nullptr) {
                const float speed = getSpeed(fireComp);
                if (speed >= 1.0f && speed <= 5000.0f) return speed;
            }
        }
        // No verified answer: a fixed AR-ish speed, so leading degrades to a
        // constant small offset instead of switching itself off.
        return 800.0f;
    }

    // Get fire mode of current weapon - Weapon.get_IsAutoFire / get_IsBurstFire
    // (dump 742570 / 739129); Weapon has no int get_FireMode in the dump.
    inline FireMode GetWeaponFireMode(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return FireMode::Unknown;

        static auto getIsAuto  = (bool (*)(void *))UnityRva(RVA::Weapon_get_IsAutoFire);
        static auto getIsBurst = (bool (*)(void *))UnityRva(RVA::Weapon_get_IsBurstFire);

        if (getIsAuto == nullptr && getIsBurst == nullptr) return FireMode::Auto;
        if (getIsAuto != nullptr && getIsAuto(weapon))   return FireMode::Auto;
        if (getIsBurst != nullptr && getIsBurst(weapon)) return FireMode::Burst;
        return FireMode::Single;   // neither flag set -> semi-auto
    }

    // Get target velocity for leading (Pawn.get_Velocity @0x51B38CC, dump 713743)
    inline Vector3 GetPawnVelocity(void *pawn)
    {
        if (!Tools::IsPtrValid(pawn)) return Vector3::zero();

        static auto getVelocity = (Vector3 (*)(void *))UnityRva(RVA::Pawn_get_Velocity);
        if (getVelocity) {
            return getVelocity(pawn);
        }
        return Vector3::zero();
    }

    // Calculate bullet travel time to target
    inline float CalculateTravelTime(float distance, float bulletSpeed)
    {
        if (bulletSpeed <= 0.0f) return 0.0f;
        return distance / bulletSpeed;
    }

    // Predict target position based on velocity and bullet travel time
    inline Vector3 PredictTargetPosition(const Vector3 &currentPos, const Vector3 &velocity, float travelTime)
    {
        if (travelTime <= 0.0f) return currentPos;
        return currentPos + velocity * travelTime;
    }

    // Is the weapon able to fire right now (ammo + the game's own gate).
    // Both calls take a Weapon*, so no fire-component pointer is needed. (The
    // old version called AmmoCanFire with a fire component as `this`, but that
    // address is a *subclass* override - CarePackageGrenade.)
    inline bool IsWeaponReadyToFire(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;

        static auto ammoCanFire = (bool (*)(void *))UnityRva(RVA::AmmoCanFire);
        if (ammoCanFire != nullptr && !ammoCanFire(weapon)) return false;

        static auto getClip = (int (*)(void *))UnityRva(RVA::Weapon_get_ClipAmmoCount);
        if (getClip != nullptr && getClip(weapon) <= 0) return false;

        return true;
    }

    // Is the weapon's current spread small enough to fire? Uses
    // Weapon.get_ShotSpreadSize @0x5109B68 (the only spread getter in the dump).
    // FAIL-OPEN on purpose: no getter / NaN / nonsense value all mean "fire".
    inline bool IsShotAccurate(void *weapon, float maxAllowedSpread = 2.0f)
    {
        if (!Tools::IsPtrValid(weapon)) return true;
        static auto getSpread = (float (*)(void *))UnityRva(RVA::Weapon_get_ShotSpreadSize);
        if (getSpread == nullptr) return true;
        const float spread = getSpread(weapon);
        if (!(spread >= 0.0f) || spread > 90.0f) return true;   // NaN / nonsense units
        return spread <= maxAllowedSpread;
    }

    // Get hitbox world position for a pawn
    inline Vector3 GetHitboxPosition(void *pawn, HitboxPriority hitbox)
    {
        if (!Tools::IsPtrValid(pawn)) return Vector3::zero();
        
        switch (hitbox) {
            case HitboxPriority::Head: {
                Transform *headBone = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_HeadBone);
                if (Tools::IsPtrValid((void *)headBone)) {
                    return headBone->get_position();
                }
                break;
            }
            case HitboxPriority::Chest: {
                // Try mesh root (body center)
                Transform *mesh = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_Mesh);
                if (Tools::IsPtrValid((void *)mesh)) {
                    return mesh->get_position();
                }
                break;
            }
            case HitboxPriority::Limbs: {
                // Use LastPawnPos as fallback for limbs
                auto getLastPos = (Vector3 (*)(void *))UnityRva(RVA::Pawn_get_LastPawnPos);
                if (getLastPos) {
                    return getLastPos(pawn);
                }
                break;
            }
            case HitboxPriority::Any:
            default: {
                // Try head first, then body
                Transform *headBone = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_HeadBone);
                if (Tools::IsPtrValid((void *)headBone)) {
                    return headBone->get_position();
                }
                Transform *mesh = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_Mesh);
                if (Tools::IsPtrValid((void *)mesh)) {
                    return mesh->get_position();
                }
                break;
            }
        }
        return Vector3::zero();
    }

    // Check which hitbox is under crosshair
    inline HitboxPriority GetHitboxUnderCrosshair(void *pawn, const Vector3 &camPos, const Vector3 &crosshairDir, float radius)
    {
        if (!Tools::IsPtrValid(pawn)) return HitboxPriority::Any;
        
        // Check head hitbox
        Transform *headBone = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_HeadBone);
        if (Tools::IsPtrValid((void *)headBone)) {
            Vector3 headPos = headBone->get_position();
            Vector3 toHead = headPos - camPos;
            float dist = sqrtf(toHead.x * toHead.x + toHead.y * toHead.y + toHead.z * toHead.z);
            if (dist > 0.0001f) {
                Vector3 dir = toHead / dist;
                float dot = crosshairDir.x * dir.x + crosshairDir.y * dir.y + crosshairDir.z * dir.z;
                // If dot product is close to 1, head is in crosshair
                if (dot > 0.999f) return HitboxPriority::Head; // ~2.5 degrees
            }
        }
        
        // Check body hitbox
        Transform *mesh = *(Transform **)((uintptr_t)pawn + Class_Pawn_m_Mesh);
        if (Tools::IsPtrValid((void *)mesh)) {
            Vector3 bodyPos = mesh->get_position();
            Vector3 toBody = bodyPos - camPos;
            float dist = sqrtf(toBody.x * toBody.x + toBody.y * toBody.y + toBody.z * toBody.z);
            if (dist > 0.0001f) {
                Vector3 dir = toBody / dist;
                float dot = crosshairDir.x * dir.x + crosshairDir.y * dir.y + crosshairDir.z * dir.z;
                if (dot > 0.999f) return HitboxPriority::Chest;
            }
        }
        
        return HitboxPriority::Limbs;
    }

    // =====================================================================
    //  TRIGGERBOT  (A-Fire)  -  game-thread auto-fire
    //
    //  A port of oonalang/V3 @3dd0151 (System/Hooks/Feats.h: hook_Weapon_Tick +
    //  A_FireFindTarget + Weapon.StartFire/StopFire raw pointers). V3's exact
    //  shape is kept: hook Weapon::Tick, and act only while the ticked object is
    //  the local player's *held weapon*:
    //      (void*)GamePlay::get_LocalPawn()->get_CurrentWeapon() == instance
    //
    //  HEARTBEAT - read this before changing it:
    //    The heartbeat has to be a function that the game calls on the local
    //    player's held weapon every frame. Weapon::Tick (0x5114C34) is that
    //    function: V3 hooks it, and the earlier build of this repo that CRASHED
    //    on match join was reaching A_FireFindTarget through this very hook, so
    //    it demonstrably runs in this build.
    //
    //    Pawn::Tick (0x51C366C) is only the BASE implementation. dump.cs shows
    //    every player pawn class overriding it at its own RVA - PlayerPawn
    //    0x54816D8, MPPawn 0xB5D0DC8, BRPlayerPawn 0x78B68FC, DMZPlayerPawn
    //    0x7FF06EC, PVEPlayerPawn 0x78378AC, BRGroundWarPlayerPawn 0x6FB04A8,
    //    OpenWorldPlayerPawn 0x5A0E994, ... - so "Pawn::Tick runs for every pawn
    //    incl. the local player" is FALSE, and a build whose only heartbeat was a
    //    Pawn::Tick hook gated on the local pawn (commit 7c542b5) could never
    //    tick: the triggerbot and the queued Test Fire both went silent.
    //    It is kept here purely as a second chance, never as the only heartbeat.
    //
    //  The menu's Test Fire button only QUEUES the request: the menu runs on the
    //  render thread, and calling Weapon.StartFire from there does not fire (the
    //  old direct-call version did nothing at all). The game-thread heartbeat
    //  performs it and reports the result back.
    //
    //  Every address below is from "dump (GR .57).cs" and was re-checked against
    //  "dump.cs" (jeudhehs/dump) - both give the same function for every RVA:
    //    Weapon::Tick                  0x5114C34  void(float) Slot 33 [HOOKED]
    //    Pawn::Tick                    0x51C366C  void(float) Slot 33 (base only)
    //    Pawn::get_CurrentWeapon       0x518BEA8  Weapon*()
    //    Pawn::IsAiming()              0x519BB10  bool()  (0x516699C is
    //                                             Weapon::get_IsAiming)
    //    Pawn::get_IsFiring()          0x51A8A38  bool()
    //    Weapon::StartFire()           0x5123C84  void()   Slot 55 [raw ptr]
    //    Weapon::StopFire(bool)        0x5124EAC  void(bool) Slot 73 [raw ptr]
    //    GamePlay::get_LocalPawn()     0x856AA08  Pawn*
    //    GamePlay::get_MatchGame()     0x8569B10  MatchGame*
    //    Camera::get_main()            0x5024010  Camera*
    //    Camera::WorldToScreenPoint()  0x5023B54  Vector3(Vector3)  <- the 1-arg
    //                                             overload (0x5023AB8 takes a
    //                                             bool that must not be garbage)
    //    Screen::get_width/get_height  0x50504BC / 0x5050520
    //  Field offsets, all confirmed in the same dump:
    //    Pawn.m_Mesh 0x628   Pawn.m_HeadBone 0x308   Pawn.m_IsAlive 0x548
    //    BaseGame.EnemyPawns 0x178  (PawnList : List<Pawn>)
    //
    //  !! NEVER index the List object itself (*(uintptr_t**)(list + 0x10)):
    //  !! that walks the il2cpp ARRAY HEADER (klass / bounds pointers), which
    //  !! are aligned and mapped, so they pass every guard and then get handed
    //  !! to Pawn virtual getters - that is what crashed the client on match
    //  !! join.  List<Pawn*>::getItems() returns the real element array.
    // =====================================================================

    // Function pointers. Weapon::Tick is the heartbeat (see the header note);
    // StartFire/StopFire are called through raw pointers, never hooked.
    // fn_* are resolved once at install time - getAbsoluteAddress() re-opens
    // and regex-scans /proc/self/maps on EVERY call, so resolving them per scan
    // was the single biggest cost of the triggerbot loop.
    inline void (*orig_Weapon_Tick)(void *, float) = nullptr;
    inline void (*orig_Pawn_Tick)(void *, float) = nullptr;
    inline void (*fn_Weapon_StartFire)(void *) = nullptr;
    inline void (*fn_Weapon_StopFire)(void *, bool) = nullptr;
    inline Vector3 (*fn_WorldToScreen)(void *, Vector3) = nullptr;  // 1-arg overload
    inline bool (*fn_PhysicsRaycastLOS)(Vector3, Vector3, float, void *) = nullptr;
    inline void *(*fn_CameraGetMain)() = nullptr;
    inline bool (*fn_PawnRecentlyRendered)(void *) = nullptr;
    inline bool (*fn_PawnLastOccVisible)(void *) = nullptr;

    // State - game thread only.
    inline bool     g_afireHoldingFire       = false;
    inline void    *g_afireFiringOn          = nullptr;
    inline double   g_afireLastPressTime     = -1000.0;
    inline double   g_afireLastEvalTime      = -1000.0;
    inline bool     g_afireInTick            = false;   // re-entrancy guard
    inline bool     g_afireLosOk             = false;   // last scan: target has LOS
    inline bool     g_testFireRequest        = false;   // set by the menu (render thread)
    inline double   g_testFireRequestTime    = -1000.0;

    // Diagnostics, so the menu can show *why* nothing happens. The game thread
    // writes, the render thread reads - racy but harmless (plain ints/pointers).
    struct TriggerbotDiag
    {
        int    weaponTicks;     // Weapon::Tick calls (any weapon)          <- heartbeat
        int    weaponHeldTicks; // ... of which the local player's held weapon
        int    pawnTicks;       // Pawn::Tick calls (any pawn) - rarely the local player
        int    pawnLocalTicks;
        int    hookCalls;       // evaluations that reached the scan (game thread)
        int    scans;           // game-thread A_FireFindTarget() runs
        int    enemyCount;      // last EnemyPawns size
        int    targets;         // scans that returned a pawn
        int    fires;           // StartFire calls made by the trigger
        int    losBlocked;      // lifetime: candidates rejected by geometry
        int    losBlockedScan;  // THIS scan (reset every scan, menu reads it)
        float  radiusPx;        // crosshair radius used by the last scan
        float  closestPx;       // closest enemy to the screen centre in px (any distance)
        double lastTickTime;    // steady_clock of the last heartbeat (0 = never)
        void  *localPawn;
        void  *heldWeapon;
        void  *matchGame;
        int    testFireStatus;  // 0 idle, 1 fired, 2 no pawn, 3 no weapon,
                                // 4 no StartFire ptr, 5 pointers ok / no heartbeat
    };
    inline TriggerbotDiag g_triggerDiag{};

    // Debug state for the menu: 0 = no target, 1 = target, 2 = firing.
    inline int s_dbgTriggerState = 0;

    inline double A_FireNow()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Crosshair radius in pixels. Default is V3's zone: 2.5 % of the screen
    // width (== 27 px on a 1600-wide render - tight by design, aimbot is the
    // one that snaps FOR you, this only fires when YOU aim). The menu exposes
    // a Triggerbot Radius slider in FULL pixels so a 40-px-hardcoded value can
    // never hide legitimate targets again (device diag showed R=40 / Near=147,
    // i.e. the zone was simply smaller than the crosshair-to-dummy distance).
    inline float A_FireCrosshairRadiusPx()
    {
        const float w = (float)get_width();
        const float fallback = w > 0.0f ? w * 0.025f : 64.0f;
        float radius = (float)Config.NewFeaturesMenu.TriggerbotRadius;
        if (!(radius > 0.0f)) radius = fallback;          // 0/negative -> default
        if (radius < 4.0f)   radius = 4.0f;               // degenerate guard
        if (radius > w * 0.5f) radius = w * 0.5f;         // never the whole screen
        // The Aimbot's Aim-Assist slider may widen the zone, never shrink it.
        if (Config.Aim.AimAssistSize > 0.0f) {
            const float fromSlider = w * (Config.Aim.AimAssistSize / 100.0f) * 0.5f;
            if (fromSlider > radius) radius = fromSlider;
        }
        return radius;
    }

    // True when `localPawn` is the local player's live pawn.
    inline bool A_FireIsLiveLocalPawn(void *localPawn)
    {
        return Tools::IsPtrValid(localPawn) &&
               ((Pawn *)localPawn)->m_IsAlive();
    }

    // ---- Line of sight (the V3 wall check, for this build's exact layout) ---
    // Physics.Raycast(out RaycastHit)  @0x506C1E4  - the plain 2-arg overload,
    // END of the overload list after the 4-arg bool-returning one (dump.cs:
    // 267226..267244). Cast per CANDIDATE inside the scan: its result only
    // decides whether a pawn inside the zone is shootable, it cannot widen the
    // candidate set, so total cast count is unchanged and the cost stays one
    // ray per candidate (usually 0-1, hard cap below).
    //
    // RaycastHit struct layout at dump.cs line 268905 (TypeDefIndex 1838):
    //   m_Point Vector3 @0x00, m_Normal @0x0C, m_FaceID int @0x18,
    //   m_Distance float @0x1C, m_UV @0x20, m_Collider @0x28
    // NOTE: V3 read the distance at +0x20; THIS build has it at 0x1C. Reading
    // a stale-struct offset returns m_UV as "distance" - fail-closed, but wrong.
    constexpr size_t AF_RAYHIT_SIZE    = 0x30;   // up to m_Collider @0x28 + pad
    constexpr size_t AF_RAYHIT_DISTOFF = 0x1C;   // m_Distance for THIS build



    // Returns the first-hit distance for origin->dir, -1 when clear, -2 on
    // failure/malformed (fail-closed: the caller treats that as "blocked").
    inline float A_FireRaycastDistance(const Vector3 &origin, const Vector3 &dir, float maxDist)
    {
        unsigned char hitBuf[AF_RAYHIT_SIZE];
        memset(hitBuf, 0, sizeof hitBuf);
        bool hit;

        if (fn_PhysicsRaycastLOS != nullptr) {
            hit = fn_PhysicsRaycastLOS(origin, dir, maxDist, hitBuf);
        } else {
            return -2.0f;                                   // no raycast -> fail closed
        }

        if (!hit) return -1.0f;                             // clear line
        const float d = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
        if (!(d >= 0.0f) || !(d <= maxDist + 0.5f)) return -2.0f;  // malformed -> blocked
        return d;
    }

    // True when the ray from the camera to the target position is NOT stopped
    // by geometry before reaching it (0.35 m slack for skin/collider width).
    inline bool A_FireHasLineOfSight(const Vector3 &camPos, const Vector3 &targetPos)
    {
        Vector3 to = targetPos - camPos;
        const float dist = sqrtf(to.x * to.x + to.y * to.y + to.z * to.z);
        if (dist < 0.01f) return true;
        const float inv = 1.0f / dist;
        const Vector3 dir(to.x * inv, to.y * inv, to.z * inv);
        const float hitDist = A_FireRaycastDistance(camPos, dir, dist);
        return hitDist < 0.0f || hitDist >= dist - 0.35f;
    }

    // Nearest live enemy whose hitbox projects inside the crosshair zone,
    // is in front of the camera, and has LOS. Returns 0 when there is none.
    // Enhanced with hitbox detection, bullet travel time prediction, and target leading.
    inline TriggerTarget A_FireFindTargetEnhanced()
    {
        TriggerbotDiag &d = g_triggerDiag;
        d.enemyCount = 0;
        d.closestPx  = -1.0f;
        d.losBlockedScan = 0;
        
        TriggerTarget result{};
        
        MatchGame *matchGame = GamePlay::get_MatchGame();
        d.matchGame = (void *)matchGame;
        if (!Tools::IsPtrValid((void *)matchGame)) return result;
        
        Camera *cam = Camera::get_main();
        if (!Tools::IsPtrValid((void *)cam)) return result;
        
        List<Pawn *> *enemyPawns = matchGame->EnemyPawns();
        if (!Tools::IsPtrValid((void *)enemyPawns)) return result;
        
        Pawn **items = (Pawn **)enemyPawns->getItems();
        const int count = enemyPawns->getSize();
        d.enemyCount = count;
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024) return result;
        
        const float cx = (float)get_width()  * 0.5f;
        const float cy = (float)get_height() * 0.5f;
        const float radius = A_FireCrosshairRadiusPx();
        const float radiusSq = radius * radius;
        d.radiusPx = radius;
        
        auto worldToScreen = fn_WorldToScreen;
        auto camPos = cam->get_transform()->get_position();
        
        // Get local pawn and weapon for bullet speed
        Pawn *localPawn = GamePlay::get_LocalPawn();
        Weapon *held = nullptr;
        float bulletSpeed = 800.0f;
        FireMode fireMode = FireMode::Auto;
        
        if (Tools::IsPtrValid(localPawn)) {
            held = localPawn->get_CurrentWeapon();
            if (Tools::IsPtrValid(held)) {
                bulletSpeed = GetWeaponBulletSpeed(held);
                fireMode = GetWeaponFireMode(held);
            }
        }
        
        // Hitbox priority from config
        HitboxPriority hitboxPriority = (HitboxPriority)Config.NewFeaturesMenu.TriggerbotHitboxPriority;
        bool enableLeading = Config.NewFeaturesMenu.TriggerbotLeadTarget;
        bool checkSpread = Config.NewFeaturesMenu.TriggerbotCheckSpread;
        
        // ---- Phase 1: collect all valid enemies with hitbox info ----
        constexpr int MAX_CANDIDATES = 16;
        struct Candidate { 
            Pawn *pawn; 
            float dist2; 
            Vector3 aimPoint;
            Vector3 screenPos;
            float worldDist;
            HitboxPriority hitbox;
            Vector3 velocity;
        };
        Candidate candidates[MAX_CANDIDATES];
        int nCandidates = 0;
        
        for (int i = 0; i < count; i++) {
            Pawn *pawn = items[i];
            if (!Tools::IsPtrValid((void *)pawn)) continue;
            if (!pawn->m_IsAlive()) continue;
            
            // Team check
            if (IsSameTeam(localPawn, pawn)) continue;
            
            // Check each hitbox based on priority
            float bestD2 = -1.0f;
            Vector3 bestAim;
            Vector3 bestScreen;
            float bestWorldDist = 0.0f;
            HitboxPriority bestHitbox = HitboxPriority::Any;
            Vector3 pawnVelocity = GetPawnVelocity(pawn);
            
            // Check hitboxes in priority order
            const HitboxPriority hitboxes[] = {HitboxPriority::Head, HitboxPriority::Chest, HitboxPriority::Limbs};
            for (HitboxPriority hb : hitboxes) {
                // Skip if not matching priority filter
                if (hitboxPriority != HitboxPriority::Any && hb != hitboxPriority) continue;
                
                Vector3 hitboxPos = GetHitboxPosition(pawn, hb);
                if (hitboxPos == Vector3::zero()) continue;
                
                const Vector3 sc = worldToScreen(cam, hitboxPos);
                if (sc.z <= 0.0f) continue; // Behind camera
                
                const float dx = sc.x - cx;
                const float dy = sc.y - cy;
                float d2 = dx * dx + dy * dy;
                
                if (d2 < 0.0f) continue;
                if (d2 > radiusSq) continue; // Outside crosshair zone
                
                float worldDist = Vector3::Distance(camPos, hitboxPos);
                
                if (bestD2 < 0.0f || d2 < bestD2) {
                    bestD2 = d2;
                    bestAim = hitboxPos;
                    bestScreen = sc;
                    bestWorldDist = worldDist;
                    bestHitbox = hb;
                }
            }
            
            if (bestD2 < 0.0f) continue;
            
            if (d.closestPx < 0.0f || bestD2 < d.closestPx) d.closestPx = bestD2;
            
            if (nCandidates < MAX_CANDIDATES) {
                candidates[nCandidates] = {
                    pawn, bestD2, bestAim, bestScreen, bestWorldDist, bestHitbox, pawnVelocity
                };
                nCandidates++;
            }
        }
        
        // ---- Phase 2: LOS check and travel time calculation for candidates ----
        // Spread and readiness do not depend on WHICH candidate - evaluate both
        // once here instead of once per candidate (and both helpers take the
        // Weapon*, not a fire-component pointer).
        const bool shotAccurate = !checkSpread || IsShotAccurate(held);
        const bool weaponReady  = IsWeaponReadyToFire(held);

        TriggerTarget bestTarget{};
        float bestDist2 = radiusSq + 1.0f;
        
        for (int c = 0; c < nCandidates; c++) {
            Pawn *cp = candidates[c].pawn;
            bool visible = false;
            
            // Game's own visibility checks
            if (fn_PawnRecentlyRendered != nullptr) {
                visible = fn_PawnRecentlyRendered(cp);
                if (!visible && fn_PawnLastOccVisible != nullptr)
                    visible = fn_PawnLastOccVisible(cp);
            }
            
            if (!visible) {
                // Fallback: raycast
                Vector3 to = candidates[c].aimPoint - camPos;
                const float len = sqrtf(to.x * to.x + to.y * to.y + to.z * to.z);
                if (len < 0.0001f) continue;
                const Vector3 dir(to.x / len, to.y / len, to.z / len);
                const float hit = A_FireRaycastDistance(camPos, dir, candidates[c].worldDist + 0.35f);
                visible = (hit == -1.0f);
            }
            
            if (!visible) {
                d.losBlocked++;
                d.losBlockedScan++;
                continue;
            }
            
            // Calculate travel time for leading
            float travelTime = 0.0f;
            Vector3 predictedPos = candidates[c].aimPoint;
            
            if (enableLeading && bulletSpeed > 0.0f) {
                travelTime = CalculateTravelTime(candidates[c].worldDist, bulletSpeed);
                predictedPos = PredictTargetPosition(candidates[c].aimPoint, candidates[c].velocity, travelTime);
                
                // Re-check LOS to predicted position
                Vector3 toPred = predictedPos - camPos;
                float predDist = sqrtf(toPred.x * toPred.x + toPred.y * toPred.y + toPred.z * toPred.z);
                if (predDist > 0.0001f) {
                    Vector3 predDir(toPred.x / predDist, toPred.y / predDist, toPred.z / predDist);
                    float hit = A_FireRaycastDistance(camPos, predDir, predDist + 0.35f);
                    if (hit != -1.0f) continue; // Predicted path blocked
                }
            }
            
            // Candidate-independent gates (computed once above): if the weapon
            // cannot fire, no candidate can pass - stop scanning.
            if (!shotAccurate || !weaponReady) break;
            
            // Select best target (nearest to crosshair center)
            if (candidates[c].dist2 < bestDist2) {
                bestDist2 = candidates[c].dist2;
                bestTarget = {
                    cp,
                    candidates[c].aimPoint,
                    candidates[c].screenPos,
                    candidates[c].dist2,
                    candidates[c].worldDist,
                    candidates[c].hitbox,
                    true,
                    travelTime,
                    predictedPos
                };
            }
        }
        
        g_afireLosOk = (bestTarget.pawn != nullptr);
        if (bestTarget.pawn != nullptr) d.targets++;
        return bestTarget;
    }

    // Backward compatibility - returns pawn pointer only
    inline uintptr_t A_FireFindTarget()
    {
        TriggerTarget target = A_FireFindTargetEnhanced();
        return target.pawn ? (uintptr_t)target.pawn : 0;
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

    // The menu's "Test Fire" button only queues the request: the menu runs on
    // the render thread and the game must be told to fire from the game thread.
    inline void A_FireRunTestFire()
    {
        TriggerbotDiag &d = g_triggerDiag;
        g_testFireRequest = false;

        Pawn *local = GamePlay::get_LocalPawn();
        d.localPawn = (void *)local;
        if (!A_FireIsLiveLocalPawn(local)) { d.testFireStatus = 2; return; }

        Weapon *held = local->get_CurrentWeapon();
        d.heldWeapon = (void *)held;
        if (!Tools::IsPtrValid((void *)held)) { d.testFireStatus = 3; return; }

        if (fn_Weapon_StartFire == nullptr) { d.testFireStatus = 4; return; }

        fn_Weapon_StartFire(held);
        d.fires++;
        d.testFireStatus = 1;
    }

    // Trigger evaluation. Runs on the game thread from the tick hooks.
    // `tickInstance` is the object the hook was called on: the fired path is
    // taken only while the local player's *held weapon* ticks, exactly like the
    // reference implementation. Pass nullptr when the caller is already sure the
    // local player ticked.
    inline void A_FireTick(void *tickInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        d.lastTickTime = A_FireNow();

        const double now = d.lastTickTime;
        if (now - g_afireLastEvalTime < 0.010) return;   // ~10 ms, like V3
        g_afireLastEvalTime = now;

        if (g_afireInTick) return;                       // re-entrancy guard

        // Queued Test Fire is honoured even while the toggle is off.
        if (g_testFireRequest) A_FireRunTestFire();

        Pawn *localPawn = GamePlay::get_LocalPawn();
        d.localPawn = (void *)localPawn;

        if (!Config.NewFeaturesMenu.Triggerbot) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        if (!A_FireIsLiveLocalPawn(localPawn)) {
            A_FireReleaseTrigger(nullptr);
            d.heldWeapon = nullptr;
            s_dbgTriggerState = 0;
            return;
        }

        Weapon *held = localPawn->get_CurrentWeapon();
        d.heldWeapon = (void *)held;
        if (!Tools::IsPtrValid((void *)held)) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        // V3 semantics: only act on the local player's held weapon tick.
        if (tickInstance != nullptr && tickInstance != (void *)held) return;
        if (tickInstance != nullptr) d.weaponHeldTicks++;

        d.hookCalls++;
        d.scans++;

        // Weapon swapped while the trigger was down - release the old one.
        if (g_afireHoldingFire && g_afireFiringOn != (void *)held)
            A_FireReleaseTrigger(nullptr);

        // Use enhanced target finding
        TriggerTarget target = A_FireFindTargetEnhanced();
        bool ready = target.pawn != nullptr && g_afireLosOk;

        // Modes: 0 = Always, 1 = ADS only, 2 = Firing only.
        if (ready && Config.NewFeaturesMenu.TriggerbotMode == 1) {
            auto isAiming = (bool (*)(void *))UnityRva(RVA::Pawn_IsAiming);
            ready = isAiming != nullptr && isAiming(localPawn);
        } else if (ready && Config.NewFeaturesMenu.TriggerbotMode == 2) {
            auto getIsFiring = (bool (*)(void *))UnityRva(RVA::Pawn_get_IsFiring);
            ready = getIsFiring != nullptr && getIsFiring(localPawn);
        }

        // Fire mode specific handling
        if (ready && Tools::IsPtrValid(held)) {
            FireMode mode = GetWeaponFireMode(held);
            
            // For single-shot weapons, only fire once per trigger press
            if (mode == FireMode::Single && g_afireHoldingFire) {
                ready = false; // Don't hold fire on single-shot
            }
            
            // Burst needs no special handling: StartFire/StopFire repeat per burst.
        }

        if (ready && Config.NewFeaturesMenu.TriggerbotDelay > 0) {
            if (now - g_afireLastPressTime <
                (double)Config.NewFeaturesMenu.TriggerbotDelay / 1000.0)
                ready = false;
        }

        if (!ready) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            return;
        }

        s_dbgTriggerState = 1;                           // target under the crosshair

        // For single-shot, only fire once
        FireMode mode = FireMode::Auto;
        if (Tools::IsPtrValid(held)) {
            mode = GetWeaponFireMode(held);
        }
        
        if (g_afireHoldingFire && g_afireFiringOn == (void *)held) {
            // For single-shot, release after first shot
            if (mode == FireMode::Single) {
                A_FireReleaseTrigger(nullptr);
                return;
            }
            g_afireLastPressTime = now;
            s_dbgTriggerState = 2;
            return;
        }

        g_afireInTick      = true;                       // guard raised first
        g_afireHoldingFire = true;
        g_afireFiringOn    = (void *)held;

        if (fn_Weapon_StartFire != nullptr) {
            fn_Weapon_StartFire(held);
            d.fires++;
        }

        g_afireLastPressTime = now;
        g_afireInTick        = false;
        s_dbgTriggerState    = 2;                        // firing
        LOGI("[NewFeatures] triggerbot: StartFire(held=%p) local=%p hitbox=%d travel=%.3f", 
             (void *)held, (void *)localPawn, (int)target.hitbox, target.travelTime);
    }

    // ---- Heartbeats -------------------------------------------------------
    // Weapon::Tick IS called on the local player's held weapon every frame in
    // this build (V3 hooks the same address and its triggerbot fires), and it is
    // the address the earlier crashing build of this repo was demonstrably
    // reaching. Pawn::Tick is kept as a second chance, but note that PlayerPawn /
    // MPPawn / BRPlayerPawn / ... each override Tick at their own RVA, so
    // Pawn::Tick alone is NOT enough and must never be the only heartbeat.
    inline void Hook_Weapon_Tick(void *instance, float deltaTime)
    {
        if (orig_Weapon_Tick != nullptr)
            orig_Weapon_Tick(instance, deltaTime);

        if (instance == nullptr) return;

        g_triggerDiag.weaponTicks++;
        A_FireTick(instance);
    }

    inline void Hook_Pawn_Tick(void *instance, float deltaTime)
    {
        if (orig_Pawn_Tick != nullptr)
            orig_Pawn_Tick(instance, deltaTime);

        if (instance == nullptr) return;

        TriggerbotDiag &d = g_triggerDiag;
        d.pawnTicks++;
        if ((void *)GamePlay::get_LocalPawn() != instance) return;

        d.pawnLocalTicks++;
        A_FireTick(nullptr);        // the local player's own pawn ticked
    }

    // Installation: Weapon::Tick + Pawn::Tick hooks and the two raw
    // StartFire/StopFire pointers. All three targets are large functions, so
    // Dobby's 16-byte trampoline never eats a neighbouring function.
    inline void InstallTriggerbotHooks()
    {
        // Resolve every per-scan fn pointer ONCE. getAbsoluteAddress() re-opens
        // and regex-scans the whole /proc/self/maps text on every call; it was
        // being called ~100x/s during scans and in every scan loop iteration.
        auto startFireAddr = UnityRva(RVA::Weapon_StartFire);
        if (Tools::IsPtrValid(startFireAddr))
            fn_Weapon_StartFire = (void (*)(void *))startFireAddr;

        auto stopFireAddr = UnityRva(RVA::Weapon_StopFire);
        if (Tools::IsPtrValid(stopFireAddr))
            fn_Weapon_StopFire = (void (*)(void *, bool))stopFireAddr;

        auto w2sAddr = UnityRva(RVA::Camera_WorldToScreenPoint);
        if (Tools::IsPtrValid(w2sAddr))
            fn_WorldToScreen = (Vector3 (*)(void *, Vector3))w2sAddr;

        // 0x506C1E4 = Physics.Raycast(Vector3, Vector3, out RaycastHit) - the
        // plain 2-arg overload (dump.cs 267239-267240). RaycastHit layout for
        // THIS build: m_Distance @0x1C (V3 read 0x20 - wrong here).
        static_assert(RVA::Physics_RaycastLOS == 0x506C1E4, "Physics_RaycastLOS must stay 0x506C1E4 - the 2-arg out-struct overload");
        auto rayAddr = UnityRva(RVA::Physics_RaycastLOS);
        if (Tools::IsPtrValid(rayAddr))
            fn_PhysicsRaycastLOS =
                (bool (*)(Vector3, Vector3, float, void *))rayAddr;

        // Visibility cache getters (the game's own occlusion/frustum verdicts).
        auto rrAddr = UnityRva(RVA::Pawn_get_RecentlyRendered);
        if (Tools::IsPtrValid(rrAddr))
            fn_PawnRecentlyRendered = (bool (*)(void *))rrAddr;

        auto lvAddr = UnityRva(RVA::Pawn_get_LastOccVisible);
        if (Tools::IsPtrValid(lvAddr))
            fn_PawnLastOccVisible = (bool (*)(void *))lvAddr;

        auto camMainAddr = UnityRva(RVA::Camera_get_main);
        if (Tools::IsPtrValid(camMainAddr))
            fn_CameraGetMain = (void *(*)())camMainAddr;

        void *weaponTick = UnityRva(RVA::Weapon_Tick);
        if (Tools::IsPtrValid(weaponTick)) {
            Tools::Hook(weaponTick, (void *)&Hook_Weapon_Tick, (void **)&orig_Weapon_Tick);
            if (orig_Weapon_Tick == nullptr)
                LOGE("[NewFeatures] triggerbot: Weapon::Tick hook FAILED at %p", weaponTick);
            else
                LOGI("[NewFeatures] triggerbot: hooked Weapon::Tick @%p -> orig %p",
                     weaponTick, (void *)orig_Weapon_Tick);
        } else {
            LOGE("[NewFeatures] triggerbot: Weapon::Tick RVA 0x%llX did not resolve -"
                 " triggerbot DISABLED", (unsigned long long)RVA::Weapon_Tick);
        }

        void *pawnTick = UnityRva(RVA::Pawn_Tick);
        if (Tools::IsPtrValid(pawnTick)) {
            Tools::Hook(pawnTick, (void *)&Hook_Pawn_Tick, (void **)&orig_Pawn_Tick);
            LOGI("[NewFeatures] triggerbot: hooked Pawn::Tick @%p -> orig %p",
                 pawnTick, (void *)orig_Pawn_Tick);
        } else {
            LOGE("[NewFeatures] triggerbot: Pawn::Tick RVA 0x%llX did not resolve",
                 (unsigned long long)RVA::Pawn_Tick);
        }

        LOGI("[NewFeatures] triggerbot: StartFire=%p StopFire=%p W2S=%p Raycast=%p RecentlyRendered=%p OccVisible=%p",
             (void *)fn_Weapon_StartFire, (void *)fn_Weapon_StopFire,
             (void *)fn_WorldToScreen, (void *)fn_PhysicsRaycastLOS,
             (void *)fn_PawnRecentlyRendered, (void *)fn_PawnLastOccVisible);
    }

    // True while the game-thread heartbeat has run recently.
    inline bool A_FireHeartbeatAlive()
    {
        const double t = g_triggerDiag.lastTickTime;
        return t > 0.0 && (A_FireNow() - t) < 0.5;
    }

    // ---- Render-thread diagnostics ---------------------------------------
    // Called from the menu (render thread) every frame. Deliberately RAY-FREE
    // and SCAN-FREE: it must NOT call A_FireFindTarget(). That scan casts
    // Physics.Raycast (A_FireHasLineOfSight / the candidate visibility
    // fallback), and doing it here ran a second scan of the same world
    // concurrently with the game-thread scan inside A_FireTick - two physics
    // scene queries at once = the crash that appeared the moment the
    // crosshair-ray gate was added to the render path. The physics world now
    // has exactly ONE caller: A_FireTick, on the game thread.
    //
    // All this keeps is the part that was genuinely useful and is safe on any
    // thread: a Test Fire the game thread never picked up means the heartbeat
    // is dead, so report the pointer chain instead of silently doing nothing.
    // It only reads pointers the menu already reads every frame (local pawn,
    // held weapon, the StartFire ptr resolved once at install time).
    inline void A_FireRenderTick()
    {
        TriggerbotDiag &d = g_triggerDiag;

        if (g_testFireRequest && A_FireNow() - g_testFireRequestTime > 0.35) {
            g_testFireRequest = false;
            Pawn *local = GamePlay::get_LocalPawn();
            d.localPawn = (void *)local;
            if (!A_FireIsLiveLocalPawn(local)) { d.testFireStatus = 2; return; }
            Weapon *held = local->get_CurrentWeapon();
            d.heldWeapon = (void *)held;
            if (!Tools::IsPtrValid((void *)held)) { d.testFireStatus = 3; return; }
            d.testFireStatus = (fn_Weapon_StartFire == nullptr) ? 4 : 5;
        }
    }

    inline int GetTriggerbotDebugState() { return s_dbgTriggerState; }
    inline TriggerbotDiag &GetTriggerbotDiag() { return g_triggerDiag; }
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
    // Stable Scope was removed (UI feature deleted). Its four hooks
    // (GetBreatheTremblingPos / CollectWeaponShakeOffsets /
    // UpdateFirstPersonCameraOffset / OnJumpHitGround) are gone. Note: the old
    // breathing hook returned Vector2::Zero() on EVERY path, which kept
    // zeroing scope sway even with every toggle off - that was the real cause
    // of "No Drift can't be turned off". With the hooks gone, the NoDrift
    // MemoryPatch restore now actually restores the game behaviour.

    // ---- No Slide Fatigue ----------------------------------------------
    // NOT inline-hooked any more. SlideTackleComponent::get_UseSlideTackleLimit()
    // @0x8BF722C (Slot 39) and its PVEArcadeSlideTackleComponent override
    // @0x651274C are 8-byte leaf getters (ldrb w0,[x0,#off] ; ret), and their
    // very next function in .text starts 8 bytes later. That is the one shape an
    // A64 inline hook must never touch: the 16-byte "ldr x17,#8 ; br x17" stub
    // overwrites the neighbour, and for the main getter the neighbour is
    // LimitSlideTackle() @0x8BF7234 - the function that applies the slide
    // limit/fatigue in the first place. Hooking the getter therefore corrupted
    // the exact code the feature was supposed to neutralise.
    //
    // Both getters are 8-byte byte patches now, created and toggled from
    // Main.cpp exactly like Patches.NoDrift:
    //     Patches.NoSlideFatigue     "mov w0,#0 ; ret" on 0x8BF722C
    //     Patches.NoSlideFatiguePve  "mov w0,#0 ; ret" on 0x651274C
    // Semantics are unchanged - false = "this component does not use the slide
    // tackle limit", which switches the whole fatigue system off
    // (m_SlideTackleLimitDecreaseRate, m_SlideTackleIdleLimitDecreaseRate,
    // m_SlideTackleLimitMinIdleTime and the LimitSlideTackle path), so repeated
    // slides keep their full distance/speed instead of decaying.

    // =====================================================================
    inline void InstallHooks()
    {
        // ---- Triggerbot (V3-style game-thread) ---------------------------
        InstallTriggerbotHooks();

        // ---- No Slide Fatigue -------------------------------------------
        // Two 8-byte byte patches owned by Main.cpp (see the note above) - there
        // is nothing to inline-hook here.

        // ---- Firing timing : 3.2 No Hipfire Delay -----------------------
        // NoSprintFireDelay (3.1) and NoFireDelay (3.3) are gated inside the
        // existing Feats.h hooks for 0x513CBC4 / 0x5105C74 / 0x51237F4 - see
        // NoSprintFireDelayActive() / NoFireDelayActive() above.
        // NoHipfireDelay is a pair of 8-byte patches, created and toggled in
        // Main.cpp (Patches.NoHipfireUseGetReadyTime /
        // Patches.NoHipfireSpringFireGetReady) - the two getters are 8-byte leaf
        // functions and must never be inline-hooked (their setters are adjacent).

        LOGI("[NewFeatures] hooks installed: Pawn_Tick=%p Weapon_StartFire=%p Weapon_StopFire=%p "
             "(NoHipfireDelay + NoSlideFatigue are 8-byte patches owned by Main.cpp)",
             (void *)orig_Pawn_Tick,
             (void *)fn_Weapon_StartFire,
             (void *)fn_Weapon_StopFire);
    }
}
