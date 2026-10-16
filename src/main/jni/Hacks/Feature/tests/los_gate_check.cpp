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
//   * the path A body bound   - the dot has to be ON the enemy's projected
//                              BODY (its WIDTH, not half its height): a dot
//                              beside his arm or weapon is refused.
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
// Put the enemy's body exactly under the CENTRE dot (800,450 in the fake world).
// The fake world has to be self-consistent for path A: a ray through the dot
// cannot honestly name a body that projects 118 px to the side of it - that IS
// the beside-the-arm case the bound refuses (X9) - so every check that expects
// the dot's own ray to fire first puts him where the dot is.
static void objTargetUnderDot(float dist = 3.3f)
{
    objPos(g_targetHead, 0.0f, 1.7f, dist);
    objPos(g_targetMesh, 0.0f, 0.0f, dist);
    objPos(g_targetRoot, 0.0f, 0.0f, dist);
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

// Camera.ScreenToWorldPoint(0x5023D34) - the inverse of the projection above.
// The camera basis is (+Z forward, +X right, +Y up) with the camera at
// (0, 1.6, 0), and the focal length is the fallback the code itself would use
// for a 1600x900 screen, so this is a REAL inverse projection: the ray the scan
// builds from the dot's pixel can be projected back onto that pixel.
static Vector3 fake_screen_to_world(void *, Vector3 v)
{
    const float f = (900.0f * 0.5f) /
                    tanf(NewFeatures::A_FireTune::kFallbackFov * 0.5f *
                         NewFeatures::A_FireTune::kDegToRad);
    const float dx = v.x - 800.0f;
    const float dy = v.y - 450.0f;
    Vector3 dir = Vector3{0.0f, 0.0f, 1.0f}
                + Vector3{1.0f, 0.0f, 0.0f} * (dx / f)
                - Vector3{0.0f, 1.0f, 0.0f} * (dy / f);
    dir = Vector3::Normalized(dir);
    return Vector3{0.0f + dir.x * v.z, 1.6f + dir.y * v.z, 0.0f + dir.z * v.z};
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
    fn_ScreenToWorldPoint     = fake_screen_to_world;
    fn_PawnIsAlive            = nullptr;      // field read already answers
    fn_CameraGetFov           = nullptr;      // -> kFallbackFov, focal still sane
    fn_CameraGetMain          = nullptr;
    fn_WeaponGetAimingTarget  = nullptr;
    fn_WeaponGetAimingTargetDis = nullptr;
    fn_BaseGameGetTeamPawns   = nullptr;      // no teammates in this world
    // The triggerbot's ADS + re-press entry points. They are installed per test
    // (see testAdsGate / testRepressPacing) and nulled here so nothing leaks
    // from one test into the next.
    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    fn_WeaponCanFire                 = nullptr;
    fn_WeaponRealFireInterval        = nullptr;
    fn_WeaponIsAutoFire              = nullptr;

    // The dot crosshair IS the trigger's crosshair now: its pixel is what the
    // scan measures from and its radius is the zone it accepts. Put it on the
    // centre with an 80 px dot (a 40 px radius), which is the zone the worlds
    // below were built around.
    Config.Crosshair.Enabled = true;
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
    Config.Crosshair.Size    = 80.0f;
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

// Make the HONEST dot ray unavailable: an off-centre dot that neither the game's
// projection nor the camera basis can honour -> CrosshairRayDir answers with no
// flags, so the accuracy gate leaves the zone scan running - the only runner
// left that can still measure the drawn dot's zone. This is exactly the device
// condition that needs path A skipped and path B tested on its own.
static void killHonestDotRay()
{
    Config.Crosshair.OffsetX = 30.0f;
    Config.Crosshair.OffsetY = 0.0f;
    fn_ScreenToWorldPoint    = nullptr;
    fn_TransformGetRight     = nullptr;
    fn_TransformGetUp        = nullptr;
}
static void restoreHonestDotRay()
{
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
    fn_ScreenToWorldPoint    = fake_screen_to_world;
    fn_TransformGetRight     = fake_get_right;
    fn_TransformGetUp        = fake_get_up;
}

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

// The REAL forward projection, the inverse of fake_screen_to_world: the camera
// at (0,1.6,0) with the basis (+Z forward, +X right, +Y up) and the fallback
// focal length. Path A's projected-body bound computes the body half-span from
// head/feet projections - a constant-point fake would make every body a
// 0-px span and refuse everything, so where path A is expected to fire this
// projects world positions the way the device camera does.
static Vector3 w2sTrueProjection(const Vector3 &w)
{
    const float f = (900.0f * 0.5f) /
                    tanf(NewFeatures::A_FireTune::kFallbackFov * 0.5f *
                         NewFeatures::A_FireTune::kDegToRad);
    const float dx = w.x - 0.0f;
    const float dy = w.y - 1.6f;
    const float dz = w.z - 0.0f;
    return Vector3{800.0f + (dx / dz) * f, 450.0f - (dy / dz) * f, dz};
}

static void testScan()
{
    // The dot is on the centre of the 1600x900 fake screen, so the scan measures
    // from (800, 450) and accepts a 40 px radius (installFns' 80 px dot).
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    // --- S1: enemy in the zone, ray hits OUR body at 0 m first -> FIRES ----
    // Path B needs a scan with NO honest dot ray: with one, the accuracy gate
    // would refuse everything the ray does not name before path B even runs.
    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
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
    restoreHonestDotRay();

    // --- S2: same, but a real wall between camera and enemy -> BLOCKED -----
    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
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
    restoreHonestDotRay();

    // --- S3: centre ray names him -> path A fires --------------------------
    // Path A now also demands a projected body: the true projection is what
    // makes head/feet land on real screen positions (the old constant-point
    // fake turned every body into a 0-px span).
    resetWorld();
    objTargetUnderDot();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
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
// 6. The target filter is GONE (All / Visible Only / Within 25-50-100 m)
//
// The option never changed a shot on device, so it was removed from the scan, the
// UI and the config. These checks pin the removal down: a pawn the game itself
// reports as occluded, and a pawn far outside every distance the old filter
// offered, must both remain shootable - the visibility RAY is the only opinion
// left about whether a shot is possible.
// ---------------------------------------------------------------------------
static void testNoTargetFilter()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    // --- F1: the game reports him occluded -> the scan does not care --------
    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_lastOccVisible = 0; g_recentlyRendered = 0;      // what "Visible Only" refused
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired,
              "F1 no filter: an occluded-by-the-game pawn is still fired");
        CHECK(g_rayAllCalls >= 1,
              "F1 ... decided by a real visibility ray, not by a getter veto");
    }
    restoreHonestDotRay();

    // --- F2: 40 m away, under the crosshair -> still fired ------------------
    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);           // what "Within 25 m" refused
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {38.0f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "F2 no filter: a 40 m pawn under the dot is fired");
        CHECK(g_triggerDiag.losBlocked == 0,
              "F2 ... and it is not reported as a wall either");
    }
    restoreHonestDotRay();

    // --- F3: both removed rules at once -------------------------------------
    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_lastOccVisible = 0; g_recentlyRendered = 0;
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {38.0f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired,
              "F3 occluded AND 40 m away: still fired - neither removed rule survives");
        CHECK(g_triggerDiag.losBlocked == 0, "F3 ... and never reported as a wall");
    }
    restoreHonestDotRay();

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
// 8. The dot crosshair IS the trigger's crosshair
//
// The crosshair_check test proves the geometry; this proves the SCAN obeys it -
// that the pixel the trigger measures from and the radius it accepts are both
// the dot, always, with no switch to disconnect them. This is the behaviour the
// "even with use-dot-for-triggerbot on, and the dot small, it still shot from
// the game's centre" report asked for.
// ---------------------------------------------------------------------------
static void setCrosshair(float offX, float offY, float sizePx, bool drawn = true)
{
    Config.Crosshair.Enabled = drawn;
    Config.Crosshair.OffsetX = offX;
    Config.Crosshair.OffsetY = offY;
    Config.Crosshair.Size    = sizePx;
}

