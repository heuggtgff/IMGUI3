// Host test for the custom dot crosshair - the work behind "our own dot
// crosshair, colour / opacity / size / x / y adjustable, at the centre by
// default, with the triggerbot connected to it".
//
// It drives the REAL geometry the feature uses:
//   * CrosshairDotCentrePx()  - where the dot is drawn,
//   * CrosshairAimCentrePx()  - where the triggerbot measures its zone from,
//   * CrosshairAimOffsetPx()  - the offsets the trigger actually applies,
//   * CrosshairRayDir()       - the centre ray through the crosshair pixel,
// plus the DEFAULTS a fresh sConfig starts with, which is what "must be at the
// centre by default" means in code.
//
// The screen is a fake 1600x900 and the camera a fixed orthonormal basis
// (forward +Z, right +X, up +Y), so every expected number below is computable
// by hand. Camera.get_fieldOfView does not resolve here, so A_FireFocalPx()
// falls back to A_FireTune::kFallbackFov - the focal length the test uses for
// its own inverse projection is derived from that constant and the screen
// height, never from the code under test.
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

static int fake_screen_w() { return (int)SCREEN_W; }
static int fake_screen_h() { return (int)SCREEN_H; }
static int fake_screen_h_zero() { return 0; }   // degenerate focal length
static Vector3 fake_get_forward(void *) { return CAM_FORWARD; }
static Vector3 fake_get_right(void *)   { return CAM_RIGHT; }
static Vector3 fake_get_up(void *)      { return CAM_UP; }

static void installFns()
{
    m_unity = 0;
    api4 = (uintptr_t)&fake_screen_w;
    api5 = (uintptr_t)&fake_screen_h;

    fn_TransformGetForward = fake_get_forward;
    fn_TransformGetRight   = fake_get_right;
    fn_TransformGetUp      = fake_get_up;
    // No camera / no fieldOfView symbol: A_FireFocalPx() then uses
    // A_FireTune::kFallbackFov, which is the value the expected focal length
    // below is built from.
    fn_CameraGetFov  = nullptr;
    fn_CameraGetMain = nullptr;
}

// The focal length in px the ray must be built with, derived here from the
// screen height and the FOV constant - NOT by calling A_FireFocalPx().
static float ExpectedFocalPx()
{
    return (SCREEN_H * 0.5f) / tanf(A_FireTune::kFallbackFov * 0.5f * A_FireTune::kDegToRad);
}

static bool Nearly(float a, float b, float eps = 1.0e-5f) { return fabsf(a - b) <= eps; }
static bool SameVec(const Vector3 &a, const Vector3 &b, float eps = 1.0e-6f)
{
    return Nearly(a.x, b.x, eps) && Nearly(a.y, b.y, eps) && Nearly(a.z, b.z, eps);
}

static void resetCrosshair()
{
    Config.Crosshair.Enabled = true;
    Config.Crosshair.TriggerbotUsesIt = true;
    Config.Crosshair.Color[0] = 1.0f;
    Config.Crosshair.Color[1] = 1.0f;
    Config.Crosshair.Color[2] = 1.0f;
    Config.Crosshair.Color[3] = 1.0f;
    Config.Crosshair.Size = 6.0f;
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
}

// ---------------------------------------------------------------------------
// 1. The defaults a fresh sConfig starts with ("at the centre by default")
// ---------------------------------------------------------------------------
static void testDefaults()
{
    // Nothing in this test has touched Config yet: this is the same zero-init
    // the APK does with `sConfig Config{0}`, so these are the shipping defaults.
    CHECK(Config.Crosshair.Enabled == false, "D1 the dot starts OFF (no overlay until it is asked for)");
    CHECK(Config.Crosshair.TriggerbotUsesIt == true,
          "D1 the triggerbot is wired to the crosshair by default");
    CHECK(Config.Crosshair.OffsetX == 0.0f && Config.Crosshair.OffsetY == 0.0f,
          "D1 both offsets start at 0 px");
    CHECK(Config.Crosshair.Size == 6.0f, "D1 the dot starts at 6 px");
    CHECK(Config.Crosshair.Color[0] == 1.0f && Config.Crosshair.Color[1] == 1.0f &&
          Config.Crosshair.Color[2] == 1.0f && Config.Crosshair.Color[3] == 0.95f,
          "D1 the dot starts white at 0.95 opacity");

    float cx = -1.0f, cy = -1.0f;
    CrosshairDotCentrePx(cx, cy);
    CHECK(cx == SCREEN_W * 0.5f && cy == SCREEN_H * 0.5f,
          "D2 at the defaults the dot is EXACTLY the render centre");

    CrosshairAimCentrePx(cx, cy);
    CHECK(cx == SCREEN_W * 0.5f && cy == SCREEN_H * 0.5f,
          "D2 ... and so is the point the triggerbot aims through");
}

