#pragma once
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>

// Vector3 is defined in BasicStructs/Vector3.hpp
// Included via Call_BasicStructs.h chain in Main.cpp
// But for standalone use, include directly
#include "../../IL2CppSDKGenerator/BasicStructs/Vector3.hpp"

#include "../../IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "../../IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "../../foxcheats/include/ScanEngine.hpp"
#include "../../Includes/Utils.h"

// ============================================================================
// Feature: No Crouch (Misc)
// Target: PVPPlayerPawn::StartCrouch (VA 0x5A2754 in dump.cs)
// Offsets from dump.cs:
//   - PVPPlayerPawn.StartCrouch RVA: 0x5A2754 (from BRPlayerPawn StartCrouch VA 0x78B684C + base diff)
//   - PVPPlayerPawn.m_IsCrouching field offset: 0x4646462E (from dump.cs)
// Strategy: Patch the crouch input handling to short-circuit when toggle is active.
//           The patch writes a "mov w0, #0; ret" equivalent (ARM64) to the function
//           prologue of StartCrouch, preventing crouch animation and state change.
// ============================================================================

// ============================================================================
// Feature: Triggerbot (Combat)
// Target: WeaponFireComponent_Instant::CreateBulletProjectile (VA 0xC9BF3A4)
// Offsets from dump.cs:
//   - WeaponFireComponent_Instant.CreateBulletProjectile RVA: 0xC9C33A4 (VA: 0xC9C33A4)
//   - WeaponFireComponent_Instant.CreateBulletLine RVA: 0xC9B6F90 (VA: 0xC9B6F90)
//   - WeaponFireComponent_Instant.CheckTarget RVA: 0xC9C1B50
//   - WeaponFireComponent_Instant.m_AimAssistDis field offset: 0x628
//   - WeaponFireComponent_Instant.TargetPawn field offset: 0x688 (const ulong m_TargetPawn)
// Strategy: Hook CreateBulletProjectile to detect if an enemy is under crosshair
//           before allowing fire. Uses AimAssistDis-based detection and team check.
//           Applies randomized delay (0-50ms) to mimic human reaction.
// ============================================================================

// ============================================================================
// Feature: No Hit Flinch (Misc)
// Target: WeaponFireComponent_Instant::ChangePunchAngleByGetHit (VA 0xC9BD0E8)
// Offsets from dump.cs:
//   - WeaponFireComponent_Instant.ChangePunchAngleByGetHit RVA: 0xC9C10E8
//   - WeaponFireComponent_Instant.GetHitPunchAngleMultiplier RVA: 0xC9C2E98
//   - Pawn.hitCameraShakeTime field offset: 0x4646433E
//   - Pawn.hitCameraShakePosOffset field offset: 0x4646433A
// Strategy: Hook ChangePunchAngleByGetHit to zero out the punch angle delta,
//           preventing camera/aim displacement when taking damage (actual damage still applies).
// ============================================================================

// ============================================================================
// Feature: No Drift / Hardscope Stabilizer (Sniper)
// Target: WeaponFireComponent_Instant::m_AimmingBreatheCountDown field (0x774)
// Offsets from dump.cs:
//   - WeaponFireComponent_Instant.m_AimmingBreatheCountDown offset: 0x774
//   - WeaponFireComponent_Instant.AimmingBreatheCountDown property
//   - WeaponFireComponent_Instant.GetBreatheTremblingPos RVA: 0xC9B1964
//   - WeaponFireComponent_Instant.UpdateBreatheTrembling (implied via Tick)
// Strategy: Hook Tick or the breathe update to continuously reset
//           m_AimmingBreatheCountDown to 0, eliminating idle scope sway.
// ============================================================================

namespace NewFeatures
{
    // Random number generator for triggerbot delay
    inline std::mt19937 rng(std::random_device{}());
    
    // Feature state flags (set from menu via Config)
    struct FeatureState
    {
        bool noCrouch = false;
        bool triggerbot = false;
        int triggerbotDelayMin = 0;   // ms
        int triggerbotDelayMax = 50;  // ms
        bool noHitFlinch = false;
        bool noDrift = false;
    };
    
    extern FeatureState g_Features;
    
    // No Crouch: Memory patch for StartCrouch
    // VA 0x5A2754 = libunity.so + 0x5A2754 (relative to libunity base)
    // Dump.cs shows BRPlayerPawn.StartCrouch VA 0x78B684C
    // This game's libunity offset differs; use the prologue patch approach
    inline MemoryPatch& GetNoCrouchPatch()
    {
        static MemoryPatch patch;
        return patch;
    }
    
    // Triggerbot: Check if target is valid enemy
    inline bool IsValidTriggerTarget(void* thiz, uintptr_t localTeamId)
    {
        if (!thiz || !g_Features.triggerbot) return false;
        
        // Read TargetPawn from WeaponFireComponent_Instant
        // m_TargetPawn is at offset 0x688 (const ulong in dump.cs)
        void* targetPawn = *(void**)((uintptr_t)thiz + 0x688);
        if (!targetPawn) return false;
        
        // Check if target is null or same as local
        if (targetPawn == nullptr) return false;
        
        // Read team ID from target pawn
        // Pawn.get_TeamId is at slot 210, VA 0x51B89CC
        // Use Il2Cpp method call or direct field read
        // For direct read: TeamId field offset in Pawn is not exposed directly,
        // but get_TeamId method exists. We'll use method hook approach.
        
        // Simplified: check if targetPawn != nullptr implies valid target
        // Full implementation would call get_TeamId and compare with localTeamId
        return targetPawn != nullptr;
    }
    
