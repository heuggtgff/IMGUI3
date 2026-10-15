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
//  TRIGGERBOT  (A-Fire) - game-thread auto-fire.  Heartbeat and StartFire /
//               StopFire come from the known-working reference oonalang/V3
//               @3dd0151 (System/Hooks/Feats.h: hook_Weapon_Tick); the fire
//               DECISION is this repo's own ShouldFire tree (below).
//  ---------------------------------------------------------------------------
//   THE DECISION (A_FireShouldFire -> A_FireScan): one named reason per scan,
//   in this order, and NOT ONE of them fires on a guess.
//
//   HARD GATES (only these can refuse a shot):
//     1  alive ............ Pawn::m_IsAlive @0x548, second chance
//                           Pawn.get_IsAlive() 0x51B7C40 (Slot 49)
//     2  in a match ....... GamePlay::get_MatchGame() 0x8569B10 +
//                           BaseGame.get_IsPaused() 0x9CAFDC4
//     3  weapon present ... Pawn.get_CurrentWeapon() 0x518BEA8
//     4  trigger mode ..... Pawn.IsAiming 0x519BB10 (ADS only),
//                           Pawn.get_IsFiring 0x51A8A38 (Firing only)
//     5  target visible ... the two paths below
//     6  friendlies ....... BaseGame.GetTeamPawns() 0x9CAFEB4 - a living
//                           teammate within 0.35 m of the bullet line and closer
//                           than the target blocks the shot
//
//   PATH A - "enemy exactly in the centre of the screen":
//     ONE ray from the camera along the camera's own forward
//     (Transform.get_forward 0x509C650), bounded by Physics.Raycast
//     (origin, direction, out RaycastHit, maxDistance) 0x506C188. Whichever pawn
//     the first hit belongs to (Transform.get_parent 0x507BDEC walks the hit's
//     parent chain against every candidate's transform) is the enemy under the
//     crosshair - centred AND unobstructed by construction, because a wall in
//     front of him would have been the first hit. This path needs no bones and
//     no projection, so it keeps working even if the Pawn bone offsets drift
//     between game builds.
//
//   PATH B - the crosshair zone, for a crosshair that is near but not exactly on
//     the enemy: BaseGame::EnemyPawns @0x178 -> team id (Pawn.get_TeamId
//     0x51B89CC) -> alive -> NOT self -> the hitbox walk (head bone 0x308, chest
//     at 0.72 of the feet->head axis, waist at 0.45, mesh root 0x628) must
//     project inside the px zone (Camera.WorldToScreenPoint 0x5023B54), then the
//     visibility ray below.
//
//   VISIBILITY (both paths): the nearest MEANINGFUL thing on the shot line
//     must BE the pawn. Physics.RaycastAll(origin, dir, maxDistance) 0x506CBB0
//     (dump.cs 174011) returns EVERY hit in the segment, so the line is walked
//     nearest-first and two classes of hit are SKIPPED before the verdict:
//       hit in the LOCAL player's hierarchy -> skipped. The camera sits inside
//                                   the local pawn's own collider stack (head /
//                                   upper-body hitbox, aim-assist volume,
//                                   weapon), and PhysX reports an initial-
//                                   overlap hit at distance ~0 for the
//                                   collider containing the ray origin. Device
//                                   proof from the broken build:
//                                   "LOShit=0.00m Rays=0/1 Fire=0" - the ray's
//                                   first "hit" was OUR OWN BODY a few cm from
//                                   the camera, so every shot was vetoed as
//                                   "wall in the way" while the enemy stood in
//                                   the open 3.3 m away.
//       non-target TRIGGER collider -> skipped. Trigger volumes (zone / streak
//                                   / pickup volumes - the dump has Pawn
//                                   m_IsInStreakTriggerVolume etc.) are
//                                   invisible; they cannot be what the player
//                                   sees in the way.
//       first remaining hit IS the target -> clear (his body, his gear, even an
//                                   arm reaching toward the muzzle - the bullet
//                                   reaches HIM either way)
//       first remaining hit is anything else -> blocked. A wall blocks however
//                                   THIN it is, because the test never guesses
//                                   from a distance. (The old kRaySlack = 0.40
//                                   rule accepted any hit within 40 cm of the
//                                   aim point, which is exactly how a thin
//                                   panel in front of the target got shot
//                                   through.)
//       no meaningful hit        -> clear
//       hit cannot be named      -> conservative stem test: a bounded ray that
//                                   STOPS SHORT of the target's own collider
//                                   surface (0.80 of the hitbox radius), so any
//                                   hit at all is geometry in front of him
//       no RaycastAll symbol     -> single nearest-hit Raycast 0x506C188 (same
//                                   naming rules; a self/trigger hit cannot be
//                                   skipped there, so it falls through to the
//                                   occlusion escape below)
//       no ray symbol at all     -> blocked (fail closed, never shoots walls)
//     NEAR-ORIGIN ESCAPE: when the first meaningful blocker sits within
//     kSelfExclusion (15 cm) of the camera it cannot honestly answer "is there
//     a wall between me and a target 3 m away" - it is geometry the camera is
//     clipping. In that ONE case the game's own occlusion verdict decides:
//     Pawn.get_LastOccVisible 0x51BA800 (Slot 235, last frame's occlusion-
//     culling result; RecentlyRendered 0x51BA7E8 second chance) true -> clear
//     (the game rendered him, the player can see him), false -> blocked. It is
//     NEVER consulted for a blocker at normal range, so a real wall keeps
//     vetoing the shot exactly as before.
//
//   ADVISORY ONLY - measured every scan, shown in the menu, never refuses:
//     Weapon.InSwitchWeaponState 0x512724C / HasSwitchingState 0x5124C70,
//     Weapon.AmmoCanFire 0x50EB73C, Weapon.get_ClipAmmoCount 0x50EC614,
//     Weapon.CanFire() 0x5126CC4 (Slot 269 - only a gate while "Require Ready"
//       is on, which is OFF by default),
//     Weapon.RealFireInterval 0x50EC250 (the weapon's own rate, NOT enforced:
//       there is no artificial re-press delay any more),
//     lead (Weapon.get_CurrentFireComponent 0x50EB1E4 ->
//       WeaponFireComponent.get_BulletInitSpeed 0xC14A158, Pawn.get_Velocity
//       0x51B38CC): the aim point moves ahead of a moving target and a covered
//       lead point falls back to the aim point instead of refusing the shot,
//     spread (Weapon.get_ShotSpreadSize 0x5109B68 with the camera focal length
//       from Camera.get_fieldOfView 0x5021F7C) and bullet travel time.
//   Diagnostics: Weapon.get_AimingTarget 0x510EAAC /
//                Weapon.get_CurrentAimingTargetDis 0x510EABC (the game's own
//                aim answer, shown in the menu and used as a tie-break)
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
//     Camera::WorldToScreenPoint  RVA 0x5023B54   (the 1-arg overload - see the
//                                 note above A_FireScan's candidate loop)
//     Pawn::IsAiming()            RVA 0x519BB10   ("ADS only" mode; note
//                                 0x516699C is Weapon::get_IsAiming, and
//                                 calling that on a Pawn is what the old
//                                 build did)
//     Pawn::get_IsFiring()        RVA 0x51A8A38   ("Firing only" mode)
//
//   The LOS rays used by the tree (dump.cs line -> RVA):
//     Physics.Raycast(origin, dir, out RaycastHit hit, float maxDistance)
//       0x506C188 (dump.cs 173930)  <- nearest hit + an explicit limit
//     Physics.Raycast(origin, dir, float maxDistance)
//       0x506BF60 (dump.cs 173912)  <- the same limit, no out struct
//     0x506C1E4 (dump.cs 173933) is the 3-parameter overload with NO maxDistance
//     and is no longer used for the verdict.
//   RaycastHit (dump.cs 175618): m_FaceID @0x18, m_Distance @0x1C, m_UV @0x20,
//     m_Collider @0x28 (the field right after m_UV).
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
#include <atomic>

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
        // REAL heartbeats for the *held weapon's* eval path. Weapon::Tick and
        // Pawn::Tick above are BASE implementations; il2cpp virtual dispatch
        // calls each subclass override instead, so neither ever runs for the
        // local player's gun - which is exactly why wTick=0 pawn=0 on device.
        // Slots all 33, dump-verified per class:
        constexpr uintptr_t PlayerPawn_Tick                 = 0x54816D8; // dump 724744
        constexpr uintptr_t MPPawn_Tick                     = 0xB5D0DC8; // dump 778862
        constexpr uintptr_t BRPlayerPawn_Tick               = 0x78B68FC; // dump 506871
        constexpr uintptr_t DMZPlayerPawn_Tick              = 0x7FF06EC; // dump 786307
        constexpr uintptr_t PVEPlayerPawn_Tick              = 0x78378AC; // dump 755326
        constexpr uintptr_t BRGroundWarPlayerPawn_Tick      = 0x6FB04A8; // dump 489851
        constexpr uintptr_t OpenWorldPlayerPawn_Tick        = 0x5A0E994; // dump 788122
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
        constexpr uintptr_t Physics_RaycastLOS              = 0x506C1E4; // bool(Vector3, Vector3, out RaycastHit) - 3-param overload (dump.cs 173933), NO maxDistance
        // ---- Visibility rays that can NAME what they hit (dump.cs 173927-173945)
        //   0x506C188  bool Raycast(Vector3 origin, Vector3 direction, out RaycastHit hit, float maxDistance)   <- nearest hit + limit
        //   0x506BF60  bool Raycast(Vector3 origin, Vector3 direction, float maxDistance)                      <- limit only, no out struct
        // Both take an explicit maxDistance, so the first hit is the nearest thing
        // on the segment instead of "whatever is behind the enemy".
        constexpr uintptr_t Physics_RaycastHitOut            = 0x506C188; // dump.cs 173930
        constexpr uintptr_t Physics_RaycastSegment           = 0x506BF60; // dump.cs 173912
        // RaycastAll(origin, direction, maxDistance) - dump.cs 174011, returns
        // RaycastHit[] (an il2cpp array). The visibility walk needs EVERY hit in
        // the segment so it can SKIP one: a single nearest-hit Raycast cannot
        // step over the local player's own collider that contains the camera.
        constexpr uintptr_t Physics_RaycastAll               = 0x506CBB0; // dump.cs 174011
        // Collider.get_isTrigger() - dump.cs 175345 (Collider, TypeDefIndex 1833).
        // An invisible trigger volume must never be why a shot is refused.
        constexpr uintptr_t Collider_IsTrigger               = 0x50279D4; // dump.cs 175345
        // Unity scene-graph accessors, used to turn "what the ray hit" into "whose
        // collider is this". Both sit in the same 0x50xxxxx block as the already
        // proven Transform.get_position (0x507C19C == api10 in Defines.h):
        //   Transform.get_forward 0x509C650 (dump.cs 172057)
        //   Transform.get_parent  0x507BDEC (dump.cs 172105)
        constexpr uintptr_t Transform_get_forward            = 0x509C650;
        constexpr uintptr_t Transform_get_parent             = 0x507BDEC;
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
        
        // ---- ShouldFire tree RVAs, every one checked against eliwoahzja/dump
        // (owner class -> dump line -> RVA). None of them is virtual except
        // Weapon.CanFire, whose only overrides are the turrets / boxing /
        // dual wield / sentry gun / helicopter.
        constexpr uintptr_t Weapon_get_IsSingleShot          = 0x50EBA5C; // bool()  Weapon (dump 739174)
        constexpr uintptr_t Weapon_CanFire                   = 0x5126CC4; // bool()  Weapon Slot 269 (dump 740581)
        constexpr uintptr_t Weapon_InSwitchWeaponState       = 0x512724C; // bool()  Weapon (dump 740662)
        constexpr uintptr_t Weapon_HasSwitchingState         = 0x5124C70; // bool()  Weapon (dump 740707)
        constexpr uintptr_t Weapon_RealFireInterval          = 0x50EC250; // float() Weapon (dump 739186)
        constexpr uintptr_t Weapon_get_FireInterval          = 0x5105C74; // float() Weapon (dump 740026, same as FiringTiming::get_FireInterval)
        constexpr uintptr_t Weapon_get_AimingTarget          = 0x510EAAC; // object() Weapon (dump 739828)
        constexpr uintptr_t Weapon_get_CurrentAimingTargetDis = 0x510EABC;// float() Weapon (dump 739834)
        constexpr uintptr_t Camera_get_fieldOfView           = 0x5021F7C; // float() Camera (dump 157456)
        constexpr uintptr_t BaseGame_GetTeamPawns            = 0x9CAFEB4; // PawnList*() BaseGame (dump 702853)
        constexpr uintptr_t BaseGame_get_IsPaused            = 0x9CAFDC4; // bool()  BaseGame (dump 702829)
        constexpr uintptr_t Pawn_get_IsAlive                 = 0x51B7C40; // bool()  Pawn Slot 49 (dump 714397)
        constexpr uintptr_t Pawn_get_LastFireTime            = 0x51B843C; // float() Pawn (dump 714442, diagnostics)

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

    // ---- Unity object liveness --------------------------------------------
    // Tools::IsPtrValid only answers "is this memory mapped and readable". A
    // UnityEngine.Object the game has already Destroyed still has its MANAGED
    // wrapper sitting in readable heap, so every pointer guard passes, while the
    // native object behind it is gone: Object.m_CachedPtr (dump.cs 169503,
    // Object TypeDefIndex 1761) is cleared the moment the native side dies.
    // Asking such an object for anything - Transform.get_position,
    // Component.get_transform, Pawn.get_TeamId, Weapon.StartFire - walks that
    // null native pointer and takes the client down.
    //
    // The enemy / team pawn lists keep an entry for a few frames after a pawn
    // despawns (death, respawn, match end), so this is exactly the entity-
    // detection path of a real match: the name-walk calls get_transform() on
    // every pawn in the list, and the aim-point walk calls
    // get_HeadPosition() / get_transform()->get_position() on every pawn under
    // the crosshair. A readable-but-dead pawn passed every earlier check and
    // crashed on the first native call.
    // UnityEngine.Object.m_CachedPtr (dump.cs 169503) - every Pawn / Weapon /
    // Transform / Collider derives from Object, so one offset covers all of them.
    constexpr size_t AF_OBJ_CACHEDPTR = 0x10;

    inline bool A_FireObjectAlive(void *obj)
    {
        if (!Tools::IsPtrValid(obj)) return false;
        void *cached = *(void **)((uintptr_t)obj + AF_OBJ_CACHEDPTR);
        return Tools::IsPtrValid(cached);
    }

    // The managed pawn lists hand out raw 8-byte object references. An il2cpp
    // reference is always 8-byte aligned, so an unaligned slot is a torn or
    // mid-rewrite list entry - reject it before any native call, the way V3's
    // A_FireIsReadablePawn (@c2c1e12) does.
    inline bool A_FireListObjectAlive(void *obj)
    {
        if (((uintptr_t)obj & (uintptr_t)0x7) != (uintptr_t)0) return false;
        return A_FireObjectAlive(obj);
    }

    // Pawn::IsAiming() @0x519BB10 - "the local player is scoping / ADS"
    inline bool IsLocalPlayerScoping()
    {
        void *localPawn = LocalPlayer();
        if (!IsReadable(localPawn)) return false;
        // `static`: UnityRva() re-opens and regex-scans /proc/self/maps on every
        // call - a full maps scan per query is not something a hot path may do.
        static auto isAiming = (bool (*)(void *))UnityRva(RVA::Pawn_IsAiming);
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
        static auto getIsAiming = (bool (*)(void *))UnityRva(RVA::GetIsAiming);   // maps scan once
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

    // Team gate used by the trigger scan: team id ONLY. IsSameCamp() is a
    // HOSTILE-camp test (true for any enemy hostile, bots included) - not a
    // team check - and Pawn::m_IsBot is only a marker (0x46464600 placeholder
    // in this dump), not trustworthy as "skip AI pawns". Short-circuiting on
    // either field silently dropped every PVE/training bot from the candidate
    // list - the "doesn't work against bots" report. Team id stays a real team
    // gate, so it alone decides "same team => skip".
    inline bool IsSameTeamAcceptBots(void *localPawn, void *targetPawn)
    {
        // get_TeamId is a native call - a destroyed pawn must not reach it.
        if (!A_FireObjectAlive(localPawn) || !A_FireObjectAlive(targetPawn)) return false;
        if (localPawn == targetPawn) return true;

        static auto getTeamId = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);       // Slot 216
        if (getTeamId == nullptr) return false;
        const uint32_t localTeam  = getTeamId(localPawn);
        const uint32_t targetTeam = getTeamId(targetPawn);
        return localTeam != 0 && localTeam == targetTeam;
    }

    // Function pointers. The per-class pawn tick overrides are the heartbeat
    // (see the header note); StartFire/StopFire are called through raw pointers,
    // never hooked.
    // fn_* are resolved once at install time - getAbsoluteAddress() re-opens
    // and regex-scans /proc/self/maps on EVERY call, so resolving them per scan
    // was the single biggest cost of the triggerbot loop.
    //
    // These live ABOVE the helpers because the ShouldFire tree below calls them,
    // and every one of them is allowed to stay null: a missing pointer makes the
    // gate fall through (never block), it does not disable the feature.
    inline void (*orig_Weapon_Tick)(void *, float) = nullptr;
    inline void (*orig_Pawn_Tick)(void *, float) = nullptr;
    inline void (*orig_PlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_MPPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_BRPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_DMZPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_PVEPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_OpenWorldPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_BRGroundWarPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*fn_Weapon_StartFire)(void *) = nullptr;
    inline void (*fn_Weapon_StopFire)(void *, bool) = nullptr;
    inline Vector3 (*fn_WorldToScreen)(void *, Vector3) = nullptr;  // 1-arg overload
    inline bool (*fn_PhysicsRaycastLOS)(Vector3, Vector3, float, void *) = nullptr;
    // Nearest-hit ray with an out RaycastHit and an explicit maxDistance
    // (0x506C188). Nothing in the tree can tell a wall from an enemy without
    // this: the old 3-param call always ran to infinity and was read by
    // distance, so anything within kRaySlack of the aim point counted as "the
    // enemy" - which is exactly how a thin panel in front of the target got
    // shot through.
    inline bool (*fn_PhysicsRaycastHitOut)(Vector3, Vector3, void *, float) = nullptr;
    // The same ray with no out struct (0x506BF60): the conservative fallback for
    // a build where the hit object cannot be named.
    inline bool (*fn_PhysicsRaycastSegment)(Vector3, Vector3, float) = nullptr;
    // RaycastAll 0x506CBB0 - returns the il2cpp RaycastHit[] for the whole
    // segment so the gate can walk hits nearest-first and skip the local
    // player's own geometry / non-target triggers (VISIBILITY note, file head).
    // Null when the symbol did not resolve -> single-ray fallback plus the
    // near-origin occlusion escape.
    inline void *(*fn_PhysicsRaycastAll)(Vector3, Vector3, float) = nullptr;
    // Collider.get_isTrigger() 0x50279D4 - only read on RaycastAll hits.
    inline bool (*fn_ColliderIsTrigger)(void *) = nullptr;
    // Scene-graph accessors used to name a hit: Transform.get_forward /
    // Transform.get_parent (see the RVA block).
    inline Vector3 (*fn_TransformGetForward)(void *) = nullptr;
    inline void   *(*fn_TransformGetParent)(void *) = nullptr;
    inline void *(*fn_CameraGetMain)() = nullptr;

    // Camera::WorldToScreenPoint(0x5023B54) behind a null-checked wrapper. The
    // scan called fn_WorldToScreen directly in four places; if 0x5023B54 did not
    // resolve at install time, the FIRST target projection - "aimed at a
    // player" - jumped through the null pointer and took the client down.
    // Missing symbol now degrades: the shot keeps its centre-ray visibility
    // verdict, only the pixel diagnostics are lost.
    inline bool A_FireWorldToScreenSafe(void *cam, const Vector3 &world, Vector3 &out)
    {
        if (fn_WorldToScreen == nullptr || !A_FireObjectAlive(cam)) return false;
        out = fn_WorldToScreen(cam, world);
        return true;
    }
    inline void *(*fn_GameGetMatchGame)() = nullptr;
    inline bool (*fn_PawnRecentlyRendered)(void *) = nullptr;
    inline bool (*fn_PawnLastOccVisible)(void *) = nullptr;
    inline bool (*fn_PawnIsAiming)(void *) = nullptr;
    inline bool (*fn_PawnGetIsFiring)(void *) = nullptr;

    // ---- ShouldFire tree pointers (every RVA verified, see the RVA block) ---
    inline bool  (*fn_WeaponIsAutoFire)(void *) = nullptr;            // 0x513D704
    inline bool  (*fn_WeaponIsBurstFire)(void *) = nullptr;           // 0x50EBB28
    inline bool  (*fn_WeaponIsSingleShot)(void *) = nullptr;          // 0x50EBA5C
    inline bool  (*fn_WeaponAmmoCanFire)(void *) = nullptr;           // 0x50EB73C
    inline int   (*fn_WeaponGetClipAmmoCount)(void *) = nullptr;      // 0x50EC614
    inline bool  (*fn_WeaponCanFire)(void *) = nullptr;               // 0x5126CC4
    inline bool  (*fn_WeaponInSwitchWeaponState)(void *) = nullptr;   // 0x512724C
    inline bool  (*fn_WeaponHasSwitchingState)(void *) = nullptr;     // 0x5124C70
    inline float (*fn_WeaponRealFireInterval)(void *) = nullptr;      // 0x50EC250
    inline void *(*fn_WeaponGetAimingTarget)(void *) = nullptr;       // 0x510EAAC
    inline float (*fn_WeaponGetAimingTargetDis)(void *) = nullptr;    // 0x510EABC
    inline float (*fn_CameraGetFov)(void *) = nullptr;                // 0x5021F7C
    inline void *(*fn_BaseGameGetTeamPawns)(void *) = nullptr;        // 0x9CAFEB4
    inline bool  (*fn_BaseGameIsPaused)(void *) = nullptr;            // 0x9CAFDC4
    inline bool  (*fn_PawnIsAlive)(void *) = nullptr;                 // 0x51B7C40 (second chance only)
    // =====================================================================
    //  ENHANCED TRIGGERBOT HELPERS
    //
    //  The scan below is a decision TREE, not a chain of filters: every branch
    //  either fires or returns ONE named reason (FireBlock) why it does not.
    //  The menu prints that reason live, so "why did it not shoot" is never a
    //  guess again.
    //
    //  Every gate answers a question the shot has to survive, in this order:
    //    1. local player / weapon readiness   (alive, in a match, not paused,
    //       weapon valid, not switching, ammo > 0, fire interval elapsed,
    //       the game's own Weapon.CanFire())
    //    2. trigger mode                      (Always / ADS only / Firing only)
    //    3. target acquisition & validity     (enemy, alive, not self, not a
    //       teammate, his on-screen BODY inside the crosshair zone)
    //    4. line of sight                     (camera -> aim point, raycast)
    //    5. rangeless + lead                  (travel time, predicted point,
    //       the predicted point's own line of sight)
    //    6. spread / recoil containment       (does the cone still cover the
    //       hitbox where the target will be when the bullet arrives?)
    //    7. friendly in the line of fire
    //  Only a candidate that survives all seven is fired at, and only while the
    //  crosshair keeps covering it (the trigger is released the moment it does
    //  not).
    // =====================================================================

    // Weapon fire mode. Weapon has no "get_FireMode" in the dump; the three
    // flags are asked one by one:
    //   Weapon.get_IsAutoFire()   0x513D704  dump 742570
    //   Weapon.get_IsBurstFire()  0x50EBB28  dump 739129
    //   Weapon.get_IsSingleShot() 0x50EBA5C  dump 739174
    enum class FireMode : int
    {
        Single  = 0,
        Burst   = 1,
        Auto    = 2,
        Unknown = 3
    };

    // Why the trigger did NOT fire. Exactly one of these is reported per scan.
    enum class FireBlock : int
    {
        Fired = 0,
        FeatureOff,          // the toggle is off
        NoLocalPawn,         // GamePlay::get_LocalPawn() invalid
        LocalNotAlive,       // Pawn::m_IsAlive @0x548 (Pawn.get_IsAlive 0x51B7C40 also asked)
        NoMatch,             // GamePlay::get_MatchGame() invalid -> not in a match
        MatchPaused,         // BaseGame.get_IsPaused() 0x9CAFDC4
        NoWeapon,            // Pawn.get_CurrentWeapon() 0x518BEA8 invalid
        WeaponSwitching,     // Weapon.InSwitchWeaponState 0x512724C / HasSwitchingState 0x5124C70
        NoAmmo,              // Weapon.AmmoCanFire 0x50EB73C / get_ClipAmmoCount 0x50EC614
        WeaponNotReady,      // Weapon.CanFire() 0x5126CC4 says no (reload/overheat/state)
        FireInterval,        // pacing against Weapon.RealFireInterval 0x50EC250
        ModeNotAds,          // TriggerbotMode 1 - not scoping (Pawn.IsAiming 0x519BB10)
        ModeNotFiring,       // TriggerbotMode 2 - you are not already firing
        NoEnemyInZone,       // nothing enemy/alive inside the crosshair zone
        NoLineOfSight,       // camera -> aim point is blocked by geometry
        LeadNoLineOfSight,   // camera -> PREDICTED point is blocked
        TravelTooLong,       // bullet would need longer than kMaxTravelTime
        ShotWontLand,        // spread/lead put the predicted point off the hitbox
        FriendlyInLine,      // a teammate is inside the bullet path
        Count
    };

    inline const char *A_FireBlockText(FireBlock b)
    {
        switch (b)
        {
            case FireBlock::Fired:             return "firing";
            case FireBlock::FeatureOff:        return "triggerbot off";
            case FireBlock::NoLocalPawn:       return "no local pawn";
            case FireBlock::LocalNotAlive:     return "you are dead";
            case FireBlock::NoMatch:           return "no match";
            case FireBlock::MatchPaused:       return "match paused";
            case FireBlock::NoWeapon:          return "no weapon";
            case FireBlock::WeaponSwitching:   return "switching weapon";
            case FireBlock::NoAmmo:            return "no ammo";
            case FireBlock::WeaponNotReady:    return "weapon not ready";
            case FireBlock::FireInterval:      return "fire interval";
            case FireBlock::ModeNotAds:        return "not scoped (mode)";
            case FireBlock::ModeNotFiring:     return "not firing (mode)";
            case FireBlock::NoEnemyInZone:     return "no enemy in crosshair";
            case FireBlock::NoLineOfSight:     return "wall in the way";
            case FireBlock::LeadNoLineOfSight: return "lead path blocked";
            case FireBlock::TravelTooLong:     return "bullet travel too long";
            case FireBlock::ShotWontLand:      return "spread/lead would miss";
            case FireBlock::FriendlyInLine:    return "teammate in the line";
            default:                           return "?";
        }
    }

    // The numbers the tree uses. All of them are physical (metres, seconds,
    // degrees) - never a screen size - so the feature behaves the same on any
    // resolution or aspect ratio.
    struct A_FireTune
    {
        // NOTE: the old kRaySlack = 0.40 is deliberately GONE. "The first hit is
        // within 40 cm of the aim point, so it must be the enemy" is precisely
        // the rule that shot through thin walls. A ray is now only clear when
        // the object it hit IS the target (A_FireRayToTarget).
        static constexpr float kRayReach        = 1.00f;   // m - how far past the aim point the nearest-hit ray may look
        static constexpr float kSurfaceFrac     = 0.80f;   // - fallback stem stops this far inside the hitbox radius
        static constexpr float kMinStemGap      = 0.05f;   // m - floor for that stem (never fires a 0-length ray)
        static constexpr float kHeadRadius      = 0.14f;   // m - head hitbox half width
        static constexpr float kBodyRadius      = 0.30f;   // m - torso/limb hitbox half width
        static constexpr float kFriendlyRadius  = 0.35f;   // m - teammate capsule half width
        static constexpr float kSelfExclusion    = 0.15f;   // m - a blocker this close to the
                                                           // camera only proves the camera is
                                                           // clipping geometry; the verdict is
                                                           // then handed to the game's own
                                                           // occlusion answer (LastOccVisible)
                                                           // instead of refusing the shot
        static constexpr float kCrosshairRayMax = 400.0f;  // m - the centre ray is rangeless in practice
        static constexpr float kMaxTravelTime   = 1.20f;   // s - beyond this the lead is a guess (ADVISORY only)
        static constexpr float kAssumedSpeed    = 800.0f;  // m/s - only when no fire component answers
        static constexpr float kMinFireInterval = 0.030f;  // s   - pacing floor
        static constexpr float kFallbackFov     = 60.0f;   // deg - only if Camera.get_fieldOfView fails
        static constexpr float kDegToRad        = 0.01745329f;
        static constexpr float kRadToDeg        = 57.29578f;
    };

    // An aim point on a pawn: where the crosshair has to be, how big the hitbox
    // living there is (spread-containment test), plus the pawn's head and feet in
    // world space so the scan can measure the on-screen BODY instead of one
    // floating point. There is NO user-facing hitbox choice any more.
    struct A_FireAimPoint
    {
        Vector3 pos;       // chest (or head when the root transform did not answer)
        Vector3 head;      // Pawn.get_HeadPosition() RVA 0x51B86EC
        Vector3 feet;      // pawn root transform position
        float   radius;    // metres
        bool    valid;
        bool    headOk;
        bool    feetOk;
    };

    // One enemy inside the crosshair zone, with everything the tree needs.
    struct A_FireCandidate
    {
        Pawn          *pawn;
        A_FireAimPoint aim;
        float          pxOffset;    // px from the crosshair to the aim point
        float          worldDist;   // m  camera -> aim point
        Vector3        velocity;    // m/s target velocity
    };

    // Weapon.CurrentFireComponent (dump 739078, RVA 0x50EB1E4). This dump prints
    // every object/struct return as "ref object" (get_MatchGame and
    // WorldToScreenPoint print identically and both are called by value here in
    // game), so it is called by value.
    inline void *GetCurrentFireComponent(void *weapon)
    {
        if (!A_FireObjectAlive(weapon)) return nullptr;
        static auto fn = (void *(*)(void *))UnityRva(RVA::Weapon_get_CurrentFireComponent);
        if (fn == nullptr) return nullptr;
        void *comp = fn(weapon);
        return A_FireObjectAlive(comp) ? comp : nullptr;
    }

    // Bullet speed in m/s, from the live fire component. `known` reports whether
    // the game answered - a guessed speed must NOT be used to lead a shot.
    inline float GetWeaponBulletSpeed(void *weapon, bool *known = nullptr)
    {
        if (known != nullptr) *known = false;
        void *fireComp = GetCurrentFireComponent(weapon);
        if (Tools::IsPtrValid(fireComp)) {
            // WeaponFireComponent.get_BulletInitSpeed, base class @0xC14A158
            // (reads m_BulletInitSpeed @0x220) - correct for every subclass.
            static auto getSpeed = (float (*)(void *))UnityRva(RVA::WeaponFireComponent_get_BulletInitSpeed);
            if (getSpeed != nullptr) {
                const float speed = getSpeed(fireComp);
                if (speed >= 1.0f && speed <= 5000.0f) {
                    if (known != nullptr) *known = true;
                    return speed;
                }
            }
        }
        // No verified answer. The caller must treat this as "hitscan-ish": no
        // lead, no travel-time gate, so an unknown speed can never block a shot.
        return A_FireTune::kAssumedSpeed;
    }

    // Fire mode of the held weapon - the three dump flags, single-shot LAST so
    // a weapon that reports none of them still behaves (semi) safely.
    inline FireMode GetWeaponFireMode(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return FireMode::Unknown;
        if (fn_WeaponIsAutoFire != nullptr && fn_WeaponIsAutoFire(weapon)) return FireMode::Auto;
        if (fn_WeaponIsBurstFire != nullptr && fn_WeaponIsBurstFire(weapon)) return FireMode::Burst;
        if (fn_WeaponIsSingleShot != nullptr && fn_WeaponIsSingleShot(weapon)) return FireMode::Single;
        // No flag answered: assume an automatic weapon so one press is enough
        // and the weapon's own fire interval controls the rate.
        return FireMode::Auto;
    }

    // Target velocity for leading (Pawn.get_Velocity @0x51B38CC, dump 713743).
    inline Vector3 GetPawnVelocity(void *pawn)
    {
        if (!A_FireObjectAlive(pawn)) return Vector3::zero();
        static auto getVelocity = (Vector3 (*)(void *))UnityRva(RVA::Pawn_get_Velocity);
        if (getVelocity != nullptr) return getVelocity(pawn);
        return Vector3::zero();
    }

    // Camera focal length in pixels: (screenHeight/2) / tan(fov/2).
    // Camera.get_fieldOfView() 0x5021F7C (dump 157456) is a plain float getter,
    // so the shot geometry (px <-> metres at a distance) is derived, never
    // guessed from a hardcoded field of view.
    inline float A_FireFocalPx()
    {
        float fov = A_FireTune::kFallbackFov;
        if (fn_CameraGetFov != nullptr && fn_CameraGetMain != nullptr) {
            void *cam = fn_CameraGetMain();
            if (Tools::IsPtrValid(cam)) {
                const float v = fn_CameraGetFov(cam);
                if (v > 1.0f && v < 179.0f) fov = v;
            }
        }
        const float tanHalf = tanf(fov * 0.5f * A_FireTune::kDegToRad);
        if (!(tanHalf > 0.0001f)) return 0.0f;
        return ((float)get_height() * 0.5f) / tanHalf;
    }

    // Point at `t` of the way from the feet up to the head (0 = feet mesh root,
    // 1 = head bone). Used to probe a body height without a spine bone: this dump
    // exposes no instance spine/chest bone that works on every pawn subclass
    // (PlayerPawn.m_SpineBone @0x1D68 is a STATIC field, and
    // PlayerPawn.get_SpineBone() 0x5477EA8 only exists on PlayerPawn - not on the
    // BR / MP / PVE pawns), so the torso height stays an axis fraction.
    inline Vector3 A_FireAlongBody(const Vector3 &head, const Vector3 &feet, float t)
    {
        return Vector3(feet.x + (head.x - feet.x) * t,
                       feet.y + (head.y - feet.y) * t,
                       feet.z + (head.z - feet.z) * t);
    }

    // Squared screen distance from (px,py) to the segment (ax,ay)->(bx,by), all
    // in pixels. This is how "is the crosshair on this pawn's BODY" is answered
    // now: a body is a SEGMENT on screen, not a point, so a body that fills the
    // screen at 2 m and a body a few px tall at 150 m are measured the same way
    // and neither depends on a bone field offset.
    inline float A_FirePointSegDistSq(float px, float py,
                                      float ax, float ay, float bx, float by)
    {
        const float vx = bx - ax, vy = by - ay;
        const float wx = px - ax, wy = py - ay;
        const float vv = vx * vx + vy * vy;
        float t = 0.0f;
        if (vv > 0.0001f) {
            t = (wx * vx + wy * vy) / vv;
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
        }
        const float dx = px - (ax + vx * t);
        const float dy = py - (ay + vy * t);
        return dx * dx + dy * dy;
    }

    // Where on this pawn to aim. There is NO user hitbox pick any more, and no
    // raw bone FIELD offset either:
    //
    //   head  - Pawn.get_HeadPosition() RVA 0x51B86EC (Slot 215, dump 714489).
    //           This is the game's OWN answer and the very getter DrawESP already
    //           calls in game every frame, so it keeps working when the
    //           m_HeadBone @0x308 / m_Mesh @0x628 FIELD offsets drift between
    //           builds - which is exactly why the old bone-field walk found ZERO
    //           candidates in BR (device diag: Enemy=5 Cand=0 while the centre
    //           ray still named pawns 597 times).
    //   chest - 0.72 of the feet->head axis (the plain midpoint is the HIPS, so
    //           aiming centre-mass was validated against a point below the chest).
    //   feet  - the pawn root transform (Component.get_transform ->
    //           Transform.get_position), the same call the hit-naming walk uses.
    //
    // A zero head vector means the game did not answer and the root position is
    // used instead - one of the two is enough, so a pawn is never dropped just
    // because one accessor failed.
    inline A_FireAimPoint A_FireAimPointFor(Pawn *pawn)
    {
        A_FireAimPoint ap{};
        ap.valid = false;
        ap.radius = A_FireTune::kBodyRadius;
        if (!A_FireObjectAlive((void *)pawn)) return ap;

        const Vector3 head = pawn->get_HeadPosition();
        ap.head = head;
        ap.headOk = (head.x != 0.0f || head.y != 0.0f || head.z != 0.0f);

        Transform *root = pawn->get_transform();
        const bool rootOk = A_FireObjectAlive((void *)root);
        const Vector3 feet = rootOk ? root->get_position() : Vector3::zero();
        ap.feet = feet;
        ap.feetOk = rootOk;

        if (ap.headOk && ap.feetOk) {
            ap.pos = A_FireAlongBody(head, feet, 0.72f);        // chest
            ap.radius = A_FireTune::kBodyRadius;
        } else if (ap.headOk) {
            ap.pos = head;                                      // head only
            ap.radius = A_FireTune::kHeadRadius;
        } else if (ap.feetOk) {
            ap.pos = Vector3(feet.x, feet.y + 1.05f, feet.z);   // eye height off the root
            ap.radius = A_FireTune::kBodyRadius;
        } else {
            return ap;                                          // the game answered nothing
        }
        ap.valid = true;
        return ap;
    }

    // ---- Weapon readiness -------------------------------------------------
    // All of these sit on the same object the scan already holds (a Weapon*):
    //   Weapon.AmmoCanFire()           0x50EB73C  dump 739774
    //   Weapon.get_ClipAmmoCount()     0x50EC614  dump 739204
    //   Weapon.CanFire()               0x5126CC4  dump 740581
    //   Weapon.InSwitchWeaponState()   0x512724C  dump 740662
    //   Weapon.HasSwitchingState()     0x5124C70  dump 740707
    // CanFire() is the game's own gate (it covers reloading, overheat, bolt
    // cycling, fire-mode transitions and the fire interval). Its base
    // implementation is the one every normal gun uses - only WeaponDualWield,
    // WeaponBoxing, the turrets, the sentry gun and the helicopter override it.
    // Every call FAILS OPEN on a missing symbol: an unresolved address must
    // never be the reason a legitimate shot is dropped.
    inline bool A_FireWeaponSwitching(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponInSwitchWeaponState != nullptr && fn_WeaponInSwitchWeaponState(weapon)) return true;
        if (fn_WeaponHasSwitchingState  != nullptr && fn_WeaponHasSwitchingState(weapon))  return true;
        return false;
    }

    inline bool A_FireWeaponHasAmmo(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponAmmoCanFire != nullptr && !fn_WeaponAmmoCanFire(weapon)) return false;
        if (fn_WeaponGetClipAmmoCount != nullptr && fn_WeaponGetClipAmmoCount(weapon) <= 0) return false;
        return true;
    }

    inline bool A_FireWeaponCanFire(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponCanFire == nullptr) return true;      // unknown -> never block
        return fn_WeaponCanFire(weapon);
    }

    // The weapon's own fire interval, in seconds
    // (Weapon.RealFireInterval 0x50EC250 dump 739186, Weapon.get_FireInterval
    // 0x5105C74 dump 740026). Used as the trigger's re-press pacing, so the
    // triggerbot never hammers StartFire faster than the gun can shoot.
    inline float A_FireWeaponInterval(void *weapon)
    {
        float interval = 0.0f;
        if (fn_WeaponRealFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
            const float v = fn_WeaponRealFireInterval(weapon);
            if (v > 0.0f && v < 10.0f) interval = v;
        }
        if (interval <= 0.0f) {
            static auto getFireInterval = (float (*)(void *))UnityRva(RVA::Weapon_get_FireInterval);
            if (getFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
                const float v = getFireInterval(weapon);
                if (v > 0.0f && v < 10.0f) interval = v;
            }
        }
        if (!(interval > 0.0f)) interval = A_FireTune::kMinFireInterval;
        if (interval < A_FireTune::kMinFireInterval) interval = A_FireTune::kMinFireInterval;
        return interval;
    }

    // A pawn is alive if the game's OWN getter says so, or failing that if the
    // m_IsAlive @0x548 field (the read DrawESP does) says so. The getter
    // (Pawn.get_IsAlive 0x51B7C40, Slot 49 - dump 714396) is asked FIRST because
    // it cannot drift with a field offset; only "dead" from BOTH blocks a shot.
    inline bool A_FirePawnAlive(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        if (fn_PawnIsAlive != nullptr && fn_PawnIsAlive(pawn)) return true;
        return ((Pawn *)pawn)->m_IsAlive();
    }

    // ---- Spread / lead containment ---------------------------------------
    // Will the bullet still land on the hitbox?
    // The bullet leaves along the crosshair, the target keeps moving while the
    // bullet flies, and the weapon's spread opens a cone around that line. All
    // of it is compared in one unit - pixels at the crosshair - using the camera's
    // focal length from get_fieldOfView:
    //     hitboxPx = hitboxRadius (m) * focalPx / distance   (how big the hitbox is)
    //     spreadPx = tan(ShotSpreadSize) * focalPx           (how wide the cone is)
    //     missPx   = px distance from the crosshair to the PREDICTED point
    // The shot goes out while missPx <= max(zonePx, hitboxPx) + spreadPx:
    //   - zonePx is the user's Radius slider, i.e. how much aim error is allowed
    //     by choice; the physics test never becomes STRICTER than that slider
    //     (at long range a 1 px hitbox would otherwise veto shots the user aimed),
    //     and it becomes LOOSER when the hitbox is bigger than the slider zone
    //     (point blank the head covers far more than the zone).
    //   - spreadPx adds the weapon's own cone, so a shot that the spread alone
    //     would throw off the hitbox is not taken.
    //   - the LEAD lands here too: missPx is the PREDICTED point, so a crosshair
    //     left on where the target WAS is rejected instead of fired at.
    inline bool A_FireShotWouldLand(float missPx, float zonePx, float hitboxPx, float spreadPx)
    {
        if (!(missPx >= 0.0f)) return true;          // no answer -> do not block
        float allowed = hitboxPx > zonePx ? hitboxPx : zonePx;
        if (allowed < 1.0f) allowed = 1.0f;          // degenerate distance/px scale
        if (spreadPx > 0.0f && spreadPx < 40000.0f) allowed += spreadPx;
        return missPx <= allowed;
    }

    // Spread cone radius in pixels at the crosshair.
    // Weapon.get_ShotSpreadSize 0x5109B68 (dump 739732) is the only spread getter
    // in this dump. Its unit is not documented, so it is read as degrees and any
    // absurd value disables the spread part (fail-open): a wrong unit must not be
    // able to stop the trigger.
    inline float A_FireSpreadPx(void *weapon, float focalPx, bool checkSpread)
    {
        if (!checkSpread) return 0.0f;
        if (!A_FireObjectAlive(weapon) || !(focalPx > 0.0f)) return 0.0f;
        static auto getSpread = (float (*)(void *))UnityRva(RVA::Weapon_get_ShotSpreadSize);
        if (getSpread == nullptr) return 0.0f;
        const float spread = getSpread(weapon);
        if (!(spread >= 0.0f) || spread > 45.0f) return 0.0f;   // NaN / not degrees
        return tanf(spread * A_FireTune::kDegToRad) * focalPx;
    }

    // ---- Friendly in the line of fire ------------------------------------
    // The bullet flies along camera -> aim point. Any living TEAMMATE whose body
    // is within kFriendlyRadius of that line and closer than the target would
    // take the bullet. Pure geometry, so it also catches the teammate the ray
    // misses but the spread cone would hit.
    //
    // Positions are collected ONCE per scan (A_FireCollectFriendProbes) because
    // they are identical for every candidate: the old version re-walked the whole
    // team list, with two native reads per mate, for EVERY candidate of EVERY
    // scan - on a 4-man team that is 8 native calls per candidate wasted.
    struct AF_FriendProbe { Vector3 feet; Vector3 chest; };

    inline int A_FireCollectFriendProbes(Pawn *localPawn, AF_FriendProbe *out, int maxOut)
    {
        if (out == nullptr || maxOut <= 0) return 0;
        if (fn_BaseGameGetTeamPawns == nullptr || fn_GameGetMatchGame == nullptr) return 0;

        void *game = fn_GameGetMatchGame();
        if (!Tools::IsPtrValid(game)) return 0;
        List<Pawn *> *team = (List<Pawn *> *)fn_BaseGameGetTeamPawns(game);
        if (!Tools::IsPtrValid((void *)team)) return 0;
        Pawn **items = (Pawn **)team->getItems();
        const int count = team->getSize();
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 128) return 0;

        int n = 0;
        for (int i = 0; i < count && n < maxOut; i++) {
            Pawn *mate = items[i];
            if (!A_FireListObjectAlive((void *)mate)) continue;
            if (mate == localPawn) continue;
            if (!A_FirePawnAlive(mate)) continue;

            Transform *t = mate->get_transform();
            if (!A_FireObjectAlive((void *)t)) continue;
            const Vector3 pos = t->get_position();
            out[n].feet = pos;
            // Two probes per teammate, because m_Mesh/parent transform sits at the
            // FEET: one at the origin and one at chest height. A teammate in front
            // of the muzzle is normally only in the bullet path at chest height.
            out[n].chest = Vector3(pos.x, pos.y + 1.15f, pos.z);
            n++;
        }
        return n;
    }

    inline bool A_FireFriendlyInLine(const AF_FriendProbe *probes, int probeCount,
                                     const Vector3 &camPos, const Vector3 &dir,
                                     float targetDist, int *friendsSeen = nullptr)
    {
        if (friendsSeen != nullptr) *friendsSeen = 0;
        if (probes == nullptr || probeCount <= 0) return false;

        bool blocked = false;
        for (int i = 0; i < probeCount; i++) {
            const Vector3 probe[2] = { probes[i].feet, probes[i].chest };
            bool mateInLine = false;
            for (int k = 0; k < 2; k++) {
                const Vector3 to(probe[k].x - camPos.x, probe[k].y - camPos.y, probe[k].z - camPos.z);

                const float dist = Vector3::Magnitude(to);
                if (dist < 0.2f || dist >= targetDist) continue;  // behind the target -> harmless

                // Lateral (perpendicular) distance from that point to the shot ray.
                const float along = Vector3::Dot(to, dir);
                const Vector3 lat(to.x - dir.x * along, to.y - dir.y * along, to.z - dir.z * along);
                const float lateral = Vector3::Magnitude(lat);

                if (lateral <= A_FireTune::kFriendlyRadius) { mateInLine = true; break; }
            }
            if (mateInLine) {
                if (friendsSeen != nullptr) (*friendsSeen)++;   // teammates that BLOCKED
                blocked = true;
            }
        }
        return blocked;
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

    // State - game thread only.
    inline bool     g_afireHoldingFire       = false;
    inline void    *g_afireFiringOn          = nullptr;
    inline double   g_afireLastPressTime     = -1000.0;
    inline double   g_afireLastEvalTime      = -1000.0;
    inline void    *g_afireTargetPawn       = nullptr; // the pawn the trigger is currently on

    inline bool     g_afireInTick            = false;   // re-entrancy guard
    inline std::atomic<bool> g_afireLosOk             = false;   // last scan: target has LOS (game wb / render read)
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
        // ---- ShouldFire tree (see FireBlock) -----------------------------
        int    blockCount[(int)FireBlock::Count];  // lifetime counter per FireBlock reason
        int    lastBlock;       // FireBlock of the most recent scan
        int    candidates;      // enemies inside the crosshair zone, last scan
        int    filteredOut;     // ... dropped by the user's target filter, last scan
        int    losChecked;      // candidates that got a visibility ray, last scan
        int    friendsInLine;   // teammates that blocked a shot, last scan
        bool   ammoOk;          // Weapon.AmmoCanFire()/get_ClipAmmoCount() verdict
        bool   canFire;         // Weapon.CanFire() verdict
        float  focalPx;         // Camera.get_fieldOfView() -> focal length in px
        float  lastAimPx;       // px crosshair -> chosen aim point
        float  lastPredPx;      // px crosshair -> lead point
        float  lastHitboxPx;    // hitbox half width in px at that distance
        float  lastSpreadPx;    // spread cone radius in px
        float  lastTravel;      // bullet travel time (s)
        float  lastWorldDist;   // camera -> aim point (m)
        float  lastBulletSpeed; // m/s (assumed speed if speedKnown is false)
        // ---- visibility ray bookkeeping (the anti-wall gate) --------------
        int    aimPath;         // 0 = centre-of-screen ray accepted, 1 = zone scan
        unsigned crosshairPawn; // pawn the centre ray landed on (0 = nothing named)
        float  crosshairHitDist;// m, centre ray first-hit distance (-1 = no hit)
        int    raysIdentified;  // last scan: rays whose nearest hit WAS the target
        int    raysBlocked;     // last scan: rays stopped by something else
        bool   rayUnknown;      // last scan: a ray symbol did not resolve (fail closed)
        // ---- why a ray skipped hits (the false-"wall" fix, see VISIBILITY) --
        int    selfHitsSkipped;   // last scan: hits on OUR OWN pawn/weapon skipped
        int    triggerHitsSkipped;// last scan: invisible trigger volumes skipped
        int    occOverrides;      // last scan: near-origin blockers overruled by
                                  // the game's own LastOccVisible verdict
        float  lastLosHitDist;  // m, nearest hit on the camera->aim-point line
        bool   switching;       // Weapon.InSwitchWeaponState / HasSwitchingState (advisory)
        bool   leadPathBlocked; // lead point was covered - the shot fell back to the aim point
        bool   spreadWarn;      // spread/lead containment said "would miss" (advisory)
        float  lastFireInterval;// s, the weapon's own interval (measured, NOT enforced)
        unsigned aimingTarget;  // Weapon.get_AimingTarget() - the game's own answer
        float  aimingTargetDis; // Weapon.get_CurrentAimingTargetDis()

                                // 4 no StartFire ptr, 5 pointers ok / no heartbeat
    };
    inline TriggerbotDiag g_triggerDiag{};

    // Debug state for the menu: 0 = no target, 1 = target, 2 = firing.
    inline std::atomic<int> s_dbgTriggerState = 0;

    inline double A_FireNow()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // ---- cached "who is the local player" ----------------------------------
    // The tick hooks below run for EVERY pawn and EVERY weapon in the world -
    // hundreds of native calls per frame at 60/120 Hz - and each one used to pay
    // a full GamePlay::get_LocalPawn() (a real il2cpp call) just to ask "is this
    // the local player?". That answer cannot change inside 50 ms, so it is
    // cached: the worst case is noticing a pawn/weapon swap one frame late, while
    // the per-frame native traffic of the hooks collapses from one call per tick
    // to roughly twenty calls per second. This is the triggerbot's own share of
    // the frame cost, and it is the part that scaled with how many pawns the
    // match has.
    inline Pawn  *g_afireLocalPawnCache     = nullptr;
    inline double g_afireLocalPawnCacheTime = -1000.0;
    inline void  *g_afireHeldWeaponCache    = nullptr;
    inline double g_afireHeldWeaponStamp    = -1000.0;
    inline double g_afireOffDiagStamp       = -1000.0;

    inline Pawn *A_FireLocalPawnFast()
    {
        const double now = A_FireNow();
        if (g_afireLocalPawnCache != nullptr &&
            (now - g_afireLocalPawnCacheTime) < 0.05)
            return g_afireLocalPawnCache;
        Pawn *p = GamePlay::get_LocalPawn();
        g_afireLocalPawnCache = p;
        g_afireLocalPawnCacheTime = now;
        return p;
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
        return A_FireObjectAlive(localPawn) &&
               ((Pawn *)localPawn)->m_IsAlive();
    }

    // Record why the last scan did (not) fire. Lifetime counters per reason plus
    // the last one, which the menu prints live - "why did it not shoot" is
    // answered by the feature itself instead of by guessing.
    inline void A_FireReportBlock(FireBlock b)
    {
        TriggerbotDiag &d = g_triggerDiag;
        const int idx = (int)b;
        d.lastBlock = idx;
        if (idx >= 0 && idx < (int)FireBlock::Count) d.blockCount[idx]++;
    }


    // =====================================================================
    //  TARGET + SHOT VALIDATION
    // =====================================================================

    // Everything the fire path knows about the pawn it is about to shoot.
    struct TriggerTarget
    {
        Pawn          *pawn = nullptr;
        A_FireAimPoint aim;                  // the validated aim point + hitbox
        Vector3        aimPoint;             // == aim.pos (kept for older call sites)
        Vector3        screenPos;            // aim point in screen space
        float          pxOffset = -1.0f;     // px from the crosshair to the aim point
        float          worldDist = 0.0f;     // m, camera -> aim point
        float          travelTime = 0.0f;    // s, bullet travel time
        Vector3        predicted;            // aim point + velocity * travelTime
        float          predPxOffset = -1.0f; // px from the crosshair to the predicted point
        float          hitboxPx = 0.0f;      // hitbox half width at that distance, in px
        float          spreadPx = 0.0f;      // spread cone radius, in px
        float          focalPx = 0.0f;       // camera focal length used (px)
        float          bulletSpeed = 0.0f;   // m/s
        bool           speedKnown = false;   // the game answered the bullet speed
        bool           leadUsed = false;     // the predicted point was used
        int            path = 1;             // 0 = centre-of-screen ray (path A), 1 = crosshair zone (path B)
        bool           visible = false;      // the shot line to the aim point was proven clear
        bool           visibilityIdentified = false; // ... and the thing ON that line was this very pawn
    };

    // ---- Line of sight: the nearest MEANINGFUL hit has to BE the pawn ------
    //
    // The old gate read ONE number - the first hit distance - and called the
    // line clear when that hit was within 0.40 m of the aim point. For an enemy
    // standing behind a thin panel whose front face is 30 cm in front of his
    // head bone, the first hit IS the panel, 30 cm short of the aim point, and
    // the shot went through the wall. It also produced the mirror-image failure:
    // the no-maxDistance overload ran the ray past the target, so scenery
    // BEHIND the enemy came back as the first hit and "no line of sight" was
    // reported on a clean shot.
    //
    // The next version named the single nearest hit and refused whenever that
    // hit was not the target. Correct against walls, but it broke on device:
    // the ray STARTS AT THE CAMERA, and the camera sits inside the local
    // player's own collider stack (head / upper-body hitbox, aim-assist volume,
    // weapon). PhysX reports an initial-overlap hit at distance ~0 for the
    // collider containing the ray origin, so the "first thing on the line" was
    // OUR OWN BODY a few centimetres from the camera - never the enemy - and
    // every shot was vetoed. Device diag from the broken build:
    //     Scan=9801 Cand=1 Ray=1 Tgt=0 Fire=0   LOShit=0.00m  Rays=0/1
    //
    // The gate below therefore does not look at "the first hit", it looks at
    // the first hit that can honestly block a bullet the player can see fired:
    //
    //   Physics.RaycastAll(origin, dir, maxDistance) 0x506CBB0 (dump.cs 174011)
    //   returns EVERY hit in the segment; they are walked nearest-first and:
    //     hit in the LOCAL pawn's hierarchy      -> SKIPPED (our own body /
    //                                               weapon / aim volume)
    //     non-target TRIGGER collider            -> SKIPPED (invisible volumes
    //                                               cannot be "the wall")
    //     remaining first hit IS the target      -> clear (his body, his gear,
    //                                               even an arm at the muzzle -
    //                                               the bullet reaches HIM)
    //     remaining first hit is anything else   -> blocked (a wall however
    //                                               thin, a door, a vehicle,
    //                                               another player)
    //     nothing meaningful on the line         -> clear
    //     a hit that cannot be named             -> conservative stem test
    //                                               (stops short of the target's
    //                                               own collider surface)
    //     RaycastAll symbol / result unusable    -> single nearest-hit Raycast
    //                                               0x506C188 + the escape below
    //     no ray symbol at all                   -> Unknown, fail closed
    //
    // NEAR-ORIGIN ESCAPE: a "blocker" within kSelfExclusion (15 cm) of the
    // camera only proves the camera is clipping geometry - it cannot answer
    // whether a target at range is behind cover. In that one case the game's
    // own occlusion verdict decides: Pawn.get_LastOccVisible 0x51BA800 (Slot
    // 235, last frame's occlusion-culling result; RecentlyRendered 0x51BA7E8
    // second chance) true -> clear (the game rendered him - the player sees
    // him), false -> blocked. Never consulted for a blocker at normal range,
    // so a real wall keeps vetoing the shot exactly as before.
    //
    constexpr size_t AF_RAYHIT_SIZE      = 0x30;  // through m_Collider @0x28 + pad
    constexpr size_t AF_RAYHIT_DISTOFF   = 0x1C;  // m_Distance for THIS build
    constexpr size_t AF_RAYHIT_COLLOFF   = 0x28;  // m_Collider for THIS build

    // What a trace along the shot line found.
    enum class AF_Ray : int
    {
        Clear   = 0,   // nothing, or the target itself
        Blocked = 1,   // something that is not the target is in the way
        Unknown = 2    // no ray symbol resolved - caller must fail closed
    };

    // The collider the ray stopped on (null when the struct is unusable).
    inline void *A_FireHitCollider(const unsigned char *hitBuf)
    {
        if (hitBuf == nullptr) return nullptr;
        void *collider = *(void *const *)(hitBuf + AF_RAYHIT_COLLOFF);
        return Tools::IsPtrValid(collider) ? collider : nullptr;
    }

    // Collider -> Transform through the repo's own Component::get_transform
    // (Class_Component_get_transform, the same call DrawESP has used in game for
    // ages), so naming a hit needs no new il2cpp method lookup.
    inline void *A_FireColliderTransform(void *collider)
    {
        // get_transform() is a native call: a ray can hit a collider whose
        // managed wrapper the scene is already tearing down, and that wrapper
        // still passes a plain readability check with a null m_CachedPtr.
        if (!A_FireObjectAlive(collider)) return nullptr;
        return (void *)((Component *)collider)->get_transform();
    }

    // Does `hitTransform` belong to `pawn`? Walks up the parent chain (bounded),
    // accepting the pawn's own transform and its two known bones. A bone hit and a
    // body-mesh hit both reach the pawn's root transform, so the walk is what
    // makes this work for any collider layout without a hitbox enumeration API.
    inline bool A_FireHitIsTarget(void *hitTransform, Pawn *pawn, int *depthOut = nullptr)
    {
        if (depthOut != nullptr) *depthOut = -1;
        if (!A_FireObjectAlive(hitTransform) || !A_FireObjectAlive((void *)pawn)) return false;

        // The pawn's own root transform. The old version also compared the raw
        // m_HeadBone @0x308 / m_Mesh @0x628 FIELDS; those are the offsets that did
        // not hold up in BR (Enemy=5 Cand=0), so they are gone. A hit on any bone
        // still reaches this root through the parent chain - that is what makes a
        // head, a backpack or an arm count as "the enemy".
        Transform *root = pawn->get_transform();
        if (!Tools::IsPtrValid((void *)root)) return false;

        void *t = hitTransform;
        for (int depth = 0; depth < 24 && A_FireObjectAlive(t); depth++) {
            if (t == (void *)root) {
                if (depthOut != nullptr) *depthOut = depth;
                return true;
            }
            if (fn_TransformGetParent == nullptr) break;     // cannot walk -> stop
            void *parent = fn_TransformGetParent(t);
            if (!Tools::IsPtrValid(parent) || parent == t) break;
            t = parent;
        }

        // The walk did not reach the root (a deep skeleton, or a collider parented
        // under a bone further than the bound). Fall back to the game's own head
        // getter: a hit sitting on the pawn's body IS the pawn for our purpose, and
        // this is the same accessor DrawESP uses, so it does not depend on any of
        // the bone field offsets.
        const Vector3 head = pawn->get_HeadPosition();
        if (head.x != 0.0f || head.y != 0.0f || head.z != 0.0f) {
            const Vector3 hp = ((Transform *)hitTransform)->get_position();
            const float dx = hp.x - head.x, dy = hp.y - head.y, dz = hp.z - head.z;
            const float r = 0.60f;
            if (dx * dx + dy * dy + dz * dz <= r * r) return true;
        }
        return false;
    }

    // The game's own "was this pawn actually drawn / not occluded last frame"
    // verdicts. Used ONLY by the near-origin escape above.
    inline bool A_FireGameSaysVisible(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        if (fn_PawnLastOccVisible != nullptr && fn_PawnLastOccVisible(pawn)) return true;
        if (fn_PawnRecentlyRendered != nullptr && fn_PawnRecentlyRendered(pawn)) return true;
        return false;
    }

    // ---- Physics.RaycastAll result (il2cpp RaycastHit[]) --------------------
    // Il2CppArray, arm64 LP64: klass @0x00, monitor @0x08, bounds* @0x10 (null
    // for a single-dimension array), max_length @0x18, first element @0x20.
    // RaycastHit elements are AF_RAYHIT_SIZE bytes with m_Distance @0x1C and
    // m_Collider @0x28 (dump.cs 175618, re-checked against jeudhehs/dump).
    // Every one of these reads is sanity-checked: a layout mismatch makes
    // A_FireRaycastAllNearest() return false and the caller degrades to the
    // single-ray path - it can never turn into an unnoticed "clear".
    constexpr size_t AF_ARRAY_LEN_OFF  = 0x18;
    constexpr size_t AF_ARRAY_DATA_OFF = 0x20;
    constexpr int    AF_MAX_RAY_HITS   = 64;   // sanity ceiling, not a real limit

    // What the deciding hit on the line turned out to be.
    // distance < 0  : no meaningful hit at all (line is clear)
    // isTarget       : the nearest meaningful hit belongs to the target
    // transform null : the nearest meaningful hit could not be named -> stem test
    // otherwise      : named solid blocker at `distance` metres
    struct AF_NamedHit
    {
        void *transform = nullptr;
        float distance  = -1.0f;
        bool  isTarget  = false;
    };

    // Walk a RaycastAll result nearest-hit-first, skipping the local player's
    // own hierarchy and non-target trigger colliders, and stop at the first hit
    // that actually decides the verdict. `selfSkipped` / `triggerSkipped` feed
    // the diagnostics panel so "why was this clear/blocked" stays inspectable.
    inline bool A_FireRaycastAllNearest(void *hitArray, float maxDist,
                                        Pawn *target, Pawn *self,
                                        AF_NamedHit &out,
                                        int *selfSkipped = nullptr,
                                        int *triggerSkipped = nullptr)
    {
        out = AF_NamedHit{};
        if (!Tools::IsPtrValid(hitArray)) return false;

        const size_t len = *(const size_t *)((const unsigned char *)hitArray + AF_ARRAY_LEN_OFF);
        if (len > (size_t)AF_MAX_RAY_HITS) return false;   // header/layout not as expected

        const unsigned char *data = (const unsigned char *)hitArray + AF_ARRAY_DATA_OFF;
        bool taken[AF_MAX_RAY_HITS] = {};
        const float distCeiling = maxDist + A_FireTune::kRayReach + 0.01f;

        for (size_t n = 0; n < len; n++) {
            int best = -1;
            float bestDist = 0.0f;
            for (size_t i = 0; i < len; i++) {
                if (taken[i]) continue;
                const float hd = *(const float *)(data + i * AF_RAYHIT_SIZE + AF_RAYHIT_DISTOFF);
                if (!(hd >= 0.0f) || hd > distCeiling) {
                    // A distance PhysX could not have produced means the array
                    // is not laid out the way this build expects - refuse the
                    // whole result and let the caller fall back.
                    return false;
                }
                if (best < 0 || hd < bestDist) { best = (int)i; bestDist = hd; }
            }
            if (best < 0) break;

            taken[best] = true;
            const unsigned char *elem = data + (size_t)best * AF_RAYHIT_SIZE;
            void *collider = A_FireHitCollider(elem);            // null when unusable
            void *hitT     = A_FireColliderTransform(collider);  // named when readable

            if (hitT != nullptr && target != nullptr && A_FireHitIsTarget(hitT, target)) {
                out.transform = hitT;
                out.distance  = bestDist;
                out.isTarget  = true;
                return true;                                     // the enemy himself
            }
            if (hitT != nullptr && self != nullptr && A_FireHitIsTarget(hitT, self)) {
                if (selfSkipped != nullptr) (*selfSkipped)++;    // OUR body / weapon
                out = AF_NamedHit{};
                continue;
            }
            if (collider != nullptr && fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider)) {
                if (triggerSkipped != nullptr) (*triggerSkipped)++;  // invisible volume
                out = AF_NamedHit{};
                continue;
            }

            // Named solid blocker, or a hit that could not be named at all
            // (transform null): either way this is the deciding record.
            out.transform = hitT;
            out.distance  = bestDist;
            return true;
        }
        out = AF_NamedHit{};   // every hit was ours/an invisible volume: clear
        return true;
    }

    // Is the shot line from `from` to `to` (an aim point INSIDE the target)
    // actually clear to `target`?
    //
    //   probeRadius - the hitbox half width at `to`; the fallback stem test stops
    //                 this far inside it, so the enemy's OWN collider surface is
    //                 never mistaken for cover.
    //   localPawn   - OUR pawn. Its hierarchy (body, head hitbox, weapon, aim
    //                 volume) sits on most rays that start at the camera and is
    //                 skipped, otherwise every LOS test "hits" ourselves.
    //   *identified - set when the nearest meaningful hit was proven to be the
    //                 target (he is the first thing the bullet would reach).
    //   *hitDistance - distance of the deciding hit, for the diagnostics panel.
    inline AF_Ray A_FireRayToTarget(const Vector3 &from, const Vector3 &to, Pawn *target,
                                    float probeRadius, Pawn *localPawn,
                                    bool *identified = nullptr, float *hitDistance = nullptr)
    {
        if (identified != nullptr) *identified = false;
        if (hitDistance != nullptr) *hitDistance = -1.0f;

        Vector3 d(to.x - from.x, to.y - from.y, to.z - from.z);
        const float dist = Vector3::Magnitude(d);
        if (!(dist >= 0.01f)) return AF_Ray::Clear;          // same spot
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);

        // ---- 1. RaycastAll: every hit on the segment, nearest first. -------
        // The real gate: it can STEP OVER hits instead of being forced to
        // accept them (self geometry / trigger volumes at the camera).
        if (fn_PhysicsRaycastAll != nullptr) {
            const float reach = dist + A_FireTune::kRayReach;
            void *arr = fn_PhysicsRaycastAll(from, dir, reach);
            AF_NamedHit h;
            if (A_FireRaycastAllNearest(arr, reach, target, localPawn, h,
                                        &g_triggerDiag.selfHitsSkipped,
                                        &g_triggerDiag.triggerHitsSkipped)) {
                if (h.distance < 0.0f) return AF_Ray::Clear;      // nothing meaningful
                if (hitDistance != nullptr) *hitDistance = h.distance;
                if (h.isTarget) {
                    if (identified != nullptr) *identified = true;
                    return AF_Ray::Clear;                         // the enemy himself
                }
                if (h.transform != nullptr) {
                    if (h.distance <= A_FireTune::kSelfExclusion) {
                        // Blocker hugging the camera: it proves only that we are
                        // clipping geometry, nothing about a target at range.
                        if (A_FireGameSaysVisible(target)) {
                            g_triggerDiag.occOverrides++;
                            return AF_Ray::Clear;
                        }
                    }
                    return AF_Ray::Blocked;                       // wall / vehicle / player
                }
                // Hit could not be named -> stem test below.
            }
            // RaycastAll unusable -> fall through to the single-ray path.
        }

        // ---- 2. single nearest-hit ray (fallback when RaycastAll is unusable)
        if (fn_PhysicsRaycastHitOut != nullptr) {
            unsigned char hitBuf[AF_RAYHIT_SIZE];
            memset(hitBuf, 0, sizeof hitBuf);
            const bool got = fn_PhysicsRaycastHitOut(from, dir, hitBuf, dist + A_FireTune::kRayReach);
            if (!got) return AF_Ray::Clear;                  // nothing at all on the line

            const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
            if (hitDistance != nullptr && hd >= 0.0f && hd <= 100000.0f) *hitDistance = hd;

            void *collider   = A_FireHitCollider(hitBuf);
            void *hitTransform = A_FireColliderTransform(collider);
            if (Tools::IsPtrValid(hitTransform)) {
                if (A_FireHitIsTarget(hitTransform, target)) {
                    if (identified != nullptr) *identified = true;
                    return AF_Ray::Clear;                    // the enemy himself
                }
                if (localPawn != nullptr && A_FireHitIsTarget(hitTransform, localPawn)) {
                    // OUR body/weapon is first on the line: the ray learned
                    // nothing about the target. Hand the verdict to the game.
                    g_triggerDiag.selfHitsSkipped++;
                    if (A_FireGameSaysVisible(target)) {
                        g_triggerDiag.occOverrides++;
                        return AF_Ray::Clear;
                    }
                    return AF_Ray::Blocked;
                }
                if (collider != nullptr && fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider)) {
                    // A single ray cannot step over a trigger volume; an
                    // invisible volume is never an occluder, so the game's own
                    // verdict decides (as above).
                    g_triggerDiag.triggerHitsSkipped++;
                    if (A_FireGameSaysVisible(target)) {
                        g_triggerDiag.occOverrides++;
                        return AF_Ray::Clear;
                    }
                    return AF_Ray::Blocked;
                }
                if (hd >= 0.0f && hd <= A_FireTune::kSelfExclusion &&
                    A_FireGameSaysVisible(target)) {
                    g_triggerDiag.occOverrides++;
                    return AF_Ray::Clear;
                }
                return AF_Ray::Blocked;                      // somebody/something else
            }
            // Hit object could not be named - fall through to the stem test.
        }

        // ---- 3. Conservative stem test ------------------------------------
        // Cast a bounded segment that STOPS SHORT of the target's own collider
        // surface (inside the hitbox radius), so a hit can only be geometry in
        // front of him. Any hit, however thin the object, blocks the shot.
        if (fn_PhysicsRaycastSegment != nullptr) {
            float stem = probeRadius * A_FireTune::kSurfaceFrac;
            if (stem < A_FireTune::kMinStemGap) stem = A_FireTune::kMinStemGap;
            const float stop = dist - stem;
            if (!(stop > 0.01f)) return AF_Ray::Clear;       // point blank: nothing fits between
            return fn_PhysicsRaycastSegment(from, dir, stop) ? AF_Ray::Blocked : AF_Ray::Clear;
        }

        return AF_Ray::Unknown;
    }

    // A straight segment (any two world points) free of geometry? Used by the
    // centre-of-screen path, where the target is not known until the hit is
    // named - so there is nothing to compare against and Blocked is the honest
    // answer for any hit at all.
    inline AF_Ray A_FireRaySegmentClear(const Vector3 &from, const Vector3 &to)
    {
        Vector3 d(to.x - from.x, to.y - from.y, to.z - from.z);
        const float dist = Vector3::Magnitude(d);
        if (!(dist >= 0.01f)) return AF_Ray::Clear;
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);
        if (fn_PhysicsRaycastSegment != nullptr)
            return fn_PhysicsRaycastSegment(from, dir, dist) ? AF_Ray::Blocked : AF_Ray::Clear;
        return AF_Ray::Unknown;
    }

    // Which pawn owns this hit? Walks the hit's parent chain (bounded) and
    // compares every level against every pawn's transform, so ONE ray answers both
    // "what did I hit" and "whose is it". Cheap: <=12 * enemyCount getter calls,
    // and it only ever runs for the centre ray.
    inline Pawn *A_FirePawnFromHitTransform(void *hitTransform, Pawn **items, int count,
                                            int *depthOut = nullptr)
    {
        if (depthOut != nullptr) *depthOut = -1;
        if (!A_FireObjectAlive(hitTransform) || !Tools::IsPtrValid((void *)items)) return nullptr;

        // Cache every pawn's transform ONCE. The old code called
        // Pawn.get_transform() (a native call) for every pawn at EVERY level of
        // the parent walk - up to 12 x enemyCount native calls for one hit.
        constexpr int kMaxCached = 128;
        const int ncache = count < kMaxCached ? count : kMaxCached;
        void *tcache[kMaxCached];
        for (int i = 0; i < ncache; i++) {
            tcache[i] = nullptr;
            Pawn *p = items[i];
            if (A_FireListObjectAlive((void *)p)) tcache[i] = (void *)p->get_transform();
        }

        void *t = hitTransform;
        for (int depth = 0; depth < 12 && A_FireObjectAlive(t); depth++) {
            for (int i = 0; i < count; i++) {
                void *pt;
                if (i < ncache) {
                    pt = tcache[i];
                    if (pt == nullptr) continue;      // dead / torn entry
                } else {
                    // Beyond the cache (>128 enemies): same lookup as before.
                    Pawn *p = items[i];
                    if (!A_FireListObjectAlive((void *)p)) continue;
                    pt = (void *)p->get_transform();
                }
                if (pt == t) {
                    if (depthOut != nullptr) *depthOut = depth;
                    return items[i];
                }
            }
            if (fn_TransformGetParent == nullptr) break;
            void *parent = fn_TransformGetParent(t);
            if (!Tools::IsPtrValid(parent) || parent == t) break;
            t = parent;
        }
        return nullptr;
    }

    // ---- PATH A: the enemy under the exact centre of the screen -----------
    // One ray from the camera along the camera's own forward
    // (Transform.get_forward 0x509C650). Whatever it hits FIRST that is not
    // our own body and not an invisible trigger volume is what the crosshair is
    // really on, so:
    //   - the hit names an enemy pawn -> that pawn is centred AND unoccluded. A
    //     wall in front of him would have been hit first, which is what makes
    //     this path incapable of shooting through anything.
    //   - anything else               -> nothing is centred.
    // Skipping our own geometry is mandatory here too: the ray starts inside
    // the local pawn's collider stack, and without the skip the hit is always
    // ourselves - which is exactly why the broken build printed
    // "Centre=none" while the crosshair was on an enemy.
    // It needs no bones and no projection math, so it keeps working even if the
    // Pawn bone offsets drift between game builds - the failure mode that turns
    // path B into "sometimes it does not shoot" with no visible reason.
    inline Pawn *A_FireCrosshairRayPawn(const Vector3 &camPos, const Vector3 &camForward,
                                        Pawn **items, int count, Pawn *localPawn,
                                        float *hitDistOut, bool *symbolMissing)
    {
        if (hitDistOut != nullptr) *hitDistOut = -1.0f;
        if (symbolMissing != nullptr) *symbolMissing = false;
        const float len = Vector3::Magnitude(camForward);
        if (!(len > 0.0001f)) return nullptr;
        const Vector3 dir(camForward.x / len, camForward.y / len, camForward.z / len);

        // Preferred: RaycastAll, walking past our own geometry and triggers.
        if (fn_PhysicsRaycastAll != nullptr) {
            void *arr = fn_PhysicsRaycastAll(camPos, dir, A_FireTune::kCrosshairRayMax);
            AF_NamedHit h;
            if (A_FireRaycastAllNearest(arr, A_FireTune::kCrosshairRayMax,
                                        nullptr, localPawn, h,
                                        &g_triggerDiag.selfHitsSkipped,
                                        &g_triggerDiag.triggerHitsSkipped)) {
                if (h.distance < 0.0f) return nullptr;   // the crosshair is on the sky
                if (hitDistOut != nullptr) *hitDistOut = h.distance;
                if (h.transform == nullptr) return nullptr;  // cannot name the hit
                return A_FirePawnFromHitTransform(h.transform, items, count);
            }
            // RaycastAll unusable -> single-ray path below.
        }

        if (fn_PhysicsRaycastHitOut == nullptr) {
            if (symbolMissing != nullptr) *symbolMissing = true;
            return nullptr;
        }

        unsigned char hitBuf[AF_RAYHIT_SIZE];
        memset(hitBuf, 0, sizeof hitBuf);
        if (!fn_PhysicsRaycastHitOut(camPos, dir, hitBuf, A_FireTune::kCrosshairRayMax))
            return nullptr;                                  // the crosshair is on the sky

        const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
        if (hitDistOut != nullptr && hd >= 0.0f) *hitDistOut = hd;

        void *hitTransform = A_FireColliderTransform(A_FireHitCollider(hitBuf));
        if (A_FireObjectAlive(hitTransform) && localPawn != nullptr &&
            A_FireHitIsTarget(hitTransform, localPawn)) {
            g_triggerDiag.selfHitsSkipped++;   // first thing on the line is US
            return nullptr;
        }
        return A_FirePawnFromHitTransform(hitTransform, items, count);
    }

    // =====================================================================
    //  THE SCAN (target acquisition + shot validation)
    //
    //  Target validity never comes from a hitbox pick or a raw bone field: the
    //  enemy's own head getter and root transform give an on-screen BODY, the
    //  crosshair has to be on it, and the shot then has to survive the ray that
    //  NAMES him. That is what makes the trigger fire at exactly what the player
    //  can see, at any range and on any ping.
    //
    //  Two paths, in this order:
    //    A. the camera's own forward ray - "enemy exactly in the centre of the
    //       screen". Bone-free, and unoccluded by construction.
    //    B. the crosshair zone - any enemy whose hitbox projects inside the
    //       configured px radius, with the nearest-hit ray naming the first thing
    //       on the shot line.
    //  Returns FireBlock::Fired with `out` filled, or the one reason the shot was
    //  refused. Candidates are checked nearest-to-crosshair first, so the pawn
    //  under the crosshair is the one that gets the ray budget.
    // =====================================================================
    // ---- the user's target filter ------------------------------------------
    // Checked only AFTER a pawn has survived every hard gate (alive, not us, not
    // a teammate, under the crosshair), so it can only ever NARROW the candidate
    // list: it can never turn a blocked shot into a fired one. Every rule is a
    // local read or a symbol the gate already uses - no new game method is
    // involved, which is what makes the filter safe to add to a working feature.
    //
    //   All         - every valid enemy (the behaviour before the filter existed)
    //   VisibleOnly - only pawns the GAME says it drew / did not occlude last
    //                 frame (Pawn.get_LastOccVisible / get_RecentlyRendered, the
    //                 same verdicts the near-origin escape uses)
    //   Within*     - only pawns whose aim point is inside that range
    enum class TriggerFilter : int
    {
        All         = 0,
        VisibleOnly = 1,
        Within25m   = 2,
        Within50m   = 3,
        Within100m  = 4
    };

    // worldDist < 0 means "not known yet" - a rule that needs the distance lets
    // the pawn through and is re-asked once the aim point exists.
    inline bool A_FireFilterPass(int filter, void *pawn, float worldDist)
    {
        switch ((TriggerFilter)filter) {
            case TriggerFilter::VisibleOnly:
                return A_FireGameSaysVisible((Pawn *)pawn);
            case TriggerFilter::Within25m:
                return worldDist < 0.0f || worldDist <= 25.0f;
            case TriggerFilter::Within50m:
                return worldDist < 0.0f || worldDist <= 50.0f;
            case TriggerFilter::Within100m:
                return worldDist < 0.0f || worldDist <= 100.0f;
            case TriggerFilter::All:
            default:
                return true;
        }
    }

    inline FireBlock A_FireScan(Pawn *localPawn, Weapon *weapon, TriggerTarget &out)
    {
        TriggerbotDiag &d = g_triggerDiag;
        out = TriggerTarget{};
        g_afireLosOk = false;
        d.losBlockedScan = 0;
        d.enemyCount = 0;
        d.closestPx = -1.0f;
        d.candidates = 0;
        d.losChecked = 0;
        d.friendsInLine = 0;
        d.filteredOut = 0;
        d.selfHitsSkipped = 0;
        d.triggerHitsSkipped = 0;
        d.occOverrides = 0;
        d.lastAimPx = -1.0f;
        d.lastPredPx = -1.0f;
        d.lastHitboxPx = 0.0f;
        d.lastSpreadPx = 0.0f;
        d.lastTravel = 0.0f;
        d.lastWorldDist = 0.0f;
        d.lastBulletSpeed = 0.0f;

        BaseGame *game = (BaseGame *)fn_GameGetMatchGame();
        if (!Tools::IsPtrValid((void *)game)) return FireBlock::NoMatch;

        Camera *cam = Camera::get_main();
        if (!A_FireObjectAlive((void *)cam)) return FireBlock::NoMatch;
        Transform *camTransform = cam->get_transform();
        if (!A_FireObjectAlive((void *)camTransform)) return FireBlock::NoMatch;
        const Vector3 camPos = camTransform->get_position();

        // One liveness check for the camera per SCAN. The old path went through
        // A_FireWorldToScreenSafe for every projection (3 per enemy) and each of
        // those re-validated the same camera object with two probes - the camera
        // was already validated two lines above.
        auto w2s = [&](const Vector3 &world, Vector3 &sc) -> bool {
            if (fn_WorldToScreen == nullptr) return false;
            sc = fn_WorldToScreen(cam, world);
            return true;
        };

        // The game's OWN aim answers (Weapon.get_AimingTarget 0x510EAAC,
        // Weapon.get_CurrentAimingTargetDis 0x510EABC - dump 739828 / 739834).
        // Read-only diagnostics + a tie-break: the pawn the game itself has
        // locked is preferred when two candidates sit at the same pixel.
        void *aimTarget = nullptr;
        if (fn_WeaponGetAimingTarget != nullptr && Tools::IsPtrValid(weapon)) {
            aimTarget = fn_WeaponGetAimingTarget(weapon);
            d.aimingTarget = (unsigned)(uintptr_t)aimTarget;
            d.aimingTargetDis = (fn_WeaponGetAimingTargetDis != nullptr && aimTarget != nullptr)
                              ? fn_WeaponGetAimingTargetDis(weapon) : 0.0f;
        } else {
            d.aimingTarget = 0;
            d.aimingTargetDis = 0.0f;
        }

        List<Pawn *> *enemyPawns = game->EnemyPawns();
        if (!Tools::IsPtrValid((void *)enemyPawns)) return FireBlock::NoEnemyInZone;
        Pawn **items = (Pawn **)enemyPawns->getItems();
        const int count = enemyPawns->getSize();
        d.enemyCount = count;
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024)
            return FireBlock::NoEnemyInZone;

        const float cx = (float)get_width()  * 0.5f;
        const float cy = (float)get_height() * 0.5f;
        const float radius = A_FireCrosshairRadiusPx();
        const float radiusSq = radius * radius;
        d.radiusPx = radius;

        // Read once per scan, not per candidate.
        const int targetFilter = Config.NewFeaturesMenu.TriggerbotFilter;
        const float focalPx = A_FireFocalPx();
        d.focalPx = focalPx;

        // ---- 0. PATH A - the enemy under the exact centre of the screen ----
        // Tried before the zone scan because it answers the user's "shoots when the
        // enemy is exactly in the middle" requirement directly, without relying on
        // m_HeadBone/m_Mesh offsets or on a projection radius.
        if (fn_TransformGetForward != nullptr) {
            const Vector3 camForward = fn_TransformGetForward(camTransform);
            bool symbolMissing = false;
            float centreHitDist = -1.0f;
            Pawn *centred = A_FireCrosshairRayPawn(camPos, camForward, items, count, localPawn,
                                                   &centreHitDist, &symbolMissing);
            d.crosshairHitDist = centreHitDist;
            d.crosshairPawn = (unsigned)(uintptr_t)centred;
            if (symbolMissing && fn_PhysicsRaycastSegment == nullptr) {
                d.rayUnknown = true;                        // no way at all to trace
            } else if (centred != nullptr && centred != localPawn &&
                       !IsSameTeamAcceptBots(localPawn, centred) && A_FirePawnAlive(centred) &&
                       A_FireFilterPass(targetFilter, centred, centreHitDist)) {
                // A centre-ray pawn the filter refuses simply falls through to
                // the zone scan below, which applies the same filter and does the
                // counting.
                // The shot does not depend on the probe - the centre ray already
                // proved visibility AND ownership - so this is only the aim point
                // handed to the spread/lead report.
                const A_FireAimPoint ap = A_FireAimPointFor(centred);
                float bestSq = -1.0f;
                if (ap.valid) {
                    Vector3 sc = Vector3::zero();
                    if (w2s(ap.pos, sc) && sc.z > 0.0f) {
                        const float dx = sc.x - cx;
                        const float dy = sc.y - cy;
                        bestSq = dx * dx + dy * dy;
                    }
                }
                out.pawn      = centred;
                out.aim       = ap;
                out.aimPoint  = ap.valid ? ap.pos : camPos;
                out.screenPos = Vector3::zero();
                if (ap.valid) w2s(ap.pos, out.screenPos);
                out.pxOffset  = bestSq >= 0.0f ? sqrtf(bestSq) : -1.0f;
                out.worldDist = ap.valid ? Vector3::Distance(camPos, ap.pos) : centreHitDist;
                out.path      = 0;
                out.visible   = true;
                out.visibilityIdentified = true;            // the ray NAMED him
                d.lastAimPx       = out.pxOffset;
                d.lastWorldDist   = out.worldDist;
                d.aimPath         = 0;
                d.raysIdentified++;
                g_afireLosOk      = true;
                d.targets++;
                return FireBlock::Fired;
            }
        }

        // Team gate hoisted out of the loop: get_TeamId(localPawn) is the same
        // for the whole scan, while IsSameTeamAcceptBots re-validated the LOCAL
        // pawn (2 liveness probes) and re-read its team id for every enemy of
        // every scan.
        static auto getTeamIdFn = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);
        const uint32_t localTeam = (A_FireObjectAlive((void *)localPawn) && getTeamIdFn != nullptr)
                                 ? getTeamIdFn(localPawn) : 0u;
        auto sameTeamGate = [&](Pawn *p) -> bool {
            if (!A_FireObjectAlive((void *)p)) return false;   // destroyed -> not a team answer
            if (p == localPawn) return true;
            if (localTeam == 0u || getTeamIdFn == nullptr) return false;
            return getTeamIdFn(p) == localTeam;                // team id is the gate
        };

        // ---- 1. every valid enemy whose hitbox projects into the zone ----
        constexpr int MAX_CANDIDATES = 12;
        A_FireCandidate cands[MAX_CANDIDATES];
        int n = 0;

        for (int i = 0; i < count; i++) {
            Pawn *p = items[i];
            // Dead objects stay listed for a few frames and still pass a plain
            // readability check (the managed wrapper is alive); every native
            // call below would walk their cleared m_CachedPtr. A pawn the list
            // is being rewritten this frame hands out an unaligned reference.
            if (!A_FireListObjectAlive((void *)p)) continue;
            if (p == localPawn) continue;                        // never shoot yourself
            if (sameTeamGate(p)) continue;                        // team id is the gate
            if (!A_FirePawnAlive(p)) continue;                    // dead / downed / still ragdolling
            // The user's filter, first pass: a rule that needs no distance
            // (VisibleOnly) drops this pawn before its three projections.
            if (!A_FireFilterPass(targetFilter, p, -1.0f)) {
                d.filteredOut++;
                continue;
            }

            const A_FireAimPoint ap = A_FireAimPointFor(p);
            if (!ap.valid) continue;                              // the game answered nothing about his body

            // Where is his BODY on screen? Project head and feet and take the
            // crosshair's distance to the segment between them; fall back to the
            // chest point when the body does not project. This one measurement
            // replaces the whole hitbox-priority walk: no bone field offset, no
            // user pick, and a body that fills the screen at 2 m is handled exactly
            // like one a few px tall at 150 m.
            float d2 = -1.0f;
            Vector3 scChest = Vector3::zero();
            if (w2s(ap.pos, scChest) && scChest.z > 0.0f) {
                const float dx = scChest.x - cx;
                const float dy = scChest.y - cy;
                d2 = dx * dx + dy * dy;
            }
            // The chest already answers it (and is the case that fires): do not
            // pay two more WorldToScreenPoint calls for the body span.
            if (d2 >= 0.0f && d2 > radiusSq && ap.headOk && ap.feetOk) {
                Vector3 scHead = Vector3::zero();
                Vector3 scFeet = Vector3::zero();
                if (w2s(ap.head, scHead) && w2s(ap.feet, scFeet) &&
                    scHead.z > 0.0f && scFeet.z > 0.0f) {
                    const float body = A_FirePointSegDistSq(cx, cy,
                                                            scHead.x, scHead.y,
                                                            scFeet.x, scFeet.y);
                    if (d2 < 0.0f || body < d2) d2 = body;
                }
            }
            if (d2 < 0.0f) continue;                              // his body is not on screen
            if (d.closestPx < 0.0f || d2 < d.closestPx) d.closestPx = d2;   // "how close did anything get"
            if (d2 > radiusSq) continue;                          // not under the crosshair
            const float pxOffset = sqrtf(d2);
            if (n >= MAX_CANDIDATES) continue;

            // Second pass: the rules that only apply once the distance to the
            // aim point is known.
            const float candDist = Vector3::Distance(camPos, ap.pos);
            if (!A_FireFilterPass(targetFilter, p, candDist)) {
                d.filteredOut++;
                continue;
            }

            cands[n].pawn      = p;
            cands[n].aim       = ap;
            cands[n].pxOffset  = pxOffset;
            cands[n].worldDist = candDist;
            cands[n].velocity  = GetPawnVelocity(p);
            n++;
        }
        d.candidates = n;
        if (n <= 0) return FireBlock::NoEnemyInZone;

        // Teammate positions are identical for every candidate of one scan -
        // collect them (2 native reads per mate) ONCE instead of re-walking the
        // whole team list for every candidate (A_FireFriendlyInLine below).
        AF_FriendProbe friendProbes[64];
        const int friendCount = A_FireCollectFriendProbes(localPawn, friendProbes, 64);

        // ---- 2. nearest to the crosshair first, the game's own aim target
        //         winning a near-tie (0.5 px) -------------------------------
        int order[MAX_CANDIDATES];
        for (int i = 0; i < n; i++) order[i] = i;
        for (int i = 1; i < n; i++) {
            const int oi = order[i];
            float keyI = cands[oi].pxOffset;
            if (aimTarget != nullptr && (void *)cands[oi].pawn == aimTarget) keyI -= 0.5f;
            int j = i - 1;
            while (j >= 0) {
                const int oj = order[j];
                float keyJ = cands[oj].pxOffset;
                if (aimTarget != nullptr && (void *)cands[oj].pawn == aimTarget) keyJ -= 0.5f;
                if (keyJ <= keyI) break;
                order[j + 1] = order[j];
                j--;
            }
            order[j + 1] = oi;
        }

        // Lead and spread are always MEASURED (they feed the diagnostics panel)
        // and neither can veto a shot - that decision is fixed; the option that
        // used to sit here changed nothing.
        constexpr bool checkSpread = true;
        constexpr bool wantLead    = true;
        const float spreadPx   = A_FireSpreadPx(weapon, focalPx, checkSpread);
        d.lastSpreadPx = spreadPx;

        // Only geometry and teammates can stop a shot now. Everything else the tree
        // can compute (ready state, fire interval, travel time, spread, lead) is
        // reported through the diagnostics and never vetoes.
        bool blockedLos = false, blockedFriend = false;

        // Lead inputs are per-WEAPON, not per-candidate: resolve once per scan
        // instead of walking the fire component for every candidate.
        bool speedKnown = false;
        const float bulletSpeed = GetWeaponBulletSpeed(weapon, &speedKnown);

        // ---- 3. validate each candidate, closest to the crosshair first ----
        for (int oi = 0; oi < n; oi++) {
            const A_FireCandidate &c = cands[order[oi]];

            // (a) LINE OF SIGHT - the NEAREST thing on the camera -> aim point line
            //     has to BE this pawn. A wall, however thin, is not this pawn, so it
            //     blocks - while an arm, a backpack or the weapon itself reaching
            //     toward the muzzle does not, because that is his own hierarchy.
            d.losChecked++;
            bool losIdentified = false;
            float losHitDist = -1.0f;
            AF_Ray los = A_FireRayToTarget(camPos, c.aim.pos, c.pawn, c.aim.radius,
                                           localPawn, &losIdentified, &losHitDist);

            // The chest can be behind cover while the HEAD is exposed - the classic
            // "peeking over the wall" shot, and one of the "sometimes it does not
            // shoot" cases. When the chest line is blocked, retry at the game's own
            // head position before giving up on this pawn. The shot only ever goes
            // out on a ray that came back Clear, and the probe below is the one that
            // was actually validated.
            Vector3 probePoint  = c.aim.pos;
            float   probeRadius = c.aim.radius;
            if (los != AF_Ray::Clear && c.aim.headOk) {
                bool headIdentified = false;
                float headHitDist = -1.0f;
                const AF_Ray losHead = A_FireRayToTarget(camPos, c.aim.head, c.pawn,
                                                         A_FireTune::kHeadRadius,
                                                         localPawn, &headIdentified, &headHitDist);
                if (losHead == AF_Ray::Clear) {
                    los           = losHead;
                    losIdentified = headIdentified;
                    losHitDist    = headHitDist;
                    probePoint    = c.aim.head;
                    probeRadius   = A_FireTune::kHeadRadius;
                }
            }
            const float probeDist = Vector3::Distance(camPos, probePoint);
            d.lastLosHitDist = losHitDist;
            if (losIdentified) d.raysIdentified++; else d.raysBlocked++;
            if (los == AF_Ray::Unknown) d.rayUnknown = true;   // no ray symbol -> fail closed
            if (los != AF_Ray::Clear) {
                d.losBlocked++;
                d.losBlockedScan++;
                blockedLos = true;
                continue;
            }

            // (b) RANGELESS + LEAD. No maximum range is applied (that is the
            //     point of a rangeless trigger), but the bullet still has to
            //     arrive while the target is still there.
            float travel = 0.0f;
            Vector3 predicted = probePoint;
            bool lead = false;

            if (wantLead && speedKnown && probeDist > 1.0f) {
                travel = probeDist / bulletSpeed;
                d.lastTravel = travel;
                if (travel > 0.002f && travel <= A_FireTune::kMaxTravelTime) {
                    predicted = probePoint + c.velocity * travel;
                    lead = true;
                }
                // ADVISORY ONLY. A long flight time (kMaxTravelTime) and a covered
                // lead path both used to `continue` and drop the shot - that was the
                // long-range half of "sometimes it does not shoot". The lead is a
                // prediction, so when it happens to be covered the shot is taken at
                // the aim point instead of being refused.
                if (lead && A_FireRayToTarget(camPos, predicted, c.pawn, probeRadius,
                                              localPawn) != AF_Ray::Clear) {
                    d.leadPathBlocked = true;
                    lead = false;
                    predicted = probePoint;
                }
            }

            // (c) SPREAD / RECOIL CONTAINMENT - is the crosshair still on the
            //     hitbox where the target will be when the bullet arrives?
            float predPx = c.pxOffset;
            if (lead) {
                Vector3 scPred = Vector3::zero();
                if (w2s(predicted, scPred) && scPred.z > 0.0f) {
                    const float pdx = scPred.x - cx;
                    const float pdy = scPred.y - cy;
                    predPx = sqrtf(pdx * pdx + pdy * pdy);
                } else {
                    predicted = probePoint;         // prediction left the screen
                    lead = false;
                }
            }
            const float hitboxPx = (focalPx > 0.0f && probeDist > 0.01f)
                                 ? (probeRadius * focalPx / probeDist) : 0.0f;
            // ADVISORY ONLY - this used to `continue`. At range a hitbox is only a
            // few px wide while the cone is wide, so the old test vetoed shots the
            // user had the crosshair on: the other half of "sometimes it does not
            // shoot". The numbers are still measured and shown in the panel.
            if (!A_FireShotWouldLand(predPx, radius, hitboxPx, spreadPx))
                d.spreadWarn = true;

            // (d) FRIENDLY IN THE LINE OF FIRE - pure geometry, catches the
            //     teammate the ray misses but the spread cone would hit.
            Vector3 dirToAim(probePoint.x - camPos.x, probePoint.y - camPos.y, probePoint.z - camPos.z);
            const float dirLen = Vector3::Magnitude(dirToAim);
            if (dirLen > 0.01f) {
                dirToAim = Vector3(dirToAim.x / dirLen, dirToAim.y / dirLen, dirToAim.z / dirLen);
                int friends = 0;
                if (A_FireFriendlyInLine(friendProbes, friendCount, camPos, dirToAim, probeDist, &friends)) {
                    d.friendsInLine += friends;
                    blockedFriend = true;
                    continue;
                }
            }

            // ---- SURVIVES EVERYTHING -> this is the shot ----
            out.pawn       = c.pawn;
            out.aim        = c.aim;
            out.aimPoint   = probePoint;
            out.screenPos = Vector3::zero();
            w2s(probePoint, out.screenPos);
            out.pxOffset   = c.pxOffset;
            out.worldDist  = probeDist;
            out.travelTime = travel;
            out.predicted  = predicted;
            out.predPxOffset = predPx;
            out.hitboxPx   = hitboxPx;
            out.spreadPx   = spreadPx;
            out.focalPx    = focalPx;
            out.bulletSpeed = bulletSpeed;
            out.speedKnown = speedKnown;
            out.leadUsed   = lead;
            out.path       = 1;                       // the crosshair-zone scan
            out.visible    = true;                    // proven by the ray above
            out.visibilityIdentified = losIdentified; // the ray NAMED this pawn

            d.lastAimPx  = c.pxOffset;
            d.lastPredPx = predPx;
            d.lastHitboxPx = hitboxPx;
            d.lastTravel = travel;
            d.lastWorldDist = probeDist;
            d.lastBulletSpeed = bulletSpeed;
            g_afireLosOk = true;
            d.targets++;
            return FireBlock::Fired;
        }

        // Nothing survived. Report the reason that is actually worth acting on:
        // a teammate in the line first (dangerous), then the geometry reasons.
        if (blockedFriend)   return FireBlock::FriendlyInLine;
        if (blockedLos)      return FireBlock::NoLineOfSight;
        return FireBlock::NoEnemyInZone;
    }

    // =====================================================================
    //  SHOULD FIRE - the ordered tree. Everything cheap and local is answered
    //  before a single ray is cast; only a candidate that is genuinely under
    //  the crosshair ever costs a physics query.
    // =====================================================================
    inline FireBlock A_FireShouldFire(TriggerTarget &out)
    {
        TriggerbotDiag &d = g_triggerDiag;
        out = TriggerTarget{};

        // ---- 1. local player --------------------------------------------
        Pawn *localPawn = GamePlay::get_LocalPawn();
        d.localPawn = (void *)localPawn;
        if (!A_FireObjectAlive((void *)localPawn)) return FireBlock::NoLocalPawn;
        if (!A_FirePawnAlive(localPawn)) return FireBlock::LocalNotAlive;

        // ---- 2. match / round state -------------------------------------
        if (fn_GameGetMatchGame == nullptr) return FireBlock::NoMatch;
        void *game = fn_GameGetMatchGame();
        d.matchGame = game;
        if (!Tools::IsPtrValid(game)) return FireBlock::NoMatch;
        if (fn_BaseGameIsPaused != nullptr && fn_BaseGameIsPaused(game))
            return FireBlock::MatchPaused;

        // ---- 3. held weapon readiness ------------------------------------
        Weapon *held = localPawn->get_CurrentWeapon();
        d.heldWeapon = (void *)held;
        if (!A_FireObjectAlive((void *)held)) return FireBlock::NoWeapon;

        // ADVISORY: a weapon that is mid-switch, mid-reload or between shots does
        // not need to stop the trigger - Weapon.StartFire itself refuses an illegal
        // shot, so being permissive here costs nothing and removes three more ways
        // for the trigger to silently do nothing. All three are still measured and
        // printed, and "Require Ready" turns Weapon.CanFire() back into a hard gate
        // for anyone who wants it.
        d.switching = A_FireWeaponSwitching(held);
        d.ammoOk    = A_FireWeaponHasAmmo(held);
        d.canFire   = A_FireWeaponCanFire(held);
        // Weapon.CanFire() is measured for the panel but never gates: StartFire
        // itself refuses an illegal shot, and hard-gating on it here is what made
        // the trigger go quiet mid-fight with no visible reason.

        // ---- 4. trigger key gating (the mode combo) ----------------------
        // 0 = Always  : the trigger fires on its own the moment a target is under
        //               the crosshair (0-delay, no reaction delay).
        // 1 = ADS only: only while the player is scoping (Pawn.IsAiming 0x519BB10).
        // 2 = Firing only: only while the player's own fire input is active
        //               (Pawn.get_IsFiring 0x51A8A38). Note the trigger's own
        //               press also raises that flag, which is what keeps a burst
        //               going while the crosshair stays on the target - the same
        //               semantics the reference implementation had.
        if (Config.NewFeaturesMenu.TriggerbotMode == 1) {
            if (fn_PawnIsAiming == nullptr || !fn_PawnIsAiming(localPawn))
                return FireBlock::ModeNotAds;
        } else if (Config.NewFeaturesMenu.TriggerbotMode == 2) {
            if (fn_PawnGetIsFiring == nullptr || !fn_PawnGetIsFiring(localPawn))
                return FireBlock::ModeNotFiring;
        }

        // ---- 5. targets + line of sight + lead + spread + friendlies -----
        return A_FireScan(localPawn, held, out);
    }

    // Backward compatibility - returns the pawn pointer only. (The menu's
    // diagnostics call the tree above; nothing else uses this.)
    inline uintptr_t A_FireFindTarget()
    {
        TriggerTarget target{};
        if (A_FireShouldFire(target) != FireBlock::Fired) return 0;
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
    // `tickInstance` is the object the hook was called on: the Weapon::Tick hook
    // passes the weapon, the pawn-tick hooks pass nullptr. A scan only runs while
    // the local player's *held weapon* (or the local player's own pawn) ticked,
    // exactly like the reference implementation.
    //
    // The fire path itself is a straight read of the decision tree:
    //   A_FireShouldFire() -> Fired?  -> press / hold / release
    //                       -> blocked -> release (never leave the trigger down)
    // Every scan reports its reason through A_FireReportBlock(), so the menu can
    // show the live gate that is stopping the shot.
    inline void A_FireTick(void *tickInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        const double now = A_FireNow();
        d.lastTickTime = now;               // hook liveness (the menu's heartbeat row)

        // FAST OUT while the feature is off. A session with the triggerbot
        // switched off used to pay a full native resolve (local pawn + held
        // weapon) for every pawn and every weapon tick in the world - hundreds of
        // calls per frame - only to reach the FeatureOff branch below. Nothing
        // below can change while it is off, so the hooks return here; the reason
        // is still refreshed at 10 Hz so the panel keeps showing a live verdict.
        if (!Config.NewFeaturesMenu.Triggerbot &&
            !g_afireHoldingFire && !g_testFireRequest) {
            if (now - g_afireOffDiagStamp < 0.1) return;
            g_afireOffDiagStamp = now;
        }

        Pawn *localPawn = A_FireLocalPawnFast();
        d.localPawn = (void *)localPawn;

        // INSTANCE FILTER. When the hook passes an instance (Weapon::Tick) only
        // the local player's HELD weapon may spend an evaluation: the game ticks
        // every weapon in the world through that address, and each AI tick used
        // to consume the slot below before the instance was even looked at -
        // which threw away the local player's own tick that frame. The held
        // weapon cannot change inside 50 ms, so every other weapon that ticks in
        // that window is rejected from the cache, without touching the game.
        if (tickInstance != nullptr) {
            if (now - g_afireHeldWeaponStamp < 0.05 &&
                tickInstance != g_afireHeldWeaponCache)
                return;                                     // cached: not our weapon
            Weapon *held = A_FireObjectAlive((void *)localPawn) ? localPawn->get_CurrentWeapon() : nullptr;
            if (!A_FireObjectAlive((void *)held) || tickInstance != (void *)held)
                return;
            g_afireHeldWeaponCache = (void *)held;
            g_afireHeldWeaponStamp = now;
            d.weaponHeldTicks++;
        }
        // ONE scan per frame, not one per heartbeat. Three hooks reach this
        // point for the local player inside the SAME frame - base Pawn::Tick,
        // the concrete pawn override (which usually chains into the base) and
        // Weapon::Tick - and they sit milliseconds apart, so the old 1 ms gate
        // let one frame scan two or three times. Every scan is a full enemy
        // walk + physics rays + team/friendly reads, so the duplicates were
        // pure wasted work. 4 ms admits the first tick of every frame at
        // 60/120 Hz and rejects the rest; worst-case added reaction is a
        // quarter of a frame.
        if (now - g_afireLastEvalTime < 0.004) return;
        g_afireLastEvalTime = now;
        if (g_afireInTick) return;                       // re-entrancy guard
        g_afireInTick = true;

        // Queued Test Fire is honoured even while the toggle is off.
        if (g_testFireRequest) A_FireRunTestFire();

        d.hookCalls++;
        d.scans++;
        // Reset the per-scan visibility bookkeeping before the tree runs.
        d.aimPath = 1;
        d.crosshairPawn = 0;
        d.crosshairHitDist = -1.0f;
        d.raysIdentified = 0;
        d.raysBlocked = 0;
        d.rayUnknown = false;
        d.lastLosHitDist = -1.0f;
        d.leadPathBlocked = false;
        d.spreadWarn = false;

        if (!Config.NewFeaturesMenu.Triggerbot) {
            A_FireReleaseTrigger(nullptr);
            A_FireReportBlock(FireBlock::FeatureOff);
            s_dbgTriggerState = 0;
            g_afireInTick = false;
            return;
        }

        TriggerTarget target{};
        const FireBlock block = A_FireShouldFire(target);
        A_FireReportBlock(block);

        if (block != FireBlock::Fired || !Tools::IsPtrValid((void *)target.pawn)) {
            A_FireReleaseTrigger(nullptr);
            s_dbgTriggerState = 0;
            g_afireInTick = false;
            return;
        }

        // A shootable target is under the crosshair right now.
        s_dbgTriggerState = 1;

        Weapon *held = (Weapon *)d.heldWeapon;
        if (!A_FireObjectAlive((void *)held)) {
            A_FireReleaseTrigger(nullptr);
            g_afireInTick = false;
            return;
        }

        // Weapon swapped while the trigger was down - let the old one go.
        if (g_afireHoldingFire && g_afireFiringOn != (void *)held)
            A_FireReleaseTrigger(nullptr);

        const FireMode mode = GetWeaponFireMode(held);

        // Already holding the trigger on this weapon and this target: keep
        // holding. The game's own fire interval (and Weapon.CanFire above) is
        // what limits the rate, so there is nothing to re-press.
        if (g_afireHoldingFire && g_afireFiringOn == (void *)held &&
            g_afireTargetPawn == (void *)target.pawn) {
            if (mode == FireMode::Single) {
                // Semi-auto: one press is one shot - release now so the next
                // scan can press again once the weapon's interval has elapsed.
                A_FireReleaseTrigger(nullptr);
            }
            s_dbgTriggerState = 2;
            g_afireInTick = false;
            return;
        }

        // NO RE-PRESS PACING. The old gate held the trigger off until 85 % of the
        // weapon's own interval had elapsed, which is literally an artificial delay
        // between shots - exactly what this feature is supposed to remove. StartFire
        // is called the moment a visible enemy is under the crosshair and the game's
        // own fire interval decides the rate. The interval is still measured for the
        // diagnostics panel.
        d.lastFireInterval = A_FireWeaponInterval(held);

        if (fn_Weapon_StartFire == nullptr) {
            A_FireReleaseTrigger(nullptr);
            A_FireReportBlock(FireBlock::WeaponNotReady);
            g_afireInTick = false;
            return;
        }

        g_afireHoldingFire    = true;
        g_afireFiringOn       = (void *)held;
        g_afireTargetPawn     = (void *)target.pawn;
        g_afireLastPressTime  = now;
        fn_Weapon_StartFire(held);
        d.fires++;
        s_dbgTriggerState = 2;                           // firing

        LOGI("[NewFeatures] triggerbot: StartFire(held=%p) target=%p path=%s aim=%.1fpx "
             "hit=%.3fm ident=%d dist=%.3f travel=%.4f",
             (void *)held, (void *)target.pawn,
             target.path == 0 ? "centre-ray" : "zone",
             target.pxOffset,
             d.lastLosHitDist, (int)target.visibilityIdentified,
             target.worldDist, target.travelTime);

        g_afireInTick = false;
    }

    // MemoryPatches for spawn-time safety: reset held-weapon reference on the
    // triggerbot side when a stale weapon pointer is being held across matches.
    inline void A_FireResetForNewMatch()
    {
        g_afireHoldingFire = false;
        g_afireTargetPawn  = nullptr;
        g_afireFiringOn    = nullptr;
        g_afireLastPressTime = -1000.0;
        g_afireLastEvalTime  = -1000.0;
        g_afireInTick        = false;
        g_afireLosOk         = false;
        // A new match means new pawn/weapon objects: drop the cached identities
        // instead of waiting out their 50 ms window.
        g_afireLocalPawnCache     = nullptr;
        g_afireLocalPawnCacheTime = -1000.0;
        g_afireHeldWeaponCache    = nullptr;
        g_afireHeldWeaponStamp    = -1000.0;
    }

    // ---- Heartbeats -------------------------------------------------------
    // Weapon::Tick / Pawn::Tick are BASE implementations; il2cpp virtual
    // dispatch invokes each subclass's Slot-33 override instead, so neither
    // base ever runs for the local player's gun (device proof: wTick=0,pawn=0
    // while everything else resolved). The real drivers are the per-class pawn
    // tick overrides below - each passes the pawn instance.
    inline void A_FirePawnTicked(void *pawnInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        d.pawnTicks++;
        // Cached local pawn: this hook runs once per pawn in the world, and each
        // of those used to cost a native GamePlay::get_LocalPawn() call.
        if (A_FireLocalPawnFast() == (Pawn *)pawnInstance) {
            d.localPawn = pawnInstance;
            d.pawnLocalTicks++;
            A_FireTick(nullptr);        // the local player's own pawn ticked
        }
    }

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
        A_FirePawnTicked(instance);
    }

    inline void Hook_PlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_PlayerPawn_Tick != nullptr)
            orig_PlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_MPPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_MPPawn_Tick != nullptr)
            orig_MPPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_BRPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_BRPlayerPawn_Tick != nullptr)
            orig_BRPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_DMZPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_DMZPlayerPawn_Tick != nullptr)
            orig_DMZPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_PVEPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_PVEPlayerPawn_Tick != nullptr)
            orig_PVEPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_OpenWorldPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_OpenWorldPlayerPawn_Tick != nullptr)
            orig_OpenWorldPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_BRGroundWarPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_BRGroundWarPlayerPawn_Tick != nullptr)
            orig_BRGroundWarPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
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

        // Nearest-hit ray WITH an out RaycastHit + explicit maxDistance
        // (0x506C188). This is the anti-wall gate: the ray can now be read as
        // "what is the first thing on this line", not as a bare distance.
        auto rayHitOutAddr = UnityRva(RVA::Physics_RaycastHitOut);
        if (Tools::IsPtrValid(rayHitOutAddr))
            fn_PhysicsRaycastHitOut =
                (bool (*)(Vector3, Vector3, void *, float))rayHitOutAddr;

        // The same bounded ray without the out struct (0x506BF60) - the
        // conservative fallback used when a hit object cannot be named.
        auto raySegAddr = UnityRva(RVA::Physics_RaycastSegment);
        if (Tools::IsPtrValid(raySegAddr))
            fn_PhysicsRaycastSegment = (bool (*)(Vector3, Vector3, float))raySegAddr;

        // RaycastAll 0x506CBB0 (dump.cs 174011) - the whole RaycastHit[] for the
        // segment, so the LOS walk can SKIP the local player's own geometry and
        // non-target trigger volumes instead of being forced to accept them.
        // This is the fix for the false "BLOCKED: wall in the way": the old
        // single nearest-hit ray always landed on our own collider at the camera
        // (device diag: LOShit=0.00m) and never on the enemy.
        auto rayAllAddr = UnityRva(RVA::Physics_RaycastAll);
        if (Tools::IsPtrValid(rayAllAddr))
            fn_PhysicsRaycastAll = (void *(*)(Vector3, Vector3, float))rayAllAddr;
        else
            LOGE("[NewFeatures] triggerbot: RaycastAll RVA 0x%llX did not resolve -"
                 " LOS falls back to the single-ray + occlusion path",
                 (unsigned long long)RVA::Physics_RaycastAll);

        // Collider.get_isTrigger() 0x50279D4 (dump.cs 175345) - marks the
        // invisible volumes the walk has to skip.
        auto trigAddr = UnityRva(RVA::Collider_IsTrigger);
        if (Tools::IsPtrValid(trigAddr))
            fn_ColliderIsTrigger = (bool (*)(void *))trigAddr;

        // Scene-graph accessors used to name a hit (Transform.get_forward /
        // Transform.get_parent, both in the proven 0x50xxxxx block).
        auto fwdAddr = UnityRva(RVA::Transform_get_forward);
        if (Tools::IsPtrValid(fwdAddr))
            fn_TransformGetForward = (Vector3 (*)(void *))fwdAddr;

        auto parentAddr = UnityRva(RVA::Transform_get_parent);
        if (Tools::IsPtrValid(parentAddr))
            fn_TransformGetParent = (void *(*)(void *))parentAddr;

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

        auto aimingAddr = UnityRva(RVA::Pawn_IsAiming);
        if (Tools::IsPtrValid(aimingAddr))
            fn_PawnIsAiming = (bool (*)(void *))aimingAddr;

        auto firingAddr = UnityRva(RVA::Pawn_get_IsFiring);
        if (Tools::IsPtrValid(firingAddr))
            fn_PawnGetIsFiring = (bool (*)(void *))firingAddr;
        // ---- ShouldFire tree pointers ------------------------------------
        auto ggmAddr = UnityRva(RVA::Gameplay_get_MatchGame);
        if (Tools::IsPtrValid(ggmAddr))
            fn_GameGetMatchGame = (void *(*)())ggmAddr;

        auto autoFireAddr = UnityRva(RVA::Weapon_get_IsAutoFire);
        if (Tools::IsPtrValid(autoFireAddr))
            fn_WeaponIsAutoFire = (bool (*)(void *))autoFireAddr;

        auto burstAddr = UnityRva(RVA::Weapon_get_IsBurstFire);
        if (Tools::IsPtrValid(burstAddr))
            fn_WeaponIsBurstFire = (bool (*)(void *))burstAddr;

        auto singleAddr = UnityRva(RVA::Weapon_get_IsSingleShot);
        if (Tools::IsPtrValid(singleAddr))
            fn_WeaponIsSingleShot = (bool (*)(void *))singleAddr;

        auto ammoOkAddr = UnityRva(RVA::AmmoCanFire);
        if (Tools::IsPtrValid(ammoOkAddr))
            fn_WeaponAmmoCanFire = (bool (*)(void *))ammoOkAddr;

        auto clipAddr = UnityRva(RVA::Weapon_get_ClipAmmoCount);
        if (Tools::IsPtrValid(clipAddr))
            fn_WeaponGetClipAmmoCount = (int (*)(void *))clipAddr;

        auto canFireAddr = UnityRva(RVA::Weapon_CanFire);
        if (Tools::IsPtrValid(canFireAddr))
            fn_WeaponCanFire = (bool (*)(void *))canFireAddr;

        auto swallowingAddr = UnityRva(RVA::Weapon_InSwitchWeaponState);
        if (Tools::IsPtrValid(swallowingAddr))
            fn_WeaponInSwitchWeaponState = (bool (*)(void *))swallowingAddr;

        auto switchingAddr = UnityRva(RVA::Weapon_HasSwitchingState);
        if (Tools::IsPtrValid(switchingAddr))
            fn_WeaponHasSwitchingState = (bool (*)(void *))switchingAddr;

        auto realIntervalAddr = UnityRva(RVA::Weapon_RealFireInterval);
        if (Tools::IsPtrValid(realIntervalAddr))
            fn_WeaponRealFireInterval = (float (*)(void *))realIntervalAddr;

        auto aimTargetAddr = UnityRva(RVA::Weapon_get_AimingTarget);
        if (Tools::IsPtrValid(aimTargetAddr))
            fn_WeaponGetAimingTarget = (void *(*)(void *))aimTargetAddr;

        auto aimTargetDisAddr = UnityRva(RVA::Weapon_get_CurrentAimingTargetDis);
        if (Tools::IsPtrValid(aimTargetDisAddr))
            fn_WeaponGetAimingTargetDis = (float (*)(void *))aimTargetDisAddr;

        auto fovAddr = UnityRva(RVA::Camera_get_fieldOfView);
        if (Tools::IsPtrValid(fovAddr))
            fn_CameraGetFov = (float (*)(void *))fovAddr;

        auto teamPawnsAddr = UnityRva(RVA::BaseGame_GetTeamPawns);
        if (Tools::IsPtrValid(teamPawnsAddr))
            fn_BaseGameGetTeamPawns = (void *(*)(void *))teamPawnsAddr;

        auto pausedAddr = UnityRva(RVA::BaseGame_get_IsPaused);
        if (Tools::IsPtrValid(pausedAddr))
            fn_BaseGameIsPaused = (bool (*)(void *))pausedAddr;

        auto isAliveAddr = UnityRva(RVA::Pawn_get_IsAlive);
        if (Tools::IsPtrValid(isAliveAddr))
            fn_PawnIsAlive = (bool (*)(void *))isAliveAddr;


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

        // The ACTUAL game-thread heartbeat: every pawn class overrides Tick at
        // its own RVA, so il2cpp dispatch never reaches the base Pawn::Tick /
        // Weapon::Tick bodies for the local player's pawn/gun. Hook the
        // concrete overrides instead - these are what the game calls per frame.
        struct TickTarget { uintptr_t rva; void *hook; void **orig; const char *name; };
        const TickTarget pawnTickTargets[] = {
            { RVA::PlayerPawn_Tick,            (void *)&Hook_PlayerPawn_Tick,            (void **)&orig_PlayerPawn_Tick,            "PlayerPawn::Tick" },
            { RVA::MPPawn_Tick,                (void *)&Hook_MPPawn_Tick,                (void **)&orig_MPPawn_Tick,                "MPPawn::Tick"     },
            { RVA::BRPlayerPawn_Tick,          (void *)&Hook_BRPlayerPawn_Tick,          (void **)&orig_BRPlayerPawn_Tick,          "BRPlayerPawn::Tick" },
            { RVA::DMZPlayerPawn_Tick,         (void *)&Hook_DMZPlayerPawn_Tick,         (void **)&orig_DMZPlayerPawn_Tick,         "DMZPlayerPawn::Tick" },
            { RVA::PVEPlayerPawn_Tick,         (void *)&Hook_PVEPlayerPawn_Tick,         (void **)&orig_PVEPlayerPawn_Tick,         "PVEPlayerPawn::Tick" },
            { RVA::OpenWorldPlayerPawn_Tick,   (void *)&Hook_OpenWorldPlayerPawn_Tick,   (void **)&orig_OpenWorldPlayerPawn_Tick,   "OpenWorldPlayerPawn::Tick" },
            { RVA::BRGroundWarPlayerPawn_Tick, (void *)&Hook_BRGroundWarPlayerPawn_Tick, (void **)&orig_BRGroundWarPlayerPawn_Tick, "BRGroundWarPlayerPawn::Tick" },
        };
        for (const TickTarget &t : pawnTickTargets) {
            void *addr = UnityRva(t.rva);
            if (Tools::IsPtrValid(addr)) {
                Tools::Hook(addr, t.hook, t.orig);
                LOGI("[NewFeatures] triggerbot: hooked %s @%p -> orig %p",
                     t.name, addr, *t.orig);
            } else {
                LOGE("[NewFeatures] triggerbot: %s RVA 0x%llX did not resolve",
                     t.name, (unsigned long long)t.rva);
            }
        }

        LOGI("[NewFeatures] triggerbot: StartFire=%p StopFire=%p W2S=%p RayAll=%p RayHitOut=%p RaySeg=%p IsTrigger=%p Fwd=%p Parent=%p Raycast=%p",
             (void *)fn_Weapon_StartFire, (void *)fn_Weapon_StopFire,
             (void *)fn_WorldToScreen, (void *)fn_PhysicsRaycastAll,
             (void *)fn_PhysicsRaycastHitOut, (void *)fn_PhysicsRaycastSegment,
             (void *)fn_ColliderIsTrigger, (void *)fn_TransformGetForward,
             (void *)fn_TransformGetParent, (void *)fn_PhysicsRaycastLOS);
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
        // `static`: NoSprintFireDelayActive() reaches this from a hooked weapon
        // getter, i.e. potentially many times per frame - resolve the RVA once.
        static auto getCurrentWeapon = (void *(*)(void *))UnityRva(FiringTiming::LocalPawn_GetCurrentWeapon);
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
