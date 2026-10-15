// Host test for the triggerbot anti-wall (LOS) gate - the fix for
// "BLOCKED: wall in the way even though I can see the player".
//
// It builds a FAKE physics world (il2cpp RaycastHit[] arrays laid out per
// dump.cs 175618 + this repo's Il2CppArray, fake colliders with a readable
// m_CachedPtr at +0x10, fake transforms wired through the very
// (m_unity + apiN) accessors Defines.h uses) and drives the real gate code
// from NewFeatures.h:
//
//   * A_FireRayToTarget      - RaycastAll walk: self hits skipped, trigger
//                              volumes skipped, target named, wall blocked,
//                              near-origin escape via the game's own
//                              LastOccVisible verdict, single-ray fallback.
//   * A_FireRaycastAllNearest - unit checks of the array guards (a bad header
//                              must be REFUSED, never read as "no hits").
//   * A_FireCrosshairRayPawn - path A names the enemy past our own body.
//   * A_FireScan             - end-to-end: zone candidate + self@0 ray ->
//                              FireBlock::Fired; self + wall -> NoLineOfSight.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers
// are the stubs in Skin/tests, __fastcall is elided like the real build, and
// Tools::IsPtrValid is stubbed because Tools.cpp is not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Hacks/Feature/tests/los_gate_check.cpp \
//       -o /tmp/los_gate_check && /tmp/los_gate_check
// Exits 0 and prints "OK: all LOS gate checks passed" on success.
// Android.mk does not wildcard Hacks/Feature/tests, so it never reaches the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/Init_Hacks.h"   // sConfig Config - referenced by NewFeatures.h
#include "Hacks/Feature/NewFeatures.h"

// ---------------------------------------------------------------------------
// Tools::IsPtrValid - Tools.cpp is intentionally not linked; on the host every
// pointer handed to the gate is either null or inside our fake arena, so a
// null check is the honest "is this a live object" answer for the fakes.
// ---------------------------------------------------------------------------
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

// __android_log_print is what LOGI() expands to. The checks below now call
// A_FireTick(), whose fire path contains a LOGI - the symbol has to exist even
// though the test never reaches that line.
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

// Substrate's hook entry (pulled in by Includes/Macros.h's non-inline hook()
// helper) - the test installs no hooks, but the symbol has to exist to link.
void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

using namespace NewFeatures;

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

// ---------------------------------------------------------------------------
// Fake il2cpp objects.
//
// Layout actually read by the gate:
//   +0x10  UnityEngine.Object.m_CachedPtr   (A_FireObjectAlive)
//   +0x38  test-only: identity returned by the fake Component::get_transform
//   +0x48  test-only: parent returned by the fake Transform::get_parent
//   +0x100 test-only: Vector3 returned by the fake Transform::get_position
// Pawns additionally carry the dump-verified fields:
//   Class_Pawn_m_HeadBone (0x308), Class_Pawn_m_IsAlive (0x548),
//   Class_Pawn_m_Mesh (0x628)  - all #define'd in Defines.h.
// ---------------------------------------------------------------------------
static const size_t OBJ_SIZE = 2048;   // must cover 0x628 + 8
struct FakeObj { alignas(8) unsigned char b[OBJ_SIZE]; };

static FakeObj g_localRoot, g_targetRoot, g_wallRoot, g_trigRoot, g_deadRoot;
static FakeObj g_localPawn, g_targetPawn, g_otherPawn;
static FakeObj g_localHead, g_localMesh, g_targetHead, g_targetMesh;
static FakeObj g_selfCol, g_targetCol, g_wallCol, g_trigCol, g_deadCol;
static FakeObj g_camObj, g_camTransform, g_gameObj, g_weaponObj;

static void objInit(FakeObj &o, FakeObj *identity = nullptr)
{
    std::memset(o.b, 0, sizeof o.b);
    *(void **)(o.b + 0x10) = o.b;                                     // m_CachedPtr
    *(void **)(o.b + 0x38) = identity ? identity->b : o.b;            // get_transform()
}
static void objParent(FakeObj &t, FakeObj *p)  { *(void **)(t.b + 0x48) = p ? p->b : nullptr; }
static void objPos(FakeObj &o, float x, float y, float z)
{
    Vector3 v{x, y, z};
    std::memcpy(o.b + 0x100, &v, sizeof v);
}
static Vector3 objGetPos(const FakeObj &o)
{
    Vector3 v;
    std::memcpy(&v, o.b + 0x100, sizeof v);
    return v;
}
static void objSetAlive(FakeObj &pawn, bool alive)
{
    pawn.b[Class_Pawn_m_IsAlive] = alive ? 1 : 0;
}
static void objSetBone(FakeObj &pawn, size_t offset, FakeObj *bone)
{
    *(void **)(pawn.b + offset) = bone ? bone->b : nullptr;
}