static FireBlock runScanOnCrosshairSetup(TriggerTarget &out, bool dotSeesPawn = false)
{
    // One enemy 20 px right of the 1600x900 centre (820,450). The dot ray's hit
    // list is separate from the LOS list, so a test can put him under the dot's
    // ray (path A fires) or leave the ray hitting only our own body - either
    // way the decision belongs to the ray once an honest dot ray exists.
    // When path A is expected to FIRE, the projection fake has to be the real
    // one: the projected-body bound measures the head/feet span on screen, and
    // a constant-point fake would refuse every accept as a 0-px body.
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
    objTargetUnderDot();          // the dot HAS to be on him when its ray fires
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = dotSeesPawn ? w2sTrueProjection : w2sOnCrosshair;
    if (dotSeesPawn)
        g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };  // the dot NAMES him
    else
        g_centreHits = { {0.02f, &g_selfCol} };                       // the dot ray misses
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    return A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
}

static void testCrosshairAim()
{
    // --- X1: the dot's pixel is what the scan measures from ----------------
    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out, /*dotSeesPawn=*/true);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X1 the enemy under the dot's own ray is fired (path A)");
        CHECK(g_triggerDiag.crosshairCx == 800.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X1 the scan reports the dot's pixel as its crosshair (800,450)");
        CHECK(g_triggerDiag.crosshairRayViaGame == 1,
              "X1 the shot ray came from the game's own projection of that pixel");
        CHECK(g_triggerDiag.radiusPx == 40.0f,
              "X1 the zone is the dot: an 80 px dot is a 40 px radius");
        CHECK(g_triggerDiag.projKeepFrac >= 0.0f && g_triggerDiag.projKeepFrac <= 1.0f,
              "X1 ... and the accepted shot is INSIDE his projected body (keep <= 1)");
    }

    // --- X2: the zone IS the dot - shrink the dot, tighten the trigger -----
    resetWorld();
    setCrosshair(0.0f, 0.0f, 10.0f);              // 10 px dot -> 5 px zone
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.radiusPx == 5.0f, "X2 a 10 px dot is a 5 px zone");
        CHECK(b == FireBlock::NoEnemyInZone,
              "X2 the same enemy 20 px off the dot is now OUTSIDE it");
        CHECK(b != FireBlock::Fired,
              "X2 ... so shrinking the dot really does tighten the trigger (the old build fired anyway)");
        CHECK(g_triggerDiag.dotRayDecided,
              "X2 ... and the refusal came from the dot's own ray, not from the zone");
    }

    // --- X3: moving the dot moves the whole scan ---------------------------
    resetWorld();
    setCrosshair(40.0f, -25.0f, 100.0f);          // dot at (840,425), 50 px radius
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 840.0f && g_triggerDiag.crosshairCy == 425.0f,
              "X3 the scan aims through the dot (800+40, 450-25)");
        CHECK(g_triggerDiag.crosshairRayViaGame == 1,
              "X3 ... and the ray is the ray through that pixel");
        // The enemy (820,450) is ~32 px off the moved dot - inside the drawn
        // zone but NOT under the dot's ray. The ray decides, so no fire: this
        // is exactly the over-fire the report complained about.
        CHECK(b == FireBlock::NoEnemyInZone && g_triggerDiag.dotRayDecided,
              "X3 near the dot but NOT under its ray -> NO fire (the over-fire fix)");
    }
    resetWorld();
    setCrosshair(40.0f, -25.0f, 100.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out, /*dotSeesPawn=*/true);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X3 the same dot ON him still fires (the decision follows the dot)");
    }

    // --- X4: a far-away dot stops firing what is under the old centre ------
    resetWorld();
    setCrosshair(200.0f, 0.0f, 80.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X4 with the dot 200 px right the enemy at +20 px is outside it");
        CHECK(b != FireBlock::Fired, "X4 ... so the trigger does NOT fire him");
    }

    // --- X5: there is no way to detach the trigger from the dot ------------
    // The menu option that used to do it is gone: hiding the DRAWING cannot move
    // the trigger back to the screen centre.
    resetWorld();
    setCrosshair(200.0f, 0.0f, 80.0f, /*drawn=*/false);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 1000.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X5 a hidden dot still aims the scan (1000,450) - there is no switch to undo it");
        CHECK(b == FireBlock::NoEnemyInZone,
              "X5 ... and the enemy at +20 px stays unfired");
    }

    // --- X6: nothing can honour the offset -> the centre ray, REPORTED -----
    resetWorld();
    setCrosshair(40.0f, -25.0f, 80.0f);
    fn_TransformGetRight  = nullptr;
    fn_TransformGetUp     = nullptr;
    fn_ScreenToWorldPoint = nullptr;
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairRayViaGame == 0 &&
              g_triggerDiag.crosshairRayFellBack == 0 &&
              g_triggerDiag.crosshairRayPlain == 0,
              "X6 nothing can apply the offset -> NO honest dot ray, not a fake plain one");
        CHECK(b == FireBlock::Fired,
              "X6 the dot's zone still covers the enemy, so the trigger keeps working");
    }

    // --- X7: the game's projection alone is enough (no basis needed) -------
    resetWorld();
    setCrosshair(40.0f, -25.0f, 80.0f);
    fn_TransformGetRight  = nullptr;
    fn_TransformGetUp     = nullptr;
    fn_ScreenToWorldPoint = fake_screen_to_world;
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairRayViaGame == 1 && g_triggerDiag.crosshairRayPlain == 0,
              "X7 the game's projection alone carries the dot's offset");
        CHECK(b == FireBlock::NoEnemyInZone && g_triggerDiag.dotRayDecided,
              "X7 ... and the ray's verdict rules: NOT under the dot's ray -> no fire");
    }
    fn_TransformGetRight  = fake_get_right;
    fn_TransformGetUp     = fake_get_up;
    fn_ScreenToWorldPoint = fake_screen_to_world;

    // --- X8: THE BESIDE-THE-ARM REPORT, pinned ------------------------------
    // The dot is 200 px right of centre; the dot ray grazes an UNREGISTERED
    // piece of the enemy's component hierarchy (his "arm", from the trigger's
    // point of view) a few centimetres off his silhouette - the wall-collider
    // in the fake, because that is exactly what an unregistered near-arm hit
    // looks like. The parent walk reaches his root transform, so the ray does
    // "name" him; but his projected body sits ~190 px LEFT of the dot, so the
    // projected-body bound refuses: the dot is BESIDE the arm, not on him.
    // This is the exact over-fire the report describes.
    resetWorld();
    // 40 m out: his projected body is ~33 px tall / ~9 px wide, so a dot a
    // couple hundred px sideways is BESIDE the arm, not inside the silhouette.
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    setCrosshair(200.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };  // the ray "names" him
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X8 aimed BESIDE the enemy's arm (200 px off his body) -> no fire");
        CHECK(g_triggerDiag.dotRayDeclined >= 1,
              "X8 ... and the panel counts it as a projected-body refusal");
        CHECK(out.pawn == nullptr, "X8 no target escapes the bound");
    }
    // --- X9: THE REPORT, pinned at CLOSE RANGE ------------------------------
    // The screenshot case: an enemy a few metres away who is NOT under the dot,
    // with the dot ray grazing an unregistered piece of his hierarchy (his
    // outstretched arm / weapon, the collider the fake world uses for it) so the
    // parent walk DOES reach him. The bound that used to run here was HALF HIS
    // BODY HEIGHT used as a radius around his chest - at 3.3 m that is ~200 px,
    // so a dot 118 px to his side passed and the trigger fired beside his arm.
    // The body's own WIDTH is the bound now, and 118 px is not inside it.
    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };   // the ray "names" him
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);                     // 3.3 m, 118 px to the RIGHT of the dot
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X9 close range, dot 118 px beside his body: NO fire (the report)");
        CHECK(g_triggerDiag.dotRayDeclined >= 1,
              "X9 ... counted as a projected-body refusal");
        CHECK(out.pawn == nullptr, "X9 no target escapes");
    }

    // --- X9b: the same 40 px offset is ON him at 3 m and BESIDE him at 40 m --
    // The bound is physical (px per metre comes from his own projected body), so
    // it tightens with distance instead of being a fixed pixel slack.
    resetWorld();
    objTargetUnderDot(3.3f);
    setCrosshair(40.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X9b 40 px off a body 3 m away is still ON him -> fires");
    }
    resetWorld();
    objTargetUnderDot(40.0f);
    setCrosshair(40.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone && out.pawn == nullptr,
              "X9b the same 40 px at 40 m is 1.4 m to his side -> NO fire");
    }

    // --- X10: an unmeasurable body is not a free pass -----------------------
    // With no head the body axis cannot be projected at all. That used to skip
    // the bound ENTIRELY (the shot fired wherever the ray pointed); now the dot
    // is held to the drawn dot's own radius around his aim point, so a dot far
    // off a body that cannot be measured is refused instead of accepted.
    resetWorld();
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, nullptr);   // no head -> no axis
    setCrosshair(0.0f, 0.0f, 10.0f);                            // 10 px dot -> 5 px radius
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone && out.pawn == nullptr,
              "X10 body unmeasurable AND the dot 88 px off his aim point -> NO fire");
        CHECK(g_triggerDiag.dotRayDeclined >= 1, "X10 ... counted, not silently fired");
    }
    // ... and the fallback is not a blanket refusal: with the same unmeasurable
    // body, a small dot sitting exactly ON his aim point still fires. (The
    // fallback's reference is that aim point, so this is the case it accepts.)
    resetWorld();
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, nullptr);
    objTargetUnderDot();
    setCrosshair(0.0f, 130.0f, 10.0f);   // dot at (800,580) = his aim point +0 px
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X10 ... while a small dot ON his aim point still fires");
    }

    restoreHonestDotRay();
    g_w2sImpl = nullptr;

    // Leave the config as the rest of the suite expects it.
    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    Config.Crosshair.Enabled = true;
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

