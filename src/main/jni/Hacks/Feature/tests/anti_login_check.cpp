// Host test for the Anti-Login decision (Hacks/Feature/AntiLogin.h).
//
// Anti-Login has exactly two observable behaviours and this file drives both
// through the module's own entry points - no game, no device:
//
//   * HandleLoginFailResult / hook_DisplayLoginFailResult
//       - the account-conflict results (GameBase.LoginResult.PeerStopSession
//         100004 - the "0E100004" in the CONNECT popup - and GCloud's 205) are
//         swallowed: the game's own DisplayLoginFailResult is never called, so
//         the popup and its QUIT GAME button are never built.
//       - every other login failure is forwarded unchanged (args included).
//       - the switch off = the old behaviour, popup and all.
//   * ServicePendingRelogin
//       - fires the game's CheckAutoLogin once, late enough for the failed flow
//         to unwind, and only while Re-claim is on.
//       - is rate limited, and clears its pending request so one conflict can
//         never turn into a re-login loop.
//
// Host test, no NDK needed (the Android/JNI headers are the stubs in
// Skin/tests/stubs, __fastcall is elided like the real build, and
// Tools::IsPtrValid is stubbed because Tools.cpp is not linked). One line:
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' -I
//   src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui -I
//   src/main/jni/Includes src/main/jni/Hacks/Feature/tests/anti_login_check.cpp
//   -o /tmp/anti_login_check && /tmp/anti_login_check
// Exits 0 and prints "OK: all anti-login checks passed" on success.
// Android.mk does not wildcard Hacks/Feature/tests, so this file never reaches
// the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>
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
#include "Hacks/Feature/Init_Hacks.h"   // sConfig Config - referenced by the module
#include "Hacks/Feature/NewFeatures.h"
#include "Hacks/Feature/AntiLogin.h"

// ---------------------------------------------------------------------------
// Linker stubs: Tools.cpp is intentionally not linked, and the module calls
// Tools::IsPtrValid on the controller it is about to hand back to the game.
// ---------------------------------------------------------------------------
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

// __android_log_print is what LOGE() expands to (pulled in by NewFeatures.h).
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

// Substrate's hook entry (pulled in by Includes/Macros.h). Install() is never
// called from this test, so nothing here patches a single byte.
void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

using namespace anti_login;

static int g_failures = 0;
static int g_checks   = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);         \
                       g_failures++; }                                         \
        else         { printf("ok  : %s\n", msg); }                            \
    } while (0)

// ---- recorded "game" side -------------------------------------------------
struct ForwardedCall { void *self; int result; int innerResult; };
static std::vector<ForwardedCall> g_forwarded;
static std::vector<void *>        g_reloginCalls;

static void record_orig(void *self, int result, int innerResult)
{
    g_forwarded.push_back({self, result, innerResult});
}

static void record_autoLogin(void *self)
{
    g_reloginCalls.push_back(self);
}

static void resetModule()
{
    g_forwarded.clear();
    g_reloginCalls.clear();
    g_blocked = 0;
    g_reloginStarted = 0;
    g_lastCode = 0;
    g_pendingController = nullptr;
    g_pendingKlass = nullptr;
    g_pendingSince = 0.0;
    g_reloginWindowStart = -1000.0;
    g_reloginInWindow = 0;
    orig_DisplayLoginFailResult = &record_orig;
    fn_CheckAutoLogin = &record_autoLogin;
    // The delay is compared against the real clock; 0 makes the checks
    // deterministic without sleeping for real seconds.
    g_reloginDelaySeconds = 0.0;
    g_reloginMaxPerWindow = 3;
    g_reloginWindowSeconds = 30.0;
    Config.NewFeaturesMenu.AntiLogin = true;
    Config.NewFeaturesMenu.AntiLoginRelogin = true;
}

// The module reads offset 0 of the controller (the il2cpp class pointer) to
// prove the instance is still the same object before calling back into it, so
// the fake has to carry one.
struct FakeController { void *klass; };
static int g_loginKlassA = 0;
static int g_loginKlassB = 0;