// ---- scene-graph fakes, installed through the (m_unity + apiN) macros -----
static Transform *fake_get_transform(Component *c)
{
    return (Transform *)(*(void **)((char *)c + 0x38));
}
static Vector3 fake_get_position(Transform *t)
{
    Vector3 v;
    std::memcpy(&v, (char *)t + 0x100, sizeof v);
    return v;
}
static void *fake_get_parent(void *t)
{
    void *p = *(void **)((char *)t + 0x48);
    return p;
}
static Vector3 fake_get_forward(void *) { return Vector3{0.0f, 0.0f, 1.0f}; }
// The rest of the camera basis. Together with the forward above this is an
// orthonormal basis, which is what the crosshair ray math assumes and what a
// Unity camera transform actually gives.
static Vector3 fake_get_right(void *)   { return Vector3{1.0f, 0.0f, 0.0f}; }
static Vector3 fake_get_up(void *)      { return Vector3{0.0f, 1.0f, 0.0f}; }

// Pawn.get_HeadPosition() (m_unity + api18 = 0x51B86EC, Slot 215). This is the
// game's OWN head getter - the accessor the triggerbot uses instead of the
// m_HeadBone FIELD offset. The fake answers with the position of the pawn's
// m_HeadBone transform, i.e. exactly what the real getter returns on device.
static Vector3 fake_headPosition(Pawn *p)
{
    void *bone = *(void **)((char *)p + Class_Pawn_m_HeadBone);
    if (bone == nullptr) return Vector3{0.0f, 0.0f, 0.0f};
    return fake_get_position((Transform *)bone);
}

// ---- camera / screen fakes ------------------------------------------------
static Camera *fake_camera_main() { return (Camera *)g_camObj.b; }
static int fake_screen_w() { return 1600; }
static int fake_screen_h() { return 900; }

static Vector3 (*g_w2sImpl)(const Vector3 &) = nullptr;
static Vector3 fake_w2s(void *, Vector3 w)
{
    return g_w2sImpl ? g_w2sImpl(w) : Vector3{0.0f, 0.0f, 0.0f};
}

// ---- physics fakes --------------------------------------------------------
// One world per ray class: the centre ray uses kCrosshairRayMax (400 m), the
// LOS / lead rays use a bounded segment - the fake picks the list by maxDist.
static std::vector<std::pair<float, FakeObj *>> g_centreHits;
static std::vector<std::pair<float, FakeObj *>> g_losHits;
static int   g_segmentHit = 0;      // stem-test fallback answer
static int   g_rayAllCalls = 0, g_singleRayCalls = 0, g_segmentCalls = 0;
static float g_lastRayAllMax = -1.0f;

static unsigned char g_hitArrayBuf[0x20 + 64 * 0x30 + 64];

static const std::vector<std::pair<float, FakeObj *>> &hitsForMaxDist(float maxDist)
{
    return (maxDist > 100.0f) ? g_centreHits : g_losHits;
}

static void *fake_raycastAll(Vector3, Vector3, float maxDist)
{
    g_rayAllCalls++;
    g_lastRayAllMax = maxDist;
    const std::vector<std::pair<float, FakeObj *>> &hits = hitsForMaxDist(maxDist);
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = hits.size();
    for (size_t i = 0; i < hits.size(); i++) {
        unsigned char *e = g_hitArrayBuf + AF_ARRAY_DATA_OFF + i * AF_RAYHIT_SIZE;
        *(float *)(e + AF_RAYHIT_DISTOFF) = hits[i].first;
        *(void **)(e + AF_RAYHIT_COLLOFF) = hits[i].second->b;
    }
    return g_hitArrayBuf;
}

static bool fake_raycastHitOut(Vector3, Vector3, void *hitBuf, float maxDist)
{
    g_singleRayCalls++;
    const std::vector<std::pair<float, FakeObj *>> &hits = hitsForMaxDist(maxDist);
    if (hits.empty()) return false;
    std::memset(hitBuf, 0, AF_RAYHIT_SIZE);
    *(float *)((unsigned char *)hitBuf + AF_RAYHIT_DISTOFF) = hits[0].first;
    *(void **)((unsigned char *)hitBuf + AF_RAYHIT_COLLOFF) = hits[0].second->b;
    return true;
}