// ---------------------------------------------------------------------------
// 9. The two options that crashed the client: No Sprint Fire Delay, ADS Only
//
// Both called back into the game from a place no guard on this side could cover:
//
//   * the hooked Weapon.get_DelaySprintFire body resolved the local pawn and
//     called Pawn.get_CurrentWeapon() from INSIDE that getter, on a pawn that may
//     already have been destroyed (its managed wrapper stays readable, so
//     Tools::IsPtrValid passes and the native call walks the cleared m_CachedPtr);
//   * the "ADS only" gate called Pawn.IsAiming() 0x519BB10 after validating the
//     pawn with Tools::IsPtrValid() alone - which passes for a pawn the game has
//     already Destroyed, whose managed wrapper stays readable while
//     Object.m_CachedPtr is cleared. The GUARD was the bug, not the symbol.
//
// The sprint-delay gate is PURE now: one pointer compare against a cache
// refreshed from the local pawn's own tick. Pawn.IsAiming() IS called again (it
// is the game's own scoping flag and the .57 dump's answer), but only through
// A_FirePawnCallable(), and the ADS gate has a second, independent source plus a
// fail-open verdict when neither can answer. Every fake below COUNTS, and the
// counters must stay at zero where it matters - that is what proves the crash is
// gone rather than moved.
// ---------------------------------------------------------------------------
static int g_isAimingCalls      = 0;   // the api15 macro path - must stay 0
static int g_currentWeaponCalls = 0;   // Pawn::get_CurrentWeapon - only our tick may call it

