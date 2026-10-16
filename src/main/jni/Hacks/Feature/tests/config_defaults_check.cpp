// Host test for the feature defaults in the real Config object.
//
// The unlock switch (HOME tab, "Mythic Attachments") has to be ON out of the box:
// the whole sConfig global is aggregate-initialised as `sConfig Config{0}`, so a
// member's default only survives if its default member initialiser really is
// applied to the members the {0} does not cover. This test builds the REAL
// Hacks/Feature/Init_Hacks.h and asserts that - plus two pre-existing defaults
// that rely on the same rule, so a regression in the rule itself shows up here.
//
// Host test, no NDK needed (the Android/JNI headers are the stubs in
// Skin/tests/stubs, __fastcall is elided like the real build):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes -I src/main/jni/foxcheats/include \
//       src/main/jni/Hacks/Feature/tests/config_defaults_check.cpp -o /tmp/config_defaults_check \
//       && /tmp/config_defaults_check
// Exits 0 and prints "OK: ..." on success. Android.mk's wildcards do not cover
// Hacks/Feature/tests, so this file never reaches the APK.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/Feature/Init_Hacks.h"

// Macros.h pulls in the Substrate headers; nothing here hooks anything.
extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace; (void)result;
}

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

int main()
{
    CHECK(Config.ExtraMenu.MythicAttachments == true,
          "Mythic Attachments is ON by default (the HOME tab switch)");
    CHECK(Config.ExtraMenu.Kinetic == false && Config.ExtraMenu.Recoil == false,
          "the other ExtraMenu switches still default off");

    // Same aggregate-init rule, pre-existing defaults.
    CHECK(Config.NewFeaturesMenu.TriggerbotRadius == 40,
          "the triggerbot radius default still comes from its initialiser");
    CHECK(Config.NewFeaturesMenu.TriggerbotMode == 0 &&
          Config.NewFeaturesMenu.NoSlideFatigue == false,
          "the NewFeatures switches still default off");
    CHECK(Config.Crosshair.Size == 6.0f && Config.Crosshair.Enabled == true,
          "the crosshair defaults still come from their initialisers");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all config-default checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
