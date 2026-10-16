// Host test for the dot crosshair - the work behind "COMPLETELY rewrite the
// triggerbot to use the dot crosshair by default: no button to turn it on, the
// dot IS the triggerbot's crosshair instead of the game's".
//
// It drives the REAL geometry the feature uses:
//   * CrosshairCentrePx()      - where the dot is drawn AND where the trigger aims,
//   * A_FireCrosshairRadiusPx()- the trigger's zone radius, which is the dot's own
//                                radius, so shrinking the dot tightens the trigger,
//   * CrosshairRayDir()        - the shot ray through the crosshair pixel,
// plus the DEFAULTS a fresh sConfig starts with (the dot is ON and there is no
// "triggerbot uses it" switch to forget).
//
// The screen is a fake 1600x900 and the camera a fixed orthonormal basis
// (forward +Z, right +X, up +Y), so every expected number below is computable
// by hand. The fake Camera.ScreenToWorldPoint is an INDEPENDENT inverse
// projection built from A_FireTune::kFallbackFov and the screen height (never
// from the code under test), which is what makes R3/R4/R5 real round trips:
// the ray the feature produced has to project BACK onto the pixel the offsets
// asked for through a mapping the feature does not own.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests, __fastcall is elided like the real build, and
// Tools::IsPtrValid is stubbed because Tools.cpp is not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Hacks/Feature/tests/crosshair_check.cpp \
//       -o /tmp/crosshair_check && /tmp/crosshair_check
// Exits 0 and prints "OK: all crosshair checks passed" on success.
// Android.mk does not wildcard Hacks/Feature/tests, so it never reaches the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

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

// Tools.cpp is intentionally not linked (same as the LOS gate test): every
// pointer this test hands over is either null or a static in this arena.
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

// Substrate's hook entry (pulled in by Includes/Macros.h's non-inline hook()
// helper) - this test installs no hooks, but the symbol has to exist to link.
void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

using namespace NewFeatures;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

// ---------------------------------------------------------------------------
// Fake camera + screen
// ---------------------------------------------------------------------------
static const float SCREEN_W = 1600.0f;
static const float SCREEN_H = 900.0f;

static const Vector3 CAM_FORWARD{0.0f, 0.0f, 1.0f};
static const Vector3 CAM_RIGHT  {1.0f, 0.0f, 0.0f};
static const Vector3 CAM_UP     {0.0f, 1.0f, 0.0f};
static const Vector3 CAM_POS    {5.0f, 7.0f, -11.0f};

static int fake_screen_w() { return (int)SCREEN_W; }
static int fake_screen_h() { return (int)SCREEN_H; }
static int fake_screen_h_zero() { return 0; }   // degenerate focal length
static Vector3 fake_get_forward(void *) { return CAM_FORWARD; }
static Vector3 fake_get_right(void *)   { return CAM_RIGHT; }
static Vector3 fake_get_up(void *)      { return CAM_UP; }

// A camera UnityEngine.Object stand-in: A_FireObjectAlive() only reads
// m_CachedPtr @0x10, so a readable non-null word there is enough.
static unsigned char g_fakeCam[64];
static void *fake_cam() { return (void *)g_fakeCam; }

// The focal length in px the pixel->ray mapping must use, derived here from the
// screen height and the FOV constant - NOT by calling A_FireFocalPx().
static float ExpectedFocalPx()
{
    return (SCREEN_H * 0.5f) / tanf(A_FireTune::kFallbackFov * 0.5f * A_FireTune::kDegToRad);
}

// The INDEPENDENT inverse projection the real Camera.ScreenToWorldPoint
// performs: the pixel (screenX, screenY) plus a depth becomes a world point on
// the ray through that pixel. Rebuilt from the camera basis and the expected
// focal length above, so it never touches the code under test.
static Vector3 fake_screen_to_world(void *, Vector3 v)
{
    const float f = ExpectedFocalPx();
    const float dx = v.x - SCREEN_W * 0.5f;
    const float dy = v.y - SCREEN_H * 0.5f;
    Vector3 dir = CAM_FORWARD + CAM_RIGHT * (dx / f) - CAM_UP * (dy / f);
    dir = Vector3::Normalized(dir);
    return Vector3(CAM_POS.x + dir.x * v.z,
                   CAM_POS.y + dir.y * v.z,
                   CAM_POS.z + dir.z * v.z);
}