static bool fake_raySegment(Vector3, Vector3, float)
{
    g_segmentCalls++;
    return g_segmentHit != 0;
}

static int g_isTriggerFlag = 0;
static bool fake_isTrigger(void *) { return g_isTriggerFlag != 0; }

static int g_lastOccVisible = 1;
static bool fake_lastOccVisible(void *) { return g_lastOccVisible != 0; }
static int g_recentlyRendered = 1;
static bool fake_recentlyRendered(void *) { return g_recentlyRendered != 0; }

// ---------------------------------------------------------------------------
// World setup
// ---------------------------------------------------------------------------
static void resetWorld()
{
    objInit(g_localRoot);   objInit(g_targetRoot);
    objInit(g_wallRoot);    objInit(g_trigRoot);   objInit(g_deadRoot);

    objInit(g_localPawn,  &g_localRoot);
    objInit(g_targetPawn, &g_targetRoot);
    objInit(g_otherPawn);
    objInit(g_localHead);  objInit(g_localMesh);
    objInit(g_targetHead); objInit(g_targetMesh);

    objInit(g_selfCol,  &g_localRoot);     // OUR body/weapon collider
    objInit(g_targetCol, &g_targetRoot);   // the enemy's own collider
    objInit(g_wallCol,  &g_wallRoot);      // geometry
    objInit(g_trigCol,  &g_trigRoot);      // invisible trigger volume
    objInit(g_deadCol,  &g_deadRoot);      // readable collider, dead wrapper
    *(void **)(g_deadCol.b + 0x10) = nullptr;

    objInit(g_camObj, &g_camTransform);
    objInit(g_camTransform);
    objPos(g_camTransform, 0.0f, 1.6f, 0.0f);
    objInit(g_gameObj);
    objInit(g_weaponObj);

    objSetBone(g_localPawn,  Class_Pawn_m_HeadBone, &g_localHead);
    objSetBone(g_localPawn,  Class_Pawn_m_Mesh,     &g_localMesh);
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, &g_targetHead);
    objSetBone(g_targetPawn, Class_Pawn_m_Mesh,     &g_targetMesh);
    objSetAlive(g_localPawn, true);
    objSetAlive(g_targetPawn, true);
    objSetAlive(g_otherPawn, true);

    objPos(g_targetHead, 0.5f, 1.7f, 3.3f);
    objPos(g_targetMesh, 0.5f, 0.0f, 3.3f);
    // The pawn ROOT transform sits at the feet on device (same place as m_Mesh),
    // so the body span the trigger now measures (head -> root) is the same body
    // the old bone-field walk used.
    objPos(g_targetRoot, 0.5f, 0.0f, 3.3f);
    objPos(g_localRoot,  0.0f, 0.0f, 0.0f);
    objParent(g_wallRoot, nullptr);
    objParent(g_trigRoot, nullptr);

    g_centreHits.clear();
    g_losHits.clear();
    g_segmentHit = 0;
    g_isTriggerFlag = 0;
    g_lastOccVisible = 1;
    g_recentlyRendered = 1;
    g_rayAllCalls = g_singleRayCalls = g_segmentCalls = 0;
    g_lastRayAllMax = -1.0f;
    g_triggerDiag = TriggerbotDiag{};
}

static void installFns()
{
    // The two scene-graph entry points and the camera live behind
    // (m_unity + apiN) macros in Defines.h -> install the fakes there.
    m_unity = 0;
    api7  = (uintptr_t)&fake_camera_main;
    api9  = (uintptr_t)&fake_get_transform;
    api10 = (uintptr_t)&fake_get_position;
    api18 = (uintptr_t)&fake_headPosition;     // Pawn.get_HeadPosition
    api4  = (uintptr_t)&fake_screen_w;
    api5  = (uintptr_t)&fake_screen_h;

    fn_PhysicsRaycastAll      = fake_raycastAll;
    fn_PhysicsRaycastHitOut   = fake_raycastHitOut;
    fn_PhysicsRaycastSegment  = fake_raySegment;
    fn_ColliderIsTrigger      = fake_isTrigger;
    fn_TransformGetParent     = fake_get_parent;
    fn_TransformGetForward    = fake_get_forward;
    fn_TransformGetRight      = fake_get_right;
    fn_TransformGetUp         = fake_get_up;
    fn_PawnLastOccVisible     = fake_lastOccVisible;
    fn_PawnRecentlyRendered   = fake_recentlyRendered;
    fn_WorldToScreen          = fake_w2s;
    fn_PawnIsAlive            = nullptr;      // field read already answers
    fn_CameraGetFov           = nullptr;      // -> kFallbackFov, focal still sane
    fn_CameraGetMain          = nullptr;
    fn_WeaponGetAimingTarget  = nullptr;
    fn_WeaponGetAimingTargetDis = nullptr;
    fn_BaseGameGetTeamPawns   = nullptr;      // no teammates in this world
}