// ---------------------------------------------------------------------------
// 2. Offsets move the drawn dot and the aim point together
// ---------------------------------------------------------------------------
static void testCentres()
{
    resetCrosshair();
    Config.Crosshair.OffsetX = 123.0f;
    Config.Crosshair.OffsetY = -45.0f;

    float dx = 0.0f, dy = 0.0f;
    CrosshairDotCentrePx(dx, dy);
    CHECK(dx == SCREEN_W * 0.5f + 123.0f && dy == SCREEN_H * 0.5f - 45.0f,
          "O1 the dot is drawn at the centre plus the offsets");

    float ax = 0.0f, ay = 0.0f;
    CrosshairAimCentrePx(ax, ay);
    CHECK(ax == SCREEN_W * 0.5f + 123.0f && ay == SCREEN_H * 0.5f - 45.0f,
          "O1 the triggerbot measures from the same point (it is connected)");

    // Unwiring keeps the dot where it is drawn but returns the trigger to the
    // screen centre, which is exactly what the option promises.
    Config.Crosshair.TriggerbotUsesIt = false;
    CrosshairAimCentrePx(ax, ay);
    CHECK(ax == SCREEN_W * 0.5f && ay == SCREEN_H * 0.5f,
          "O2 unwired: the triggerbot is back on the exact screen centre");
    CrosshairDotCentrePx(dx, dy);
    CHECK(dx == SCREEN_W * 0.5f + 123.0f && dy == SCREEN_H * 0.5f - 45.0f,
          "O2 unwired: the dot itself stays where the sliders put it");

    float offX = -99.0f, offY = -99.0f;
    CrosshairAimOffsetPx(offX, offY);
    CHECK(offX == 0.0f && offY == 0.0f, "O2 unwired: the ray offsets are zeroed too");
    Config.Crosshair.TriggerbotUsesIt = true;
    CrosshairAimOffsetPx(offX, offY);
    CHECK(offX == 123.0f && offY == -45.0f, "O2 wired: the offsets reach the ray");
}

