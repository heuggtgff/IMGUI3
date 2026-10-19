#pragma once
//
// ============================================================================
//  AntiLogin.h  -  "Anti-Login": never show the account-conflict popup, and
//                  take the session back instead.
//
//  WHAT THAT POPUP IS
//    When the same account signs in on a second device the server stops THIS
//    client's session, and the client answers with a CONNECT dialog:
//
//      "Account accessed from another location. Please secure your account.
//       (0E100004)"                       [ QUIT GAME ]
//
//    - so the fight over the account always ended with OUR client quitting.
//
//    The dialog is built by UILoginMainController.DisplayLoginFailResult(int
//    result, int innerResult) - dump.cs (GR .57), TypeDefIndex 29077,
//    dump line 1404044:
//
//      DisplayLoginFailResult              RVA 0xB119D50  <- the popup itself
//      OnMessageBoxCallback                RVA 0xB119FE0  <- its QUIT GAME button
//      CheckAutoLogin                      RVA 0xB1112D8  <- the game's own auto-login
//
//    The code the dialog prints comes from LocalizationLoginHelper
//    (TypeDefIndex 28984): GetLoginResultMessage 0xA3C3320 formats it through
//    BuildFakeErrCodeString 0xA3C320C, and 0E100004 is exactly
//    GameBase.LoginResult.PeerStopSession == 100004 - "your session was taken
//    over". GCloud numbers the same event 205
//    (ConnectorErrorCode.PeerStopSession / GCloud.Result.PeerStopSession), so
//    both values are recognised.
//
//  WHAT THIS DOES
//    1. A conflict result never reaches DisplayLoginFailResult's body, so the
//       popup is never built and QUIT GAME never appears. Every other login
//       failure is handed back to the game unchanged.
//    2. Optionally (on by default) it re-runs the game's own auto-login a
//       couple of seconds later, which re-registers THIS device as the
//       account's session. The server then stops the OTHER session, so the
//       popup lands on the device that tried to log in - the intruder.
//
//  HONEST LIMITS
//    The other device's popup is the server's own reaction to losing the
//    session; a client-side mod cannot send anything to that device. If the
//    re-login cannot run (no platform session left, game updated, switch off)
//    the popup is still suppressed - the client just stays where it is instead
//    of quitting. And suppressing the dialog does not re-create the dead
//    session by itself: only the game's own login does that, which is why the
//    re-login exists.
//
//  Every address above was checked against the GR .57 dump.
// ============================================================================
#include <atomic>
#include <chrono>
#include <cstdint>

// Same dependency style as NewFeatures.h: the Android/JNI stubs and the il2cpp
// wrappers come from the include chain (Config is declared by Init_Hacks.h,
// which every user of this header pulls in first).
#include "../../IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "../../Includes/Utils.h"   // getAbsoluteAddress

namespace anti_login
{
    namespace RVA
    {
        // UILoginMainController (TypeDefIndex 29077) - private, non-virtual, so
        // they are hooked by address, never through a slot.
        constexpr uintptr_t UILoginMain_DisplayLoginFailResult = 0xB119D50; // void(int result, int innerResult)
        constexpr uintptr_t UILoginMain_CheckAutoLogin         = 0xB1112D8; // void()
    }

    // GameBase.LoginResult.PeerStopSession - the "0E100004" in the dialog.
    constexpr int kLoginResultPeerStopSession = 100004;
    // GCloud.Result / ConnectorErrorCode.PeerStopSession - same event.
    constexpr int kGCloudPeerStopSession      = 205;

    // The re-login is deliberately late and rate limited: the failed login flow
    // has to unwind first, and a client that keeps failing must not turn the
    // login screen into a loop. Plain variables (not constexpr) so the host test
    // can drive them without sleeping for real seconds.
    inline double g_reloginDelaySeconds  = 2.0;
    inline int    g_reloginMaxPerWindow  = 3;
    inline double g_reloginWindowSeconds = 30.0;

    // ---- what the menu prints --------------------------------------------
    inline std::atomic<int>  g_blocked{0};         // popups suppressed
    inline std::atomic<int>  g_reloginStarted{0};  // auto-logins fired
    inline std::atomic<int>  g_lastCode{0};        // last conflict result seen
    inline std::atomic<bool> g_armed{false};       // the hook is in place