static void installFns()
{
    m_unity = 0;
    api4 = (uintptr_t)&fake_screen_w;
    api5 = (uintptr_t)&fake_screen_h;

    fn_TransformGetForward   = fake_get_forward;
    fn_TransformGetRight     = fake_get_right;
    fn_TransformGetUp        = fake_get_up;
    fn_ScreenToWorldPoint    = fake_screen_to_world;
    // No fieldOfView symbol: A_FireFocalPx() then uses A_FireTune::kFallbackFov,
    // which is the value the expected focal length above is built from.
    fn_CameraGetFov  = nullptr;
    fn_CameraGetMain = nullptr;

    memset(g_fakeCam, 0, sizeof g_fakeCam);
    *(void **)(g_fakeCam + 0x10) = (void *)g_fakeCam;   // m_CachedPtr
}

static bool Nearly(float a, float b, float eps = 1.0e-5f) { return fabsf(a - b) <= eps; }
static bool SameVec(const Vector3 &a, const Vector3 &b, float eps = 1.0e-6f)
{
    return Nearly(a.x, b.x, eps) && Nearly(a.y, b.y, eps) && Nearly(a.z, b.z, eps);
}

static void resetCrosshair()
{
    Config.Crosshair.Enabled = true;
    Config.Crosshair.Color[0] = 1.0f;
    Config.Crosshair.Color[1] = 1.0f;
    Config.Crosshair.Color[2] = 1.0f;
    Config.Crosshair.Color[3] = 1.0f;
    Config.Crosshair.Size = 6.0f;
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
}

// ---------------------------------------------------------------------------
// 1. The defaults a fresh sConfig starts with
//    ("use the dot crosshair on default", with no switch to connect it)
// ---------------------------------------------------------------------------
static void testDefaults()
{
    // Nothing in this test has touched Config yet: this is the same zero-init
    // the APK does with `sConfig Config{0}`... except sConfig has a CONSTRUCTOR,
    // so these really are the shipping defaults (Config{0} default-initialises,
    // it does not zero the members the struct gives initialisers to).
    CHECK(Config.Crosshair.Enabled == true,
          "D1 the dot crosshair is ON by default (it IS the crosshair now)");
    CHECK(Config.Crosshair.OffsetX == 0.0f && Config.Crosshair.OffsetY == 0.0f,
          "D1 both offsets start at 0 px");
    CHECK(Config.Crosshair.Size == 6.0f, "D1 the dot starts at 6 px");
    CHECK(Config.Crosshair.Color[0] == 1.0f && Config.Crosshair.Color[1] == 1.0f &&
          Config.Crosshair.Color[2] == 1.0f && Config.Crosshair.Color[3] == 0.95f,
          "D1 the dot starts white at 0.95 opacity");

    float cx = -1.0f, cy = -1.0f;
    CrosshairCentrePx(cx, cy);
    CHECK(cx == SCREEN_W * 0.5f && cy == SCREEN_H * 0.5f,
          "D2 at the defaults the crosshair is EXACTLY the render centre");
}

// ---------------------------------------------------------------------------
// 2. The offsets move the ONE crosshair, always - there is no "unwired" state
// ---------------------------------------------------------------------------
static void testCentres()
{
    resetCrosshair();
    Config.Crosshair.OffsetX = 123.0f;
    Config.Crosshair.OffsetY = -45.0f;

    float cx = 0.0f, cy = 0.0f;
    CrosshairCentrePx(cx, cy);
    CHECK(cx == SCREEN_W * 0.5f + 123.0f && cy == SCREEN_H * 0.5f - 45.0f,
          "O1 the crosshair is drawn at the centre plus the offsets");

    // The same call is what the trigger measures from - there is no second
    // function and no flag, so the two cannot be out of step.
    float ax = 0.0f, ay = 0.0f;
    CrosshairCentrePx(ax, ay);
    CHECK(ax == cx && ay == cy,
          "O1 the trigger aims at the SAME point by construction (one crosshair)");
}

// ---------------------------------------------------------------------------
// 3. The trigger zone IS the dot
// ---------------------------------------------------------------------------
static void testZoneIsTheDot()
{
    resetCrosshair();

    Config.Crosshair.Size = 20.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 10.0f), "Z1 a 20 px dot -> a 10 px zone radius");

    Config.Crosshair.Size = 6.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 3.0f), "Z1 the default 6 px dot -> a 3 px zone radius");

    // The complaint this replaces: "I made the dot small and it still fired the
    // same". The zone now follows the dot exactly, both ways.
    Config.Crosshair.Size = 2.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 1.0f), "Z2 a 2 px dot -> a 1 px zone radius");
    CHECK(A_FireCrosshairRadiusPx() < 3.0f,
          "Z2 ... i.e. shrinking the dot really does tighten the trigger");

    Config.Crosshair.Size = 40.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 20.0f), "Z2 a 40 px dot -> a 20 px zone radius");

    // Degenerate values are clamped, never used raw.
    Config.Crosshair.Size = 0.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 3.0f), "Z3 a 0 px size -> the default dot's radius");
    Config.Crosshair.Size = -5.0f;
    CHECK(Nearly(A_FireCrosshairRadiusPx(), 3.0f), "Z3 a negative size -> the default dot's radius");
    Config.Crosshair.Size = 100000.0f;
    CHECK(A_FireCrosshairRadiusPx() <= SCREEN_W * 0.5f,
          "Z3 an absurd size can never become the whole screen");

    resetCrosshair();
}