// Pawn::IsAiming through the api15 MACRO (Class_Pawn_IsAiming). The triggerbot
// resolves its own pointer and goes through A_FirePawnCallable; this trap answers
// "yes, scoping", so a stray macro call shows up as a shot in a test that expects
// a block.
static bool fake_pawnIsAimingTrap(void *)
{
    g_isAimingCalls++;
    return true;
}

static Weapon *fake_currentWeapon(Pawn *)
{
    g_currentWeaponCalls++;
    return (Weapon *)g_weaponObj.b;
}

// Weapon.m_IsAiming @0xC21 / m_AimingPercentage @0x460. These offsets are NOT
// verified anywhere in this repo, so the gate only ever PRINTS them - which is
// what keeps a wrong offset from turning "ADS only" into "always" or "never".
static void setWeaponAiming(bool flag, float percent)
{
    *(bool  *)(g_weaponObj.b + Offsets::Weapon_m_IsAiming)         = flag;
    *(float *)(g_weaponObj.b + Offsets::Weapon_m_AimingPercentage) = percent;
}

// The world a Fired verdict needs, without running the tree: the mode gate and
// the re-press pacing below both drive A_FireShouldFire / A_FireTick for real.
static void armFiringWorld()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
    objTargetUnderDot();          // path A fires here, so the dot is ON him
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;   // path A fires here: real body projection
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    setCrosshair(0.0f, 0.0f, 80.0f);
}