int main()
{
    FakeController ctrl{&g_loginKlassA};       // stands in for the UILoginMainController
    void *self = (void *)&ctrl;

    // ---- defaults --------------------------------------------------------
    Config.NewFeaturesMenu.AntiLogin = true;
    Config.NewFeaturesMenu.AntiLoginRelogin = true;
    CHECK(Config.NewFeaturesMenu.AntiLogin == true,
          "Anti-Login is ON by default (the popup is blocked out of the box)");
    CHECK(Config.NewFeaturesMenu.AntiLoginRelogin == true,
          "Re-claim session is ON by default");

    // ---- the two numberings of the same event ---------------------------
    CHECK(IsSessionConflict(kLoginResultPeerStopSession, 0),
          "GameBase.LoginResult.PeerStopSession (100004) is a conflict - it is the 0E100004 popup");
    CHECK(IsSessionConflict(0, kLoginResultPeerStopSession),
          "... and it is recognised when it arrives as the inner result");
    CHECK(IsSessionConflict(kGCloudPeerStopSession, 0),
          "GCloud's 205 (ConnectorErrorCode.PeerStopSession) is a conflict too");
    CHECK(!IsSessionConflict(100007, 0) && !IsSessionConflict(0, 100000) &&
          !IsSessionConflict(700, 0) && !IsSessionConflict(0, 0),
          "every other login failure (token expired, None, KingError) is NOT a conflict");

    // ---- conflict: the popup never opens --------------------------------
    resetModule();
    CHECK(HandleLoginFailResult(self, kLoginResultPeerStopSession, 0) == true,
          "a conflict is swallowed - DisplayLoginFailResult never runs, so no CONNECT popup");
    CHECK(g_blocked.load() == 1, "the blocked counter moved");
    CHECK(g_lastCode.load() == kLoginResultPeerStopSession,
          "the panel can show which code was blocked (0E100004)");
    CHECK(g_forwarded.empty(), "the game's own handler was not called");
    CHECK(g_pendingController == self, "a session re-claim was queued");

    // ---- every other failure is still the game's business ---------------
    resetModule();
    CHECK(HandleLoginFailResult(self, 100007, 0) == false,
          "AccessTokenExpired is forwarded, not swallowed");
    hook_DisplayLoginFailResult(self, 100007, 42);
    CHECK(g_forwarded.size() == 1 && g_forwarded[0].self == self &&
          g_forwarded[0].result == 100007 && g_forwarded[0].innerResult == 42,
          "... through the hook with its arguments intact");
    CHECK(g_blocked.load() == 0, "a normal failure does not move the blocked counter");
    CHECK(g_pendingController == nullptr, "and never queues a re-login");

    // ---- switch off = exactly the old behaviour -------------------------
    resetModule();
    Config.NewFeaturesMenu.AntiLogin = false;
    hook_DisplayLoginFailResult(self, kLoginResultPeerStopSession, 0);
    CHECK(g_forwarded.size() == 1,
          "with Anti-Login off the conflict reaches the game again (popup as before)");
    CHECK(g_blocked.load() == 0 && g_pendingController == nullptr,
          "... and nothing is counted or queued");

    // ---- re-claim: fires once, late enough, and only when asked ---------
    resetModule();
    g_reloginDelaySeconds = 3600.0;
    HandleLoginFailResult(self, kLoginResultPeerStopSession, 0);
    ServicePendingRelogin();
    CHECK(g_reloginCalls.empty(),
          "the re-login waits: the failed login flow has to unwind first");
    g_reloginDelaySeconds = 0.0;
    ServicePendingRelogin();
    CHECK(g_reloginCalls.size() == 1 && g_reloginCalls[0] == self,
          "CheckAutoLogin runs on the login controller after the delay");
    CHECK(g_reloginStarted.load() == 1, "the panel counts the re-login");
    ServicePendingRelogin();
    CHECK(g_reloginCalls.size() == 1,
          "one conflict = one re-login (the request is cleared, no loop)");

    // ---- Re-claim off: popup still blocked, no re-login ------------------
    resetModule();
    Config.NewFeaturesMenu.AntiLoginRelogin = false;
    CHECK(HandleLoginFailResult(self, kLoginResultPeerStopSession, 0) == true,
          "popup still blocked with Re-claim off");
    ServicePendingRelogin();
    CHECK(g_reloginCalls.empty() && g_pendingController == nullptr,
          "... and no auto-login was fired");

    // ---- rate limit ------------------------------------------------------
    resetModule();
    g_reloginMaxPerWindow = 2;
    for (int i = 0; i < 5; i++) {
        HandleLoginFailResult(self, kLoginResultPeerStopSession, 0);
        ServicePendingRelogin();
    }
    CHECK(g_reloginCalls.size() == 2,
          "a client that keeps failing re-logins at most twice per window, not five times");
    CHECK(g_blocked.load() == 5,
          "... while EVERY conflict popup is still suppressed");

    // ---- a conflict with no usable controller ---------------------------
    resetModule();
    HandleLoginFailResult(nullptr, kLoginResultPeerStopSession, 0);
    CHECK(g_blocked.load() == 1,
          "a conflict is still suppressed even with no controller to re-login on");
    ServicePendingRelogin();
    CHECK(g_reloginCalls.empty(), "and no auto-login is attempted on nothing");

    // ---- the controller must still be the same object -------------------
    // A closed login screen leaves the captured pointer pointing at memory the
    // game reuses: calling into it would be the crash this guard exists for.
    resetModule();
    ctrl.klass = &g_loginKlassA;
    HandleLoginFailResult(self, kLoginResultPeerStopSession, 0);
    ctrl.klass = &g_loginKlassB;                // memory reused by another type
    ServicePendingRelogin();
    CHECK(g_reloginCalls.empty(),
          "a controller whose class pointer changed is NOT called (native crash avoided)");
    CHECK(g_blocked.load() == 1, "... and the popup was suppressed anyway");

    resetModule();
    ctrl.klass = &g_loginKlassA;
    HandleLoginFailResult(self, kLoginResultPeerStopSession, 0);
    ctrl.klass = nullptr;                       // freed and zeroed
    ServicePendingRelogin();
    CHECK(g_reloginCalls.empty(), "a controller with no class pointer is not called either");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all anti-login checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d anti-login check(s) FAILED\n", g_failures);
    return 1;
}