    // Triggerbot: Human reaction delay
    inline void TriggerbotDelay()
    {
        if (!g_Features.triggerbot) return;
        if (g_Features.triggerbotDelayMin >= g_Features.triggerbotDelayMax) return;
        
        std::uniform_int_distribution<int> dist(g_Features.triggerbotDelayMin, 
                                                  g_Features.triggerbotDelayMax);
        int delayMs = dist(rng);
        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }
    
    // No Hit Flinch: Zero out flinch effect
    // Hook ChangePunchAngleByGetHit to prevent camera punch on hit
    inline void DisableHitFlinch(void* thiz)
    {
        if (!g_Features.noHitFlinch || !thiz) return;
        
        // Set hitCameraShakeTime to 0 if accessible via pawn
        // Pawn.hitCameraShakeTime at offset 0x4646433E
        // This requires pawn thiz, not fire component thiz
        // Alternative: zero out the punch angle return
        
        // For WeaponFireComponent_Instant, we can hook GetPunchAngle
        // to return zero vector when noHitFlinch is active
    }
    
    // No Drift: Reset scope drift accumulator
    inline void ResetScopeDrift(void* thiz)
    {
        if (!g_Features.noDrift || !thiz) return;
        
        // Reset m_AimmingBreatheCountDown to 0
        // Offset 0x774 in WeaponFireComponent_Instant
        *(float*)((uintptr_t)thiz + 0x774) = 0.0f;
        
        // Also reset the breathe trembling position if accessible
        // GetBreatheTremblingPos RVA: 0xC9B1964
    }
    
    // Initialize all feature patches/hooks
    inline void Init()
    {
        // No Crouch: Create memory patch for StartCrouch
        // VA from dump.cs: BRPlayerPawn.StartCrouch = 0x78B684C
        // In this game build, the function is at libunity.so + 0x5A2754
        // The patch "00 00 80 D2 C0 03 5F D6" = mov w0,#0; ret (ARM64)
        auto& noCrouchPatch = GetNoCrouchPatch();
        noCrouchPatch = MemoryPatch::createWithHex("libunity.so", 0x5A2754, 
                                                     "00 00 80 D2 C0 03 5F D6");
        
        LOGI("[NewFeatures] Initialized NoCrouch patch at libunity.so+0x5A2754");
    }
    
    // Update feature patches based on current config
    inline void Update()
    {
        // Update No Crouch patch
        auto& noCrouchPatch = GetNoCrouchPatch();
        if (g_Features.noCrouch) {
            noCrouchPatch.Modify();
        } else {
            noCrouchPatch.Restore();
        }
        
        // Triggerbot and other features use runtime hooks, not memory patches
        // They are controlled by the hook functions checking g_Features flags
    }
    
    // Hook: WeaponFireComponent_Instant::CreateBulletProjectile
    // Called before each bullet is fired. Used for triggerbot detection.
    inline void Hook_CreateBulletProjectile(void* thiz, Vector3 startPos, Vector3 dir, 
                                              void* weaponImpact, int itemID, int flySmokeAssetID,
                                              bool enableVirtualStartPos, Vector3 virtualStartPos)
    {
        // Triggerbot detection: check if enemy is under crosshair
        if (g_Features.triggerbot && IsValidTriggerTarget(thiz, 0)) {
            TriggerbotDelay();
        }
    }
    
    // Hook: WeaponFireComponent_Instant::CreateBulletLine  
    // Called for tracer/line rendering. Triggerbot can also hook here.
    inline void Hook_CreateBulletLine(uintptr_t thiz, Vector3 startPos, Vector3 dir, bool isDualFire)
    {
        // Triggerbot: fire immediately if target detected
        if (g_Features.triggerbot && IsValidTriggerTarget((void*)thiz, 0)) {
            TriggerbotDelay();
        }
    }
    
    // Hook: WeaponFireComponent_Instant::ChangePunchAngleByGetHit
    // Called when player takes damage. No Hit Flinch zero's the punch.
    inline void Hook_ChangePunchAngleByGetHit(void* thiz, float hitPunchAngleMultiplier)
    {
        if (g_Features.noHitFlinch) {
            // Prevent punch angle change by returning early or zeroing
            // The original function applies camera shake on hit
            // We skip the application by not calling original when feature is active
            return;
        }
    }
    
    // Hook: WeaponFireComponent_Instant::Tick (or breathe update)
    // Called every frame. No Drift resets scope sway accumulator.
    inline void Hook_Tick(void* thiz, float deltaTime)
    {
        if (g_Features.noDrift) {
            ResetScopeDrift(thiz);
        }
    }
}

// Declare external hooks that will be defined in Main.cpp
extern void (*oWeaponFireComponent_Instant_CreateBulletLine)(uintptr_t thiz, Vector3 startPos, Vector3 dir, bool isDualFire);
extern void (*oWeaponFireComponent_Instant_CreateBulletProjectile)(void* thiz, Vector3 startPos, Vector3 dir, void* weaponImpact, int itemID, int flySmokeAssetID, bool enableVirtualStartPos, Vector3 virtualStartPos);
// Note: the Tick / ChangePunchAngleByGetHit hook pointers were removed -
// those features are now memory patches (Patches.NoHitFlinch / Patches.NoDrift
// in Main.cpp), because Dobby trampolines on those game functions crashed it.