// ---------------------------------------------------------------------------
// 1. A_FireRayToTarget - the gate itself
// ---------------------------------------------------------------------------
static void testGate()
{
    const Vector3 from(0.0f, 1.6f, 0.0f);
    const Vector3 to(0.5f, 1.7f, 3.3f);
    const float bodyRadius = A_FireTune::kBodyRadius;

    // --- T1: empty line -> clear ------------------------------------------
    resetWorld();
    {
        bool ident = true; float hd = 5.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear && !ident && hd < 0.0f,
              "T1 empty line is clear, nothing identified");
        CHECK(g_rayAllCalls == 1, "T1 used RaycastAll (the walk, not the single ray)");
    }

    // --- T2: only the enemy on the line -> clear + identified -------------
    resetWorld();
    g_losHits = { {3.3f, &g_targetCol} };
    {
        bool ident = false; float hd = -1.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear && ident, "T2 nearest hit IS the target -> clear + identified");
        CHECK(std::fabs(hd - 3.3f) < 0.001f, "T2 hit distance reported for the panel");
    }

    // --- T3: OUR OWN BODY at 0 m, nothing else -> clear (THE BUG) ---------
    resetWorld();
    g_losHits = { {0.0f, &g_selfCol} };
    {
        bool ident = true; float hd = 99.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear, "T3 own body at the camera no longer blocks the shot");
        CHECK(!ident, "T3 not 'identified' - the ray never reached the enemy");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T3 self hit skipped + counted");
        CHECK(g_triggerDiag.occOverrides == 0, "T3 no occlusion override needed (line is simply empty)");
        CHECK(g_singleRayCalls == 0, "T3 answered from RaycastAll alone");
    }

    // --- T4: self at 0 m THEN the enemy -> clear + identified -------------
    resetWorld();
    g_losHits = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident,
              "T4 walk steps over our body and names the enemy behind it");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T4 exactly one self hit skipped");
    }

    // --- T5: self at 0 m + REAL wall at 2 m -> blocked ---------------------
    resetWorld();
    g_losHits = { {0.0f, &g_selfCol}, {2.0f, &g_wallCol} };
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T5 wall at range still blocks even with the self hit skipped");
        CHECK(g_triggerDiag.occOverrides == 0, "T5 occlusion does NOT rescue a blocker at range");
    }

    // --- T6: invisible trigger volume at the camera, alone -> clear --------
    resetWorld();
    g_losHits = { {0.05f, &g_trigCol} };
    g_isTriggerFlag = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T6 trigger volume cannot be 'the wall'");
        CHECK(g_triggerDiag.triggerHitsSkipped == 1, "T6 trigger hit skipped + counted");
    }

    // --- T7: trigger at the camera, enemy behind -> clear + identified -----
    resetWorld();
    g_losHits = { {0.05f, &g_trigCol}, {3.3f, &g_targetCol} };
    g_isTriggerFlag = 1;
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident, "T7 trigger skipped, enemy behind it still named");
    }

    // --- T8: solid geometry 10 cm from the camera + game says visible ------
    resetWorld();
    g_losHits = { {0.10f, &g_wallCol} };
    g_lastOccVisible = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T8 near-origin blocker + LastOccVisible -> clear");
        CHECK(g_triggerDiag.occOverrides == 1, "T8 override counted for the panel");
    }

    // --- T8b: same blocker, game says occluded -> blocked ------------------
    resetWorld();
    g_losHits = { {0.10f, &g_wallCol} };
    g_lastOccVisible = 0;
    g_recentlyRendered = 0;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T8b near-origin blocker + occluded -> blocked");
        CHECK(g_triggerDiag.occOverrides == 0, "T8b no override when the game agrees he is hidden");
    }

    // --- T9: wall at range + game says visible -> STILL blocked ------------
    resetWorld();
    g_losHits = { {2.0f, &g_wallCol} };
    g_lastOccVisible = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T9 occlusion never overrides a normal-range wall");
    }

    // --- T10: unnameable collider -> conservative stem test ----------------
    resetWorld();
    g_losHits = { {3.0f, &g_deadCol} };
    g_segmentHit = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T10 unnamed hit + something inside the stem range -> blocked");
    }
    resetWorld();
    g_losHits = { {3.0f, &g_deadCol} };
    g_segmentHit = 0;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T10b unnamed hit but the stem range is empty -> clear");
    }

    // --- T11: RaycastAll symbol missing -> single-ray fallback -------------
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {0.0f, &g_selfCol} };
    g_lastOccVisible = 1;
    {
        bool ident = true;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && !ident,
              "T11 fallback: single ray self-hit + visible -> clear (occlusion escape)");
        CHECK(g_singleRayCalls == 1, "T11 fallback really used the single-ray symbol");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T11 self hit counted on the fallback too");
    }
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {2.0f, &g_wallCol} };
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T11b fallback: a wall at range still blocks");
    }
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {3.3f, &g_targetCol} };
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident, "T11c fallback: the enemy himself is still identified");
    }
    fn_PhysicsRaycastAll = fake_raycastAll;

    // --- T12: no ray symbols at all -> Unknown (fail closed) ---------------
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    fn_PhysicsRaycastHitOut = nullptr;
    fn_PhysicsRaycastSegment = nullptr;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Unknown, "T12 no symbols at all -> Unknown, caller fails closed");
    }
    fn_PhysicsRaycastAll = fake_raycastAll;
    fn_PhysicsRaycastHitOut = fake_raycastHitOut;
    fn_PhysicsRaycastSegment = fake_raySegment;
}