static int  g_pawnAimingCalls = 0;
static bool g_pawnAimingValue = false;
static int  g_fcAimingCalls   = 0;
static bool g_fcAimingValue   = false;
static bool g_isFiringValue   = false;

static bool fake_pawnAimingCounted(void *)
{
    g_pawnAimingCalls++;
    return g_pawnAimingValue;
}
static bool fake_fireComponentAiming(void *)
{
    g_fcAimingCalls++;
    return g_fcAimingValue;
}
static void *fake_currentFireComponent(void *weapon)
{
    return weapon == (void *)g_weaponObj.b ? (void *)g_weaponObj.b : nullptr;
}
static bool fake_pawnIsFiring(void *) { return g_isFiringValue; }

static void installAdsFakes()
{
    fn_PawnIsAiming                  = fake_pawnAimingCounted;
    fn_WeaponGetCurrentFireComponent = fake_currentFireComponent;
    fn_FireComponentGetIsAiming      = fake_fireComponentAiming;
}

// The same crosshair setup the scan tests use, driven through the whole decision
// tree (A_FireShouldFire) so the mode gate really runs.
static FireBlock runTreeOnCrosshairSetup(TriggerTarget &out)
{
    // The dot's ray NAMES the enemy (path A fires): the mode gate under test
    // sits in front of the scan, so the scan's Fired verdict has to be
    // reachable at all.
    armFiringWorld();
    return A_FireShouldFire(out);
}

static void testCrashFixes()
{
    api12 = (uintptr_t)&fake_localPawnCounted;   // GamePlay::get_LocalPawn
    api15 = (uintptr_t)&fake_pawnIsAimingTrap;   // the api15 macro path - never taken
    api22 = (uintptr_t)&fake_currentWeapon;      // Pawn::get_CurrentWeapon
    Config.NewFeaturesMenu.Triggerbot = false;

    // --- A0: a destroyed weapon is refused before any ADS call --------------
    resetWorld();
    *(void **)(g_weaponObj.b + 0x10) = nullptr;    // m_CachedPtr cleared = Destroy()d
    Config.NewFeaturesMenu.TriggerbotMode = 1;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        const FireBlock b = runTreeOnCrosshairSetup(out);
        CHECK(b == FireBlock::NoWeapon,
              "A0 a destroyed weapon is refused by the liveness check");
        CHECK(g_pawnAimingCalls == 0 && g_fcAimingCalls == 0,
              "A0 ... and neither ADS source is asked about it");
    }
    objInit(g_weaponObj);                          // the rest of the suite needs it back

    // --- B1: the sprint-delay gate is pure ----------------------------------
    // This is the function the hooked Weapon.get_DelaySprintFire calls. The two
    // counters below are native game calls: the hook body must make NONE.
    resetWorld();
    api12 = (uintptr_t)&fake_localPawnCounted;
    api22 = (uintptr_t)&fake_currentWeapon;
    Config.NewFeaturesMenu.NoSprintToFireDelay = true;
    g_noSprintFireDelayWeapon.store((void *)g_weaponObj.b, std::memory_order_relaxed);
    {
        g_localPawnLookups = 0;
        g_currentWeaponCalls = 0;
        for (int i = 0; i < 200; i++)
            (void)NoSprintFireDelayActive(g_weaponObj.b);
        CHECK(g_localPawnLookups == 0 && g_currentWeaponCalls == 0,
              "B1 200 hooked-getter bodies make ZERO native calls (the sprint-delay crash)");
        CHECK(NoSprintFireDelayActive(g_weaponObj.b),
              "B1 ... and still recognise the local player's own weapon");
    }

    // --- B2: only THAT weapon, and only while the option is on -------------
    CHECK(!NoSprintFireDelayActive((void *)g_otherPawn.b),
          "B2 a remote player's weapon is never touched");
    CHECK(!NoSprintFireDelayActive(nullptr), "B2 a null instance is refused");
    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    CHECK(!NoSprintFireDelayActive(g_weaponObj.b),
          "B2 with the option off the getter falls through to the game's value");
    Config.NewFeaturesMenu.NoSprintToFireDelay = true;

    // --- B3: the cache is filled from the local pawn's OWN tick ------------
    // A_FirePawnTicked is the game ticking this very pawn - the one moment where
    // "alive with a real weapon" is not a guess, so the native calls are allowed
    // there (and only there).
    g_noSprintFireDelayWeapon.store(nullptr, std::memory_order_relaxed);
    {
        g_currentWeaponCalls = 0;
        A_FirePawnTicked((void *)g_localPawn.b);
        CHECK(g_currentWeaponCalls == 1,
              "B3 our own pawn tick is one native get_CurrentWeapon call");
        CHECK(g_noSprintFireDelayWeapon.load() == (void *)g_weaponObj.b,
              "B3 ... and it caches the held weapon for the hook to compare against");
    }

    // --- B4: switching the option off empties the cache (hook becomes inert) -
    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    A_FirePawnTicked((void *)g_localPawn.b);
    CHECK(g_noSprintFireDelayWeapon.load() == nullptr,
          "B4 with the option off the cache is cleared, so the hook changes nothing");

    // Leave the suite as it was found.
    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    objInit(g_weaponObj);
    resetWorld();
}