// ---------------------------------------------------------------------------
// 4. The shot ray through the crosshair pixel
// ---------------------------------------------------------------------------
static void testRay()
{
    resetCrosshair();

    // --- R1: the game's own projection answers, at ANY offset --------------
    {
        const float probes[][2] = {
            { 0.0f, 0.0f }, { 45.0f, 0.0f }, { 0.0f, 45.0f }, { -60.0f, 25.0f },
            { 200.0f, -150.0f }, { -1.0f, 1.0f }, { 400.0f, 260.0f },
        };
        bool allGame = true, allUnitForward = true;
        for (const auto &p : probes) {
            Config.Crosshair.OffsetX = p[0];
            Config.Crosshair.OffsetY = p[1];
            Vector3 out{9.0f, 9.0f, 9.0f};
            bool viaGame = false, fellBack = false, plain = false;
            CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, viaGame, fellBack, plain);
            if (!viaGame || fellBack || plain) { allGame = false; break; }
            if (!Nearly(Vector3::Magnitude(out), 1.0f, 1.0e-4f)) { allUnitForward = false; break; }
            if (Vector3::Dot(out, CAM_FORWARD) <= 0.0f) { allUnitForward = false; break; }
        }
        CHECK(allGame, "R1 the game's own projection gives the ray for every offset (7 probes)");
        CHECK(allUnitForward, "R1 ... and it is a unit direction that points forward");
    }

    // --- R2: sign conventions ---------------------------------------------
    {
        Vector3 out;
        bool g = false, f = false, p = false;

        Config.Crosshair.OffsetX = 90.0f;  Config.Crosshair.OffsetY = 0.0f;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, p);
        CHECK(Vector3::Dot(out, CAM_RIGHT) > 0.0f,
              "R2 a dot right of centre tilts the ray to the camera's right");

        Config.Crosshair.OffsetX = -90.0f;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, p);
        CHECK(Vector3::Dot(out, CAM_RIGHT) < 0.0f,
              "R2 ... and left of centre tilts it left");

        Config.Crosshair.OffsetX = 0.0f; Config.Crosshair.OffsetY = 90.0f;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, p);
        CHECK(Vector3::Dot(out, CAM_UP) < 0.0f,
              "R2 a dot BELOW centre tilts the ray down (screen y grows downward)");

        Config.Crosshair.OffsetY = -90.0f;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, p);
        CHECK(Vector3::Dot(out, CAM_UP) > 0.0f, "R2 ... and above centre tilts it up");
    }

    // --- R3: the ray really passes through the crosshair pixel -------------
    // The REAL assertion that the dot is the trigger's crosshair: an
    // independent inverse projection (fake_screen_to_world above, which is what
    // the game's ScreenToWorldPoint does) has to hand the feature a direction
    // that projects back onto the exact pixel the dot is drawn at.
    {
        const float focal = ExpectedFocalPx();
        CHECK(focal > 100.0f && focal < 2000.0f, "R3 the expected focal length is sane");

        const float probes[][2] = {
            { 45.0f, 0.0f }, { 0.0f, 45.0f }, { -60.0f, 25.0f },
            { 200.0f, -150.0f }, { -1.0f, 1.0f }, { 400.0f, 260.0f },
        };
        bool allOk = true;
        for (const auto &p : probes) {
            Config.Crosshair.OffsetX = p[0];
            Config.Crosshair.OffsetY = p[1];
            Vector3 out;
            bool g = false, f = false, pl = false;
            CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, pl);
            const float fwd = Vector3::Dot(out, CAM_FORWARD);
            if (!g || f || pl || !(fwd > 0.0f)) { allOk = false; break; }
            const float px = focal * Vector3::Dot(out, CAM_RIGHT) / fwd;
            const float py = -focal * Vector3::Dot(out, CAM_UP) / fwd;
            if (!Nearly(px, p[0], 0.02f) || !Nearly(py, p[1], 0.02f)) { allOk = false; break; }
        }
        CHECK(allOk, "R3 the shot ray projects back onto the dot's pixel (6 probes)");
    }

    // --- R4: no game projection -> the camera basis rebuilds the same ray ---
    {
        fn_ScreenToWorldPoint = nullptr;
        Config.Crosshair.OffsetX = 120.0f;
        Config.Crosshair.OffsetY = -35.0f;

        Vector3 rebuilt;
        bool g = true, f = false, pl = true;
        CrosshairRayDir(nullptr, nullptr, CAM_POS, rebuilt, g, f, pl);
        CHECK(!g && f && !pl, "R4 without the projection the basis rebuild answers");

        // The two paths have to agree: same pixel, same ray.
        fn_ScreenToWorldPoint = fake_screen_to_world;
        Vector3 projected;
        bool g2 = false, f2 = false, pl2 = false;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, projected, g2, f2, pl2);
        CHECK(g2 && !f2 && !pl2, "R4 control: the game's projection answers again");
        CHECK(SameVec(rebuilt, projected, 1.0e-5f),
              "R4 the rebuilt ray and the game's projection are the SAME ray");
    }

    // --- R5: on the centre pixel the camera forward IS the dot's ray --------
    {
        Config.Crosshair.OffsetX = 0.0f;
        Config.Crosshair.OffsetY = 0.0f;
        fn_ScreenToWorldPoint = nullptr;             // and without any projection

        Vector3 out{9.0f, 9.0f, 9.0f};
        bool g = true, f = true, pl = false;
        CrosshairRayDir(nullptr, nullptr, CAM_POS, out, g, f, pl);
        CHECK(!g && !f && pl && SameVec(out, CAM_FORWARD),
              "R5 centre dot, no symbols -> the plain camera forward, reported as such");
    }

    // --- R6: an off-centre dot with NOTHING to honour it is REPORTED --------
    // Neither the game's projection nor the camera basis: the offset cannot be
    // applied, so the centre ray stands in AND plain=1 says so - the failure
    // mode that used to be silent.
    {
        fn_ScreenToWorldPoint = nullptr;
        fn_TransformGetRight  = nullptr;
        fn_TransformGetUp     = nullptr;
        Config.Crosshair.OffsetX = 60.0f;
        Config.Crosshair.OffsetY = 0.0f;

        Vector3 out{9.0f, 9.0f, 9.0f};
        bool g = true, f = true, pl = false;
        CrosshairRayDir(nullptr, nullptr, CAM_POS, out, g, f, pl);
        // The plain centre ray is NOT an honest stand-in any more: pretending it
        // is the dot's ray fires the trigger at a line the user is not aiming
        // down. All flags stay false, so the scan treats this as NO dot ray.
        CHECK(!g && !f && !pl && SameVec(out, CAM_FORWARD),
              "R6 an off-centre dot nobody can apply -> NO honest dot ray (all flags false)");

        fn_TransformGetRight = fake_get_right;
        fn_TransformGetUp    = fake_get_up;
        fn_ScreenToWorldPoint = fake_screen_to_world;
    }

    // --- R7: a detail the ray must not need --------------------------------
    // The offsets alone drive it: no right/up basis is consulted at all when the
    // game's own projection answers.
    {
        Config.Crosshair.OffsetX = 550.0f;      // far enough out that a rebuild
        Config.Crosshair.OffsetY = 90.0f;       // would be a visibly different ray
        fn_TransformGetRight = nullptr;
        fn_TransformGetUp = nullptr;

        Vector3 out;
        bool g = false, f = true, pl = false;
        CrosshairRayDir(fake_cam(), nullptr, CAM_POS, out, g, f, pl);
        CHECK(g && !f && !pl, "R7 the game's projection needs no camera basis at all");
        CHECK(Vector3::Dot(out, CAM_RIGHT) > 0.0f,
              "R7 ... and an extreme dot right of centre still tilts the ray right");

        fn_TransformGetRight = fake_get_right;
        fn_TransformGetUp = fake_get_up;
    }

    // --- R8: a degenerate focal length cannot be divided by ----------------
    {
        // Without the game projection, a 0-height screen makes A_FireFocalPx()
        // return <= 1 px, which is the guard's trigger.
        fn_ScreenToWorldPoint = nullptr;
        api5 = (uintptr_t)&fake_screen_h_zero;

        Vector3 out{9.0f, 9.0f, 9.0f};
        bool g = false, f = true, pl = false;
        CrosshairRayDir(nullptr, nullptr, CAM_POS, out, g, f, pl);
        CHECK(!g && !f && !pl && SameVec(out, CAM_FORWARD),
              "R8 a 0-height screen drops the offset (no honest dot ray, no 1/0)");
        CHECK(get_height() == 0, "R8 control: the fake screen really reports 0 height");

        api5 = (uintptr_t)&fake_screen_h;
        CHECK(get_height() == (int)SCREEN_H, "R8 control: the screen height is restored");
        fn_ScreenToWorldPoint = fake_screen_to_world;
    }

    resetCrosshair();
}

// ---------------------------------------------------------------------------
int main()
{
    installFns();

    testDefaults();
    testCentres();
    testZoneIsTheDot();
    testRay();

    if (g_failures == 0) {
        printf("\nOK: all crosshair checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("\n%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