// ---------------------------------------------------------------------------
// 2. A_FireRaycastAllNearest - the array guards
// ---------------------------------------------------------------------------
static void testArrayGuards()
{
    AF_NamedHit h{};

    // --- G1: null array -> refused (caller falls back) --------------------
    resetWorld();
    CHECK(!A_FireRaycastAllNearest(nullptr, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G1 null RaycastAll result is refused, not read as 'no hits'");

    // --- G2: absurd length (bad header/layout) -> refused ------------------
    resetWorld();
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = 100000;
    CHECK(!A_FireRaycastAllNearest(g_hitArrayBuf, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G2 out-of-range length -> refused (degrades to the single-ray path)");

    // --- G3: distance PhysX could not have produced -> refused -------------
    resetWorld();
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = 1;
    {
        unsigned char *e = g_hitArrayBuf + AF_ARRAY_DATA_OFF;
        *(float *)(e + AF_RAYHIT_DISTOFF) = 999999.0f;   // beyond any maxDistance
        *(void **)(e + AF_RAYHIT_COLLOFF) = g_wallCol.b;
    }
    CHECK(!A_FireRaycastAllNearest(g_hitArrayBuf, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G3 impossible hit distance -> refused");

    // --- G4: unsorted hits -> nearest one wins -----------------------------
    resetWorld();
    g_losHits = { {4.0f, &g_wallCol}, {1.5f, &g_wallCol}, {3.0f, &g_targetCol} };
    void *arr = fake_raycastAll(Vector3{}, Vector3{}, 5.0f);
    CHECK(A_FireRaycastAllNearest(arr, 5.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G4 well-formed array accepted");
    CHECK(!h.isTarget && h.distance > 1.4f && h.distance < 1.6f,
          "G4 unsorted records are walked nearest-first (1.5 m wall, not 4 m)");
}

// ---------------------------------------------------------------------------
// 3. A_FireCrosshairRayPawn - path A names the enemy past our own body
// ---------------------------------------------------------------------------
static void testCentreRay()
{
    Pawn *items[2] = { (Pawn *)g_targetPawn.b, (Pawn *)g_otherPawn.b };
    const Vector3 camPos(0.0f, 1.6f, 0.0f);
    const Vector3 forward(0.0f, 0.0f, 1.0f);

    // --- C1: our own body at 0.02 m, enemy behind --------------------------
    resetWorld();
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    {
        float hd = -1.0f; bool missing = false;
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, &hd, &missing);
        CHECK(p == (Pawn *)g_targetPawn.b, "C1 centre ray steps over our body and names the enemy");
        CHECK(hd > 4.9f && hd < 5.1f, "C1 centre hit distance is the enemy's");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "C1 self hit counted");
    }

    // --- C2: only our body -> nothing centred ------------------------------
    resetWorld();
    g_centreHits = { {0.02f, &g_selfCol} };
    {
        float hd = 7.0f; bool missing = false;
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, &hd, &missing);
        CHECK(p == nullptr && hd < 0.0f,
              "C2 centre ray on empty space (self skipped) -> no centred pawn");
    }

    // --- C3: a wall first -> nothing centred (never a wall-bang) -----------
    resetWorld();
    g_centreHits = { {2.0f, &g_wallCol}, {5.0f, &g_targetCol} };
    {
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(p == nullptr, "C3 a wall first on the centre line -> no centred pawn");
    }
}

// ---------------------------------------------------------------------------
// 4. A_FireScan - end to end (path B fires, wall refuses)
// ---------------------------------------------------------------------------
static Il2CppArray<Pawn *> g_enemyArr;
static Il2CppList<Pawn *>  g_enemyList;

static void setEnemyList(Pawn **pawns, int n)
{
    std::memset(&g_enemyArr, 0, 16 + 8 + 4);          // header only (m_Items untouched zeros)
    g_enemyArr.max_length = n;
    g_enemyArr.bounds = nullptr;
    for (int i = 0; i < n; i++) g_enemyArr.m_Items[i] = pawns[i];
    g_enemyList.klass = nullptr;
    g_enemyList.unk1   = nullptr;
    g_enemyList.items  = &g_enemyArr;
    g_enemyList.size   = n;
    *(void **)(g_gameObj.b + Class_BaseGame_EnemyPawns) = &g_enemyList;
}

static Vector3 w2sOnCrosshair(const Vector3 &) { return Vector3{820.0f, 450.0f, 1.0f}; }   // 20 px off centre
static Vector3 w2sAwayFromCrosshair(const Vector3 &) { return Vector3{1400.0f, 450.0f, 1.0f}; }

static void testScan()
{
    // Screen centre is (800, 450) for the 1600x900 fake screen; the zone
    // defaults to 40 px (2.5 % of width) because Config is zero-initialised.
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    // --- S1: enemy in the zone, ray hits OUR body at 0 m first -> FIRES ----
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };            // nothing centred
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "S1 enemy in the open -> FIRED (the old build said 'wall in the way')");
        CHECK(out.pawn == (Pawn *)g_targetPawn.b, "S1 the right pawn was chosen");
        CHECK(out.path == 1, "S1 went through the crosshair-zone path");
        CHECK(out.visible && out.visibilityIdentified,
              "S1 visibility came from the ray naming him, not from a guess");
        CHECK(g_triggerDiag.selfHitsSkipped >= 1, "S1 the self hit was skipped on the way");
        CHECK(g_triggerDiag.losBlocked == 0, "S1 nothing counted as blocked");
        CHECK(g_triggerDiag.lastBlock != (int)FireBlock::NoLineOfSight,
              "S1 'wall in the way' was NOT the verdict");
    }

    // --- S2: same, but a real wall between camera and enemy -> BLOCKED -----
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {2.0f, &g_wallCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoLineOfSight, "S2 a genuine wall between us and him still refuses");
        CHECK(!out.visible && out.pawn == nullptr, "S2 no target escapes with the refusal");
        CHECK(g_triggerDiag.losBlockedScan == 1, "S2 the block is reported in the panel counters");
    }

    // --- S3: centre ray names him -> path A fires --------------------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "S3 centre ray names the enemy -> fires");
        CHECK(out.path == 0 && out.pawn == (Pawn *)g_targetPawn.b,
              "S3 reported as the centre-ray path");
    }

    // --- S4: nobody near the crosshair -> NoEnemyInZone --------------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sAwayFromCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "S4 nobody in the zone -> no enemy, no rays");
        CHECK(g_triggerDiag.candidates == 0, "S4 candidate counter agrees");
    }

    // --- S5: dead enemy in the zone -> refused before any ray --------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    objSetAlive(g_targetPawn, false);
    g_losHits = { {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "S5 a dead pawn is not a candidate");
        CHECK(g_rayAllCalls == 1, "S5 only the centre ray ran (no LOS ray for a non-candidate)");
    }

    g_w2sImpl = nullptr;
}