    // ---- hook / call targets ---------------------------------------------
    inline void (*orig_DisplayLoginFailResult)(void *, int, int) = nullptr;
    inline void (*fn_CheckAutoLogin)(void *)                     = nullptr;

    inline double NowSeconds()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Both numberings of "the account was signed in somewhere else". Anything
    // else is a normal login failure and stays the game's business.
    inline bool IsSessionConflict(int result, int innerResult)
    {
        return result == kLoginResultPeerStopSession || result == kGCloudPeerStopSession ||
               innerResult == kLoginResultPeerStopSession || innerResult == kGCloudPeerStopSession;
    }

    // ---- pending re-login, serviced once per rendered frame --------------
    inline void  *g_pendingController  = nullptr;
    inline void  *g_pendingKlass       = nullptr;   // its il2cpp class pointer
    inline double g_pendingSince       = 0.0;
    inline double g_reloginWindowStart = -1000.0;
    inline int    g_reloginInWindow    = 0;

    inline void RequestRelogin(void *controller)
    {
        if (controller == nullptr || !Tools::IsPtrValid(controller)) return;
        // Offset 0 of every il2cpp object is its class pointer. Captured now so
        // ServicePendingRelogin() can prove the instance is still the same
        // object before it hands it back to the game.
        void *klass = *(void **)controller;
        if (!Tools::IsPtrValid(klass)) return;

        const double now = NowSeconds();
        if (now - g_reloginWindowStart > g_reloginWindowSeconds) {
            g_reloginWindowStart = now;
            g_reloginInWindow    = 0;
        }
        if (g_reloginInWindow >= g_reloginMaxPerWindow) return;   // rate limited
        g_reloginInWindow++;
        g_pendingController = controller;
        g_pendingKlass      = klass;
        g_pendingSince      = now;
    }

    // Called every frame from the render loop (Main.cpp). Idle - and therefore
    // free - while nothing is pending, which is all of normal play.
    inline void ServicePendingRelogin()
    {
        void *controller = g_pendingController;
        if (controller == nullptr) return;
        if (NowSeconds() - g_pendingSince < g_reloginDelaySeconds) return;
        g_pendingController = nullptr;

        if (!Config.NewFeaturesMenu.AntiLogin || !Config.NewFeaturesMenu.AntiLoginRelogin) return;
        if (!Tools::IsPtrValid(controller)) return;
        // It must still be the same object: a controller the game already threw
        // away (login screen closed, memory reused by another type) fails this
        // and the call is skipped. Blocking the popup can never fail with it.
        if (*(void **)controller != g_pendingKlass) return;
        if (fn_CheckAutoLogin == nullptr) return;

        fn_CheckAutoLogin(controller);
        g_reloginStarted++;
    }

    // The whole decision, separated from the hook so the host test can drive it.
    // true = this login failure is ours: the popup must not open.
    inline bool HandleLoginFailResult(void *self, int result, int innerResult)
    {
        if (!Config.NewFeaturesMenu.AntiLogin)          return false;
        if (!IsSessionConflict(result, innerResult))    return false;

        g_blocked++;
        g_lastCode = (result != 0) ? result : innerResult;
        if (Config.NewFeaturesMenu.AntiLoginRelogin)
            RequestRelogin(self);
        return true;
    }

    inline void hook_DisplayLoginFailResult(void *self, int result, int innerResult)
    {
        if (HandleLoginFailResult(self, result, innerResult))
            return;                                  // no popup, no QUIT GAME
        if (orig_DisplayLoginFailResult != nullptr)
            orig_DisplayLoginFailResult(self, result, innerResult);
    }

    // One hook, installed once at startup. The switches are read inside the hook,
    // so flipping them in the menu takes effect immediately and never re-patches
    // game code.
    inline void Install()
    {
        void *popup = (void *)getAbsoluteAddress("libunity.so", (DWORD)RVA::UILoginMain_DisplayLoginFailResult);
        if (!Tools::IsPtrValid(popup))
            return;                                  // game updated: stay inert

        Tools::Hook(popup, (void *)&hook_DisplayLoginFailResult,
                    (void **)&orig_DisplayLoginFailResult);
        fn_CheckAutoLogin = (void (*)(void *))getAbsoluteAddress(
            "libunity.so", (DWORD)RVA::UILoginMain_CheckAutoLogin);
        g_armed = true;
    }
}
