// Host test for System/Hooks/MythicAttachments.h - "Mythic Attachments": an
// applied weapon skin counts as really owned, so the weapon's attachments (the
// mythic's own ones included) are not locked.
//
// It drives the REAL hook bodies with fake originals that record every call and
// answer "locked" (false), which is what the game returns for a skin the player
// does not own. What is pinned here:
//
//   * the default: sConfig's own aggregate init leaves MythicAttachments ON and
//     every other ExtraMenu flag off;
//   * ON  -> all four gates answer true and the game's function is NOT called
//     (nothing native runs to produce the answer);
//   * OFF -> every gate returns the original's value, the original is called
//     EXACTLY once per call, and the caller's arguments arrive unchanged
//     (including IsThisAttachmentUnlock's third bool);
//   * a null receiver never gets a fabricated answer - the original decides;
//   * the legacy Config.ExtraMenu.Attachment flag unlocks the attachment half
//     only, never the skin half;
//   * with the originals not installed yet (null), the gates answer false
//     instead of dereferencing a null pointer.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests and Tools::IsPtrValid is stubbed because Tools.cpp is
// not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes \
//       src/main/jni/System/Hooks/tests/mythic_attachments_check.cpp \
//       -o /tmp/mythic_attachments_check && /tmp/mythic_attachments_check
// Exits 0 and prints "OK: all mythic-attachment checks passed" on success.
// Android.mk compiles only ImGui / IL2CppSDKGenerator/IL2Cpp /
// IL2CppSDKGenerator/KittyMemory / System/Texture and the jni root, so nothing
// under System/Hooks is ever compiled into the APK.
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
#include "Hacks/Feature/Init_Hacks.h"      // sConfig Config - used by the header
#include "System/Hooks/MythicAttachments.h"

// Tools::IsPtrValid (Tools.cpp is intentionally not linked).
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

// __android_log_print is what LOGI() expands to; nothing here logs, but the
// symbol has to exist for the TU to link.
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

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

using namespace mythic_attachments;

// ---------------------------------------------------------------------------
// Fake originals: every call is counted, and every argument is recorded so the
// pass-through can be checked argument-for-argument.
// ---------------------------------------------------------------------------
struct Calls
{
    int  attachmentUnlock = 0;
    int  skinUnlock       = 0;
    int  loadoutGate      = 0;
    int  confirmGate      = 0;
    int  lastAttachment   = -1;
    uint32_t lastSkin     = 0;
    uint32_t lastBlueprint = 0;
    uint32_t lastAttachID  = 0;
    bool lastCheckWeapon  = false;
    uint32_t lastWeaponID = 0;
    void reset()
    {
        attachmentUnlock = skinUnlock = loadoutGate = confirmGate = 0;
        lastAttachment = -1; lastSkin = 0; lastBlueprint = 0;
        lastAttachID = 0; lastCheckWeapon = false; lastWeaponID = 0;
    }
};
static Calls g_calls;
static int   g_instance = 0;             // the receiver the gates are called on

static bool fake_origAttachmentUnlock(void *, int attachment)
{
    g_calls.attachmentUnlock++;
    g_calls.lastAttachment = attachment;
    return false;                        // "this attachment is locked"
}
static bool fake_origSkinUnlock(void *, uint32_t skinId)
{
    g_calls.skinUnlock++;
    g_calls.lastSkin = skinId;
    return false;                        // "this skin is not owned"
}
static bool fake_origLoadoutGate(void *, uint32_t blueprintWeaponId, uint32_t attachID,
                                 bool isCheckWeaponUnlock)
{
    g_calls.loadoutGate++;
    g_calls.lastBlueprint   = blueprintWeaponId;
    g_calls.lastAttachID    = attachID;
    g_calls.lastCheckWeapon = isCheckWeaponUnlock;
    return false;
}
static bool fake_origConfirmGate(void *, uint32_t weaponID, uint32_t attachID)
{
    g_calls.confirmGate++;
    g_calls.lastWeaponID = weaponID;
    g_calls.lastAttachID = attachID;
    return false;
}

static void installFakes()
{
    orig_IsWeaponAttachmentUnlock = fake_origAttachmentUnlock;
    orig_IsWeaponSkinUnlock       = fake_origSkinUnlock;
    orig_IsThisAttachmentUnlock   = fake_origLoadoutGate;
    orig_IsAttachUnlock           = fake_origConfirmGate;
    g_calls.reset();
}

static void setFlags(bool mythic, bool legacyAttachment)
{
    Config.ExtraMenu.MythicAttachments = mythic;
    Config.ExtraMenu.Attachment        = legacyAttachment;
}