// ---------------------------------------------------------------------------
// 6. Target filter (the menu's Filter dropdown)
// ---------------------------------------------------------------------------
static void testFilter()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    // --- F1: VisibleOnly, and the GAME says he is occluded -> refused -------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    Config.NewFeaturesMenu.TriggerbotFilter = (int)TriggerFilter::VisibleOnly;
    g_lastOccVisible = 0; g_recentlyRendered = 0;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "F1 VisibleOnly: game reports him occluded -> refused");
        CHECK(g_triggerDiag.filteredOut == 1, "F1 the panel counts the filtered pawn");
        CHECK(g_rayAllCalls == 1, "F1 dropped before any LOS ray (centre ray only)");
    }

    // --- F2: same, but the game reports him visible -> FIRES ---------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    Config.NewFeaturesMenu.TriggerbotFilter = (int)TriggerFilter::VisibleOnly;
    g_lastOccVisible = 1; g_recentlyRendered = 1;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "F2 VisibleOnly + visibly drawn pawn -> fires");
        CHECK(g_triggerDiag.filteredOut == 0, "F2 nothing filtered");
    }

    // --- F3: a range filter inside its own limit -> fires ------------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    Config.NewFeaturesMenu.TriggerbotFilter = (int)TriggerFilter::Within25m;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "F3 Within 25 m: a 3.3 m pawn is inside it");
        CHECK(g_triggerDiag.filteredOut == 0, "F3 in range -> not filtered");
    }

    // --- F4: the same enemy at 40 m -> refused by the range filter ---------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    Config.NewFeaturesMenu.TriggerbotFilter = (int)TriggerFilter::Within25m;
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {38.0f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "F4 Within 25 m: a 40 m pawn is out of range");
        CHECK(g_triggerDiag.filteredOut == 1, "F4 counted as filtered, not as a wall");
        CHECK(g_triggerDiag.losBlocked == 0, "F4 the range filter never reports 'no line of sight'");
    }

    // --- F5: All (the default) filters nothing -----------------------------
    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    Config.NewFeaturesMenu.TriggerbotFilter = (int)TriggerFilter::All;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "F5 Filter=All is the old behaviour");
        CHECK(g_triggerDiag.filteredOut == 0, "F5 nothing filtered");
    }

    Config.NewFeaturesMenu.TriggerbotFilter = 0;
    g_w2sImpl = nullptr;
}