// ---------------------------------------------------------------------------
// 3. The centre ray through the crosshair pixel
// ---------------------------------------------------------------------------
static void testRay()
{
    resetCrosshair();

    // --- R0: at the defaults the ray is the camera forward, untouched ------
    {
        Vector3 dir{1.0f, 2.0f, 3.0f};
        bool used = true, dropped = true;
        CrosshairRayDir(nullptr, 0.0f, 0.0f, dir, used, dropped);
        CHECK(SameVec(dir, CAM_FORWARD) && !used && !dropped,
              "R0 zero offset -> the camera forward, unchanged (used=0 dropped=0)");
    }

    // --- R1: unit length for any offset -----------------------------------
    {
        Vector3 dir;
        bool used = false, dropped = false;
        CrosshairRayDir(nullptr, 100.0f, -220.0f, dir, used, dropped);
        CHECK(Nearly(Vector3::Magnitude(dir), 1.0f, 1.0e-4f) && used && !dropped,
              "R1 the built ray is a unit direction and reports it was applied");
    }

    // --- R2: sign conventions ---------------------------------------------
    {
        Vector3 dir;
        bool used = false, dropped = false;
        CrosshairRayDir(nullptr, 90.0f, 0.0f, dir, used, dropped);
        CHECK(Vector3::Dot(dir, CAM_RIGHT) > 0.0f,
              "R2 a dot right of centre tilts the ray to the camera's right");

        CrosshairRayDir(nullptr, -90.0f, 0.0f, dir, used, dropped);
        CHECK(Vector3::Dot(dir, CAM_RIGHT) < 0.0f,
              "R2 ... and left of centre tilts it left");

        CrosshairRayDir(nullptr, 0.0f, 90.0f, dir, used, dropped);
        CHECK(Vector3::Dot(dir, CAM_UP) < 0.0f,
              "R2 a dot BELOW centre tilts the ray down (screen y grows downward)");

        CrosshairRayDir(nullptr, 0.0f, -90.0f, dir, used, dropped);
        CHECK(Vector3::Dot(dir, CAM_UP) > 0.0f, "R2 ... and above centre tilts it up");
    }

    // --- R3: the ray really passes through the crosshair pixel ------------
    // Projected back with the INDEPENDENTLY derived focal length, the direction
    // has to land on the pixel the offsets asked for. This is the inverse of
    // the forward mapping, so it catches a wrong sign or a wrong scale.
    {
        const float focal = ExpectedFocalPx();
        CHECK(focal > 100.0f && focal < 2000.0f, "R3 the expected focal length is sane");
        const float probes[][2] = {
            { 45.0f,    0.0f }, {  0.0f,  45.0f }, { -60.0f,   25.0f },
            { 200.0f, -150.0f }, { -1.0f,   1.0f }, { 400.0f,  260.0f },
        };
        bool allOk = true;
        for (const auto &p : probes) {
            Vector3 dir;
            bool used = false, dropped = false;
            CrosshairRayDir(nullptr, p[0], p[1], dir, used, dropped);
            const float f = Vector3::Dot(dir, CAM_FORWARD);
            if (!used || dropped || !(f > 0.0f)) { allOk = false; break; }
            const float px = focal * Vector3::Dot(dir, CAM_RIGHT) / f;
            const float py = -focal * Vector3::Dot(dir, CAM_UP) / f;
            if (!Nearly(px, p[0], 0.02f) || !Nearly(py, p[1], 0.02f)) { allOk = false; break; }
        }
        CHECK(allOk, "R3 projecting the ray back lands on the requested pixel (6 probes)");
    }

    // --- R4: a missing camera basis is reported, never guessed -------------
    {
        fn_TransformGetRight = nullptr;
        Vector3 dir{9.0f, 9.0f, 9.0f};
        bool used = true, dropped = false;
        CrosshairRayDir(nullptr, 60.0f, 0.0f, dir, used, dropped);
        CHECK(!used && dropped && SameVec(dir, CAM_FORWARD),
              "R4 offset asked, basis missing -> the plain centre ray + dropped=1");
        fn_TransformGetRight = fake_get_right;

        fn_TransformGetUp = nullptr;
        CrosshairRayDir(nullptr, 0.0f, 60.0f, dir, used, dropped);
        CHECK(!used && dropped, "R4 the same holds when only get_up is missing");
        fn_TransformGetUp = fake_get_up;

        // Without an offset the basis is not needed at all: the default path
        // keeps working even if neither symbol resolved.
        fn_TransformGetRight = nullptr;
        fn_TransformGetUp = nullptr;
        CrosshairRayDir(nullptr, 0.0f, 0.0f, dir, used, dropped);
        CHECK(!used && !dropped && SameVec(dir, CAM_FORWARD),
              "R4 zero offset never touches the basis (default case stays intact)");

        // Sanity: the stub really can go missing.
        CHECK(fn_TransformGetRight == nullptr,
              "R4 control: the test really did unset the symbol");
        fn_TransformGetRight = fake_get_right;
        fn_TransformGetUp = fake_get_up;
    }

    // --- R5: a degenerate focal length is refused, not divided by ---------
    {
        // A 0-height screen makes A_FireFocalPx() return <= 1 px, which is the
        // guard's trigger. Driven through the same accessor the code uses.
        api5 = (uintptr_t)&fake_screen_h_zero;

        Vector3 dir{9.0f, 9.0f, 9.0f};
        bool used = true, dropped = false;
        CrosshairRayDir(nullptr, 60.0f, 0.0f, dir, used, dropped);
        CHECK(!used && dropped && SameVec(dir, CAM_FORWARD),
              "R5 a 0-height screen (focal <= 1 px) drops the offset instead of dividing by 0");
        CHECK(get_height() == 0, "R5 control: the fake screen really reports 0 height");

        api5 = (uintptr_t)&fake_screen_h;
        CHECK(get_height() == (int)SCREEN_H, "R5 control: the screen height is restored");
    }
}

// ---------------------------------------------------------------------------
int main()
{
    installFns();

    testDefaults();
    testCentres();
    testRay();

    if (g_failures == 0) {
        printf("\nOK: all crosshair checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("\n%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