int main()
{
    // ---- M1: the documented default ---------------------------------------
    {
        sConfig fresh{0};                // exactly what Init_Hacks.h declares
        CHECK(fresh.ExtraMenu.MythicAttachments,
              "M1 a fresh config has Mythic Attachments ON (as-if-owned default)");
        CHECK(!fresh.ExtraMenu.Attachment && !fresh.ExtraMenu.Blueprints,
              "M1 ... and no other unlock flag is silently on");
    }

    installFakes();

    // ---- M2: ON -> all four gates answer YES, natively nothing runs --------
    setFlags(true, false);
    g_calls.reset();
    memset(&g_diag, 0, sizeof(g_diag));
    {
        CHECK(IsWeaponAttachmentUnlock(&g_instance, 0x1234),
              "M2 ON: an attachment counts as unlocked");
        CHECK(IsWeaponSkinUnlock(&g_instance, 0x9999),
              "M2 ON: the applied skin counts as unlocked (as if owned)");
        CHECK(IsThisAttachmentUnlock(&g_instance, 0x555, 0x1234, false),
              "M2 ON: the loadout's own gate lets the attachment through");
        CHECK(IsAttachUnlock(&g_instance, 0x555, 0x1234),
              "M2 ON: the confirm-time validator lets it through");
        CHECK(g_calls.attachmentUnlock == 0 && g_calls.skinUnlock == 0 &&
              g_calls.loadoutGate == 0 && g_calls.confirmGate == 0,
              "M2 ON: no game function is called to produce those answers");
        CHECK(g_diag.asked[GateAttachment] == 1 && g_diag.granted[GateAttachment] == 1 &&
              g_diag.asked[GateSkin] == 1 && g_diag.granted[GateSkin] == 1 &&
              g_diag.asked[GateLoadout] == 1 && g_diag.granted[GateLoadout] == 1 &&
              g_diag.asked[GateConfirm] == 1 && g_diag.granted[GateConfirm] == 1,
              "M2 ON: each gate counts one ask and one grant (the SKINS tab's instrument)");
    }

    // ---- M3: OFF -> the game's own answer, pass-through intact ------------
    setFlags(false, false);
    g_calls.reset();
    memset(&g_diag, 0, sizeof(g_diag));
    {
        CHECK(!IsWeaponAttachmentUnlock(&g_instance, 0x1234),
              "M3 OFF: the game's own attachment verdict is returned");
        CHECK(g_calls.attachmentUnlock == 1 && g_calls.lastAttachment == 0x1234,
              "M3 OFF: called exactly once, with the attachment id intact");
        CHECK(!IsWeaponSkinUnlock(&g_instance, 0x99),
              "M3 OFF: the game's own skin verdict is returned");
        CHECK(g_calls.skinUnlock == 1 && g_calls.lastSkin == 0x99u,
              "M3 OFF: ... called once, with the skin id intact");
        CHECK(!IsThisAttachmentUnlock(&g_instance, 0x555, 0x1234, true),
              "M3 OFF: the loadout verdict is returned");
        CHECK(g_calls.loadoutGate == 1 && g_calls.lastBlueprint == 0x555u &&
              g_calls.lastAttachID == 0x1234u && g_calls.lastCheckWeapon,
              "M3 OFF: ... called once, and the third bool is forwarded unchanged");
        CHECK(!IsAttachUnlock(&g_instance, 0x777, 0x1234),
              "M3 OFF: the confirm verdict is returned");
        CHECK(g_calls.confirmGate == 1 && g_calls.lastWeaponID == 0x777u &&
              g_calls.lastAttachID == 0x1234u,
              "M3 OFF: ... called once, with the weapon id intact");
        CHECK(g_diag.asked[GateAttachment] == 1 && g_diag.granted[GateAttachment] == 0 &&
              g_diag.asked[GateSkin] == 1 && g_diag.granted[GateSkin] == 0 &&
              g_diag.asked[GateLoadout] == 1 && g_diag.granted[GateLoadout] == 0 &&
              g_diag.asked[GateConfirm] == 1 && g_diag.granted[GateConfirm] == 0,
              "M3 OFF: asked but never granted - the counters tell the two states apart");
    }

    // ---- M4: a null receiver is never answered for ------------------------
    setFlags(true, true);
    g_calls.reset();
    memset(&g_diag, 0, sizeof(g_diag));
    {
        CHECK(!IsWeaponAttachmentUnlock(nullptr, 0x1234),
              "M4 a null object gets the game's answer, never a fabricated yes");
        CHECK(!IsWeaponSkinUnlock(nullptr, 0x99),
              "M4 ... for the skin gate too");
        CHECK(g_calls.attachmentUnlock == 1 && g_calls.skinUnlock == 1,
              "M4 ... and the game's own function did the deciding");
        CHECK(g_diag.granted[GateAttachment] == 0 && g_diag.granted[GateSkin] == 0,
              "M4 ... and nothing counts as granted");
    }

    // ---- M5: the legacy flag unlocks attachments only, never skins --------
    setFlags(false, /*legacyAttachment=*/true);
    g_calls.reset();
    memset(&g_diag, 0, sizeof(g_diag));
    {
        CHECK(IsWeaponAttachmentUnlock(&g_instance, 0x1234) &&
              IsThisAttachmentUnlock(&g_instance, 0x555, 0x1234, false),
              "M5 the old Attachment flag still opens the attachment half");
        CHECK(!IsWeaponSkinUnlock(&g_instance, 0x99) && g_calls.skinUnlock == 1,
              "M5 ... but it never claims a skin is owned");
        CHECK(g_diag.granted[GateAttachment] == 1 && g_diag.granted[GateSkin] == 0,
              "M5 ... which is exactly what the counters show");
    }

    // ---- M6: before the hooks install, the gates do not crash -------------
    orig_IsWeaponAttachmentUnlock = nullptr;
    orig_IsWeaponSkinUnlock       = nullptr;
    orig_IsThisAttachmentUnlock   = nullptr;
    orig_IsAttachUnlock           = nullptr;
    setFlags(false, false);
    {
        CHECK(!IsWeaponAttachmentUnlock(&g_instance, 0x1234) &&
              !IsWeaponSkinUnlock(&g_instance, 0x99) &&
              !IsThisAttachmentUnlock(&g_instance, 0x555, 0x1234, false) &&
              !IsAttachUnlock(&g_instance, 0x555, 0x1234),
              "M6 with no original installed the pass-through answers false, it does not dereference null");
    }

    // Leave the global config as the rest of the session would find it.
    setFlags(true, false);
    installFakes();

    if (g_failures == 0)
        printf("OK: all mythic-attachment checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