// ---------------------------------------------------------------------------
// 7. Tick-hook cost: the hooks fire for every pawn and weapon in the world
// ---------------------------------------------------------------------------
static int g_localPawnLookups = 0;
static Pawn *fake_localPawnCounted()
{
    g_localPawnLookups++;
    return (Pawn *)g_localPawn.b;
}

// ---------------------------------------------------------------------------
// 7. The dot crosshair drives the triggerbot
//
// The crosshair_check test proves the geometry; this proves the SCAN obeys it -
// that moving the dot really moves where the trigger fires, and that the
// defaults leave the trigger exactly where it always was.
// ---------------------------------------------------------------------------
static void setCrosshair(float offX, float offY, bool wired)
{
    Config.Crosshair.Enabled = true;
    Config.Crosshair.TriggerbotUsesIt = wired;
    Config.Crosshair.OffsetX = offX;
    Config.Crosshair.OffsetY = offY;
}

static FireBlock runScanOnCrosshairSetup(TriggerTarget &out)
{
    // Same world as S1: one enemy in the zone, the centre ray landing on our own
    // body (so path A names nothing and the zone decides).
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;            // the enemy projects 20 px right of centre
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    return A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
}

static void testCrosshairAim()
{
    // --- X1: the defaults are the old behaviour, exactly -------------------
    resetWorld();
    setCrosshair(0.0f, 0.0f, true);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(b == FireBlock::Fired && out.path == 1,
              "X1 with the default 0 px offsets the trigger still fires from the centre");
        CHECK(g_triggerDiag.crosshairCx == 800.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X1 the scan reports the screen centre as its crosshair (800,450)");
        CHECK(g_triggerDiag.crosshairOffsetUsed == 0 && g_triggerDiag.crosshairOffsetDropped == 0,
              "X1 no offset was applied and none was dropped");
    }

    // --- X2: moving the dot moves the point the scan measures from ---------
    resetWorld();
    setCrosshair(40.0f, -25.0f, true);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 840.0f && g_triggerDiag.crosshairCy == 425.0f,
              "X2 the scan now aims through the dot (800+40, 450-25)");
        CHECK(g_triggerDiag.crosshairOffsetUsed == 1,
              "X2 the offset really reached the centre ray (used=1)");
        CHECK(b == FireBlock::Fired, "X2 the enemy 20 px right of centre is still in the zone");
    }

    // --- X3: the zone follows the dot, so a far-away dot stops firing ------
    resetWorld();
    setCrosshair(200.0f, 0.0f, true);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X3 with the dot 200 px right the enemy at +20 px is outside the 40 px zone");
        CHECK(b != FireBlock::Fired, "X3 ... so the trigger does NOT fire him");
    }

    // --- X4: unwired, the dot no longer influences the trigger -------------
    resetWorld();
    setCrosshair(200.0f, 0.0f, false);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 800.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X4 unwired: the scan is back on the exact screen centre");
        CHECK(g_triggerDiag.crosshairOffsetUsed == 0, "X4 unwired: no offset reached the ray");
        CHECK(b == FireBlock::Fired, "X4 unwired: the same shot as before the dot existed");
    }

    // --- X5: a missing camera basis degrades to the centre ray, reported ---
    resetWorld();
    setCrosshair(40.0f, -25.0f, true);
    fn_TransformGetRight = nullptr;
    fn_TransformGetUp    = nullptr;
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairOffsetDropped == 1,
              "X5 offset asked with no camera basis -> reported as dropped, not silently ignored");
        CHECK(g_triggerDiag.crosshairOffsetUsed == 0,
              "X5 ... and the centre ray stays the plain camera forward");
        CHECK(b == FireBlock::Fired,
              "X5 the zone path still covers the dot, so the trigger keeps working");
    }
    fn_TransformGetRight = fake_get_right;
    fn_TransformGetUp    = fake_get_up;

    // Leave the config as the rest of the suite expects it (defaults).
    resetWorld();
    setCrosshair(0.0f, 0.0f, true);
    Config.Crosshair.Enabled = false;
}