// ---------------------------------------------------------------------------
// 10. The "ADS only" gate (TriggerbotMode 1)
//
// The mode used to read two UNVERIFIED Weapon field offsets, and a wrong offset
// is exactly what makes it block every shot - the reported "ADS only does not
// work". The verdict now comes from the game's own getters:
//   Pawn::IsAiming() 0x519BB10 (the .57 dump's "local player is scoping") and
//   WeaponFireComponent_Instant::get_IsAiming() 0xC9AF4E0 on the held weapon's
//   current fire component - each called ONLY on an object that passed the
//   m_CachedPtr liveness check (the guard the crashed version was missing), and
//   when NEITHER can answer the mode fires anyway and the panel says so.
// ---------------------------------------------------------------------------
static void testAdsGate()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api15 = (uintptr_t)&fake_pawnIsAimingTrap;
    api22 = (uintptr_t)&fake_currentWeapon;
    Config.NewFeaturesMenu.Triggerbot = false;

    // --- B1: neither source is scoping -> the mode blocks the shot ----------
    resetWorld();
    installAdsFakes();
    setWeaponAiming(false, 0.0f);
    Config.NewFeaturesMenu.TriggerbotMode = 1;
    g_pawnAimingCalls = g_fcAimingCalls = g_isAimingCalls = 0;
    g_pawnAimingValue = g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::ModeNotAds,
              "B1 ADS Only: neither source is scoping -> the shot is blocked");
        CHECK(g_pawnAimingCalls == 1 && g_fcAimingCalls == 1,
              "B1 both ADS sources were asked, exactly once");
        CHECK(g_isAimingCalls == 0,
              "B1 ... through the resolved pointer, never the api15 macro (a trap that says 'scoping')");
        CHECK(g_triggerDiag.adsPawnOk && g_triggerDiag.adsFcOk &&
              !g_triggerDiag.adsPawnAim && !g_triggerDiag.adsFcAim &&
              !g_triggerDiag.adsUnknown,
              "B1 the panel reports who answered and what they said");
    }

    // --- B2: the pawn's own flag (the .57-dump source) fires ----------------
    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = true; g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B2 Pawn.IsAiming 0x519BB10 = scoped -> the shot goes out");
        CHECK(g_triggerDiag.adsPawnAim && !g_triggerDiag.adsFcAim,
              "B2 ... and the panel credits the pawn flag");
    }

    // --- B3: the fire component alone is enough ----------------------------
    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = false; g_fcAimingValue = true;
    g_pawnAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B3 the fire component's get_IsAiming 0xC9AF4E0 = scoped -> fires");
        CHECK(g_pawnAimingCalls == 1, "B3 ... the pawn is still asked (it is the primary)");
        CHECK(g_triggerDiag.adsFcAim && !g_triggerDiag.adsPawnAim,
              "B3 ... and the fire component is credited");
    }

    // --- B4: NO usable source -> FAIL OPEN (never block every shot) --------
    resetWorld();
    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    g_pawnAimingValue = g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B4 ADS Only with no usable ADS signal FAILS OPEN (was: every shot blocked)");
        CHECK(g_triggerDiag.adsUnknown,
              "B4 ... and the panel says the signal is missing");
    }
    installAdsFakes();

    // --- B5: a pawn the game reports as dead is never asked ----------------
    {
        objSetAlive(g_localPawn, false);
        g_pawnAimingCalls = 0; g_fcAimingValue = false;
        const AdsRead ads = A_FireAdsGate((void *)g_weaponObj.b, (Pawn *)g_localPawn.b);
        CHECK(g_pawnAimingCalls == 0 && !ads.pawnOk,
              "B5 a pawn whose alive flag is clear is NEVER handed to the native getter");
        objSetAlive(g_localPawn, true);
    }

    // --- B6: the actual crash class - a Destroy()ed native object ----------
    {
        *(void **)(g_localPawn.b + 0x10) = nullptr;   // Object.m_CachedPtr cleared
        g_pawnAimingCalls = 0;
        const AdsRead ads = A_FireAdsGate((void *)g_weaponObj.b, (Pawn *)g_localPawn.b);
        CHECK(g_pawnAimingCalls == 0 && !ads.pawnOk,
              "B6 a Destroy()ed pawn (m_CachedPtr cleared) is never passed to the game");
        objInit(g_localPawn);
    }

    // --- B7: the raw field offsets are display-only ------------------------
    // Whatever those unverified offsets hold must not decide a shot: here they
    // claim "aiming" while BOTH real sources say "not scoped".
    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = g_fcAimingValue = false;
    setWeaponAiming(true, 7.5f);
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::ModeNotAds,
              "B7 garbage in the unverified field offsets cannot make the mode 'Always'");
        CHECK(g_triggerDiag.adsFlag == 1 && g_triggerDiag.adsPct == 7.5f,
              "B7 ... but the panel still reports them for the record");
    }

    // --- B8: "Always" asks neither source ----------------------------------
    resetWorld();
    installAdsFakes();
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B8 mode Always fires with a hipfiring weapon");
        CHECK(g_pawnAimingCalls == 0 && g_fcAimingCalls == 0,
              "B8 ... and asks neither ADS source");
    }

    // --- B9: "Firing only" is untouched by the ADS gate --------------------
    resetWorld();
    installAdsFakes();
    Config.NewFeaturesMenu.TriggerbotMode = 2;
    fn_PawnGetIsFiring = fake_pawnIsFiring;
    g_isFiringValue = true;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B9 mode Firing only uses Pawn.get_IsFiring, not the ADS gate");
        CHECK(g_pawnAimingCalls == 0 && g_fcAimingCalls == 0,
              "B9 ... so neither ADS source is asked in that mode");
    }
    g_isFiringValue = false;
    fn_PawnGetIsFiring = nullptr;

    objInit(g_weaponObj);
    setWeaponAiming(false, 0.0f);
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    Config.NewFeaturesMenu.Triggerbot = false;
    resetWorld();
}