static void resetLocalPawnCache()
{
    g_afireLocalPawnCache = nullptr;
    g_afireLocalPawnCacheTime = -1000.0;
    g_afireHeldWeaponCache = nullptr;
    g_afireHeldWeaponStamp = -1000.0;
    g_afireOffDiagStamp = -1000.0;
    g_afireLastEvalTime = -1000.0;
    g_afireHoldingFire = false;
    g_afireFiringOn = nullptr;
    g_afireTargetPawn = nullptr;
}

static void testTickHookCost()
{
    // GamePlay::get_LocalPawn() is (m_unity + api12) - count the calls.
    api12 = (uintptr_t)&fake_localPawnCounted;

    // --- H0: control - the raw getter really is called once per use --------
    // Without this, "<= 2 lookups for 200 calls" could just be a counter that
    // never counts.
    resetLocalPawnCache();
    g_localPawnLookups = 0;
    for (int i = 0; i < 200; i++) { Pawn *p = GamePlay::get_LocalPawn(); (void)p; }
    CHECK(g_localPawnLookups == 200,
          "H0 control: 200 raw get_LocalPawn() calls are 200 native calls");

    // --- H1: the cached local pawn ----------------------------------------
    resetLocalPawnCache();
    g_localPawnLookups = 0;
    for (int i = 0; i < 200; i++) { Pawn *p = A_FireLocalPawnFast(); (void)p; }
    CHECK(g_localPawnLookups <= 2,
          "H1 200 cached local-pawn reads cost <= 2 native GamePlay::get_LocalPawn calls");

    // --- H2: pawn ticks of OTHER pawns ------------------------------------
    resetLocalPawnCache();
    g_localPawnLookups = 0;
    g_triggerDiag = TriggerbotDiag{};
    for (int i = 0; i < 200; i++) A_FirePawnTicked(&g_otherPawn);
    CHECK(g_localPawnLookups <= 2,
          "H2 200 foreign pawn ticks cost <= 2 native lookups (was one each)");
    CHECK(g_triggerDiag.pawnLocalTicks == 0, "H2 no foreign tick was mistaken for ours");

    // --- H3: the feature is OFF -------------------------------------------
    Config.NewFeaturesMenu.Triggerbot = false;
    resetLocalPawnCache();
    g_localPawnLookups = 0;
    g_triggerDiag = TriggerbotDiag{};
    for (int i = 0; i < 200; i++) A_FireTick(nullptr);
    CHECK(g_localPawnLookups <= 1,
          "H3 200 hook calls with the triggerbot OFF cost <= 1 native lookup");
    CHECK(g_triggerDiag.blockCount[(int)FireBlock::FeatureOff] >= 1,
          "H3 the panel still receives a live 'triggerbot off' verdict");
    CHECK(g_triggerDiag.lastTickTime > 0.0, "H3 the heartbeat stays alive");
}

int main()
{
    installFns();
    resetWorld();

    testGate();
    testArrayGuards();
    testCentreRay();
    testScan();
    testFilter();
    testCrosshairAim();
    testTickHookCost();

    if (g_failures == 0) {
        printf("\nOK: all LOS gate checks passed\n");
        return 0;
    }
    printf("\n%d check(s) FAILED\n", g_failures);
    return 1;
}