// ---------------------------------------------------------------------------
// 11. Re-press pacing - an edge-press weapon must not be pressed every scan
//
// The old path re-pressed on the very next scan: one StartFire per frame, ~250
// a second, into the weapon's own state machine for as long as a target stayed
// under the crosshair. A repeat press now waits for the weapon's own cadence -
// ended early by Weapon.CanFire() the moment the gun is ready - while the FIRST
// shot on a target is never delayed at all.
// ---------------------------------------------------------------------------
static int   g_startFireCalls = 0;
static int   g_stopFireCalls  = 0;
static bool  g_canFireValue   = true;
static float g_realInterval   = 0.20f;

static void  fake_startFire(void *)       { g_startFireCalls++; }
static void  fake_stopFire(void *, bool)  { g_stopFireCalls++; }
static bool  fake_canFire(void *)         { return g_canFireValue; }
static float fake_realInterval(void *)    { return g_realInterval; }
static bool  fake_autoFire(void *)        { return true; }

// A_FireTick drops everything inside 4 ms of the previous scan, and the local
// pawn / held weapon are cached for 50 ms: open that window between the calls
// below instead of sleeping through it.
static void openTickWindow()
{
    g_afireLastEvalTime = -1000.0;
    g_afireInTick       = false;
}

static void testRepressPacing()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api22 = (uintptr_t)&fake_currentWeapon;
    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    fn_PawnGetIsFiring               = nullptr;
    fn_WeaponIsAutoFire              = nullptr;      // unresolved -> edge press
    fn_WeaponRealFireInterval        = fake_realInterval;
    fn_WeaponCanFire                 = fake_canFire;
    fn_Weapon_StartFire              = fake_startFire;
    fn_Weapon_StopFire               = fake_stopFire;
    Config.NewFeaturesMenu.Triggerbot = true;
    Config.NewFeaturesMenu.TriggerbotMode = 0;       // Always: keep the mode gate out
    g_realInterval = 0.20f;                          // -> pace = 0.18 s
    g_canFireValue = false;                          // the gun is still cycling

    resetWorld();
    armFiringWorld();
    resetLocalPawnCache();
    g_triggerDiag = TriggerbotDiag{};
    g_startFireCalls = g_stopFireCalls = 0;
    g_afireLastPressTime   = -1000.0;
    g_afireLastPressWeapon = nullptr;

    // --- P0: the pure helper's clamps --------------------------------------
    CHECK(std::fabs(A_FireRepressPaceSeconds(0.20f) - 0.18f) < 1e-5f,
          "P0 pace = 90% of the weapon's own interval");
    CHECK(A_FireRepressPaceSeconds(0.0001f) == A_FireTune::kMinFireInterval,
          "P0 a degenerate interval falls back to the pacing floor");
    CHECK(A_FireRepressPaceSeconds(9.0f) == A_FireTune::kMaxRepressPace,
          "P0 a huge interval is capped, so a repeat press never waits too long");

    // --- P1: the FIRST shot is never paced ---------------------------------
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1, "P1 the first press goes out with no delay");
    CHECK(g_triggerDiag.fires == 1, "P1 ... and the panel counts it");

    // --- P2: the release that arms the next round --------------------------
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_stopFireCalls == 1 && !g_afireHoldingFire,
          "P2 an edge-press weapon is released on the next scan (one press = one shot)");
    CHECK(g_startFireCalls == 1, "P2 ... and nothing was re-pressed yet");

    // --- P3: too soon AND the gun says "not ready" -> no press -------------
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1,
          "P3 a repeat press waits (was: one StartFire per scan, ~250/s)");
    CHECK(g_triggerDiag.repressSkipped >= 1 && g_triggerDiag.paceWaiting,
          "P3 ... and the panel shows the wait");
    CHECK(GetTriggerbotDebugState() == 1,
          "P3 ... as an 'aimed in, waiting on the gun' state");

    // --- P4: Weapon.CanFire() ends the wait immediately --------------------
    g_canFireValue = true;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 2,
          "P4 the moment the game says the gun can fire, the press goes out (no added delay)");
    g_canFireValue = false;

    // --- P5: a CanFire that never answers "ready" cannot silence it --------
    // The pace alone allows the next press - it is bounded, so the trigger can
    // never go quiet, and it is the only thing standing between us and the old
    // press-per-scan spam.
    openTickWindow();
    A_FireTick(nullptr);                                  // release
    g_afireLastPressTime = A_FireNow() - 0.30;            // past the 0.18 s pace
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 3,
          "P5 past the pace a press goes out even with CanFire stuck 'not ready'");

    // --- P6: a NEW target is never paced -----------------------------------
    g_afireLastPressTime   = A_FireNow();                 // as if we had just fired
    g_afireLastPressWeapon = (void *)g_weaponObj.b;
    g_afireTargetPawn      = (void *)g_otherPawn.b;       // ... at somebody else
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 4, "P6 a new target is pressed immediately, never paced");

    // --- P7: an auto weapon is held, never released, never re-pressed ------
    A_FireReleaseTrigger(nullptr);                        // clean, released state
    g_afireTargetPawn      = nullptr;
    g_afireLastPressTime   = -1000.0;
    g_afireLastPressWeapon = nullptr;
    fn_WeaponIsAutoFire    = fake_autoFire;
    openTickWindow();
    A_FireTick(nullptr);
    const int autoPresses = g_startFireCalls;
    const int autoStops   = g_stopFireCalls;
    openTickWindow();
    A_FireTick(nullptr);
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == autoPresses && g_stopFireCalls == autoStops,
          "P7 an auto weapon is held: no release and no re-press while the target stays");
    CHECK(g_afireHoldingFire, "P7 ... the trigger stays down");

    // --- P9: a held auto weapon re-asserts when the gun becomes ready again
    // The game clears its own fire request on an auto-reload; without this the
    // burst never resumes until the player taps the button again.
    const int beforeReassert = g_startFireCalls;
    g_canFireValue = false;                       // reloading
    openTickWindow();
    A_FireTick(nullptr);
    g_canFireValue = true;                        // reload finished
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == beforeReassert + 1,
          "P9 a held auto weapon re-presses the moment CanFire turns true (reload recovery)");
    CHECK(g_triggerDiag.reasserts >= 1, "P9 ... and the panel counts the re-assert");

    // --- P10: the backstop, for a gun that stays "ready" while held --------
    g_afireLastPressTime = A_FireNow() - 0.30;    // past kReassertPress
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == beforeReassert + 2,
          "P10 a held trigger also re-asserts on the periodic backstop");

    // --- P8: an unknown fire mode is an EDGE press, not a hold -------------
    // Holding is how a semi-auto whose flags did not resolve fires once and then
    // never again.
    fn_WeaponIsAutoFire = nullptr;
    CHECK(GetWeaponFireMode((void *)g_weaponObj.b) == FireMode::Single,
          "P8 an unrecognised weapon behaves as single shot (edge), never as a hold");

    // Leave the suite as it was found.
    g_afireHoldingFire     = false;
    g_afireFiringOn        = nullptr;
    g_afireTargetPawn      = nullptr;
    g_afireLastPressWeapon = nullptr;
    g_afireLastPressTime   = -1000.0;
    fn_Weapon_StartFire          = nullptr;
    fn_Weapon_StopFire           = nullptr;
    fn_WeaponCanFire             = nullptr;
    fn_WeaponRealFireInterval    = nullptr;
    fn_WeaponIsAutoFire          = nullptr;
    Config.NewFeaturesMenu.Triggerbot = false;
    resetWorld();
}

int main()
{
    installFns();
    resetWorld();

    testGate();
    testArrayGuards();
    testCentreRay();
    testScan();
    testNoTargetFilter();
    testCrosshairAim();
    testTickHookCost();
    testCrashFixes();
    testAdsGate();
    testRepressPacing();

    if (g_failures == 0) {
        printf("\nOK: all LOS gate checks passed\n");
        return 0;
    }
    printf("\n%d check(s) FAILED\n", g_failures);
    return 1;
}
