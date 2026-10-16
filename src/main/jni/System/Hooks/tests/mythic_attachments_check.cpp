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
#include <cstdlib>
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

// -- helpers: a List<AttachmentGroupNew> and its rows, at the offsets the
// dump documents (absolute from the object pointer, IL2CPP's 16-byte header
// included - kAttachGroupNew_OwnState / kList_* / kArray_*).
static char *make_group(int ownState)
{
    char *g = (char *)calloc(1, 0x40);
    *(int *)(g + kAttachGroupNew_OwnState) = ownState;
    return g;
}
static char *make_list(char **groups, int count, int backingCapacity)
{
    char *list = (char *)calloc(1, 0x28);
    char *arr  = (char *)calloc(1, kArray_Data + sizeof(void *) * (size_t)backingCapacity);
    *(uint64_t *)(arr + kArray_MaxLen) = (uint64_t)backingCapacity;
    for (int i = 0; i < count && i < backingCapacity; i++)
        *(void **)(arr + kArray_Data + i * sizeof(void *)) = groups[i];
    *(void **)(list + kList_Items) = arr;
    *(int *)(list + kList_Size) = count;
    return list;
}
static void free_list(char *list)
{
    if (!list) return;
    free(*(void **)(list + kList_Items));
    free(list);
}
static int own_state(char *group) { return *(int *)(group + kAttachGroupNew_OwnState); }

// -- fake originals -------------------------------------------------------
static void  *g_fakeList = nullptr;
static int    g_listCalls = 0;
static int    g_listType  = -1;
static void  *fake_origGetAttachmentList(void *, int attachmentType)
{
    g_listCalls++;
    g_listType = attachmentType;
    return g_fakeList;
}

static int    g_itemCalls = 0;
static void  *g_itemInstance = nullptr, *g_itemGroup = nullptr, *g_itemPanel = nullptr;
static void   fake_origItemSetData(void *instance, void *group, void *panel)
{
    g_itemCalls++;
    g_itemInstance = instance; g_itemGroup = group; g_itemPanel = panel;
}

static int    g_cellCalls = 0;
static int32_t g_cellWeapon = -1, g_cellType = -1, g_cellState = -1;
static void   *g_cellAttachItem = nullptr, *g_cellCallback = nullptr;
static void   *g_cellConflict = nullptr, *g_cellPosName = nullptr;
static bool   g_cellTransmog = false, g_cellReddot = false, g_cellGuar = false;
static void   fake_origCellSetData(void *, uint32_t weaponID, int32_t attachmentType,
                                   void *attachItem, void *callback, int32_t cellState,
                                   void *conflict, void *posName, bool transmog,
                                   bool reddot, bool guar)
{
    g_cellCalls++;
    g_cellWeapon = (int32_t)weaponID; g_cellType = attachmentType; g_cellState = cellState;
    g_cellAttachItem = attachItem; g_cellCallback = callback;
    g_cellConflict = conflict; g_cellPosName = posName;
    g_cellTransmog = transmog; g_cellReddot = reddot; g_cellGuar = guar;
}
static void installLayer2Fakes()
{
    orig_GetAttachmentList     = fake_origGetAttachmentList;
    orig_AttachmentItemSetData = fake_origItemSetData;
    orig_AttachmentCellSetData = fake_origCellSetData;
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

    // ==== Layer 2: the client-side DATA writes ==============================
    // These three hooks do not answer a question, they rewrite the client's own
    // objects (dump field layout) exactly the way the Skin tab writes config rows
    // when it applies a mythic - so the lock disappears without any of the four
    // answer functions above ever being asked.

    // The helpers and fake originals for these hooks live at file scope,
    // above main() - C++ does not allow nested function definitions.

    installLayer2Fakes();

    // ---- M7: ON rewrites the rows the popup is about to draw ---------------
    setFlags(true, false);
    memset(&g_diag, 0, sizeof(g_diag));
    g_listCalls = 0; g_listType = -1;
    {
        char *groups[5] = {make_group(kOwnStateELock), make_group(3 /*EEquip*/),
                           make_group(2 /*EDefault*/), make_group(kOwnStateEUnlock),
                           make_group(kOwnStateELock)};
        g_fakeList = make_list(groups, 5, 5);

        void *ret = GetAttachmentList(&g_instance, 7);
        CHECK(ret == g_fakeList,
              "M7 ON: the game's own list comes back untouched (same pointer)");
        CHECK(own_state(groups[0]) == kOwnStateEUnlock && own_state(groups[4]) == kOwnStateEUnlock,
              "M7 ON: every ELock row is rewritten to EUnlock in place");
        CHECK(own_state(groups[1]) == 3 && own_state(groups[2]) == 2 && own_state(groups[3]) == kOwnStateEUnlock,
              "M7 ON: equipped / default / already-unlocked rows keep the state the game gave them");
        CHECK(g_listCalls == 1 && g_listType == 7,
              "M7 ON: the real builder was called exactly once, with the attachment type intact");
        CHECK(g_diag.asked[GateList] == 1 && g_diag.granted[GateList] == 1,
              "M7 ON: list gate counts one ask and one grant");

        free_list((char *)g_fakeList);
        for (int i = 0; i < 5; i++) free(groups[i]);
        g_fakeList = nullptr;
    }

    // ---- M8: OFF leaves the rows exactly as the game made them -------------
    setFlags(false, false);
    memset(&g_diag, 0, sizeof(g_diag));
    g_listCalls = 0;
    {
        char *groups[2] = {make_group(kOwnStateELock), make_group(kOwnStateELock)};
        g_fakeList = make_list(groups, 2, 2);

        GetAttachmentList(&g_instance, 3);
        CHECK(own_state(groups[0]) == kOwnStateELock && own_state(groups[1]) == kOwnStateELock,
              "M8 OFF: nothing is rewritten - the game's own ownership stands");
        CHECK(g_listCalls == 1 && g_listType == 3,
              "M8 OFF: ... and the real builder still ran once with its argument");
        CHECK(g_diag.asked[GateList] == 1 && g_diag.granted[GateList] == 0,
              "M8 OFF: asked, not granted");

        free_list((char *)g_fakeList);
        for (int i = 0; i < 2; i++) free(groups[i]);
        g_fakeList = nullptr;
    }

    // ---- M9/M10: the row handed to its view -------------------------------
    setFlags(true, false);
    memset(&g_diag, 0, sizeof(g_diag));
    g_itemCalls = 0;
    {
        char *locked = make_group(kOwnStateELock);
        char *equipped = make_group(3);
        int panel = 0;

        AttachmentItemSetData(&g_instance, locked, &panel);
        CHECK(own_state(locked) == kOwnStateEUnlock,
              "M9 ON: the row is unlocked BEFORE the view reads it");
        CHECK(g_itemCalls == 1 && g_itemInstance == &g_instance &&
              g_itemGroup == locked && g_itemPanel == &panel,
              "M9 ON: the real SetData still gets all three arguments unchanged");
        CHECK(g_diag.asked[GateItem] == 1 && g_diag.granted[GateItem] == 1,
              "M9 ON: item gate counts one ask and one grant");

        AttachmentItemSetData(&g_instance, equipped, &panel);
        CHECK(own_state(equipped) == 3,
              "M9 ON: an equipped row is left alone (only ELock is rewritten)");
        CHECK(g_diag.granted[GateItem] == 1,
              "M9 ON: ... so nothing is double counted as granted");

        setFlags(false, false);
        memset(&g_diag, 0, sizeof(g_diag));
        g_itemCalls = 0;
        char *locked2 = make_group(kOwnStateELock);
        AttachmentItemSetData(&g_instance, locked2, &panel);
        CHECK(own_state(locked2) == kOwnStateELock && g_itemCalls == 1,
              "M10 OFF: the row keeps the game's lock and the real SetData runs");
        CHECK(g_diag.asked[GateItem] == 1 && g_diag.granted[GateItem] == 0,
              "M10 OFF: asked, not granted");

        free(locked); free(equipped); free(locked2);
    }

    // ---- M11/M12: the gunsmith's cell state --------------------------------
    setFlags(true, false);
    memset(&g_diag, 0, sizeof(g_diag));
    g_cellCalls = 0;
    {
        int attachItem = 0, callback = 0, conflict = 0, posName = 0;
        AttachmentCellSetData(&g_instance, 0x1122u, 4, &attachItem, &callback,
                              kCellStateELocked, &conflict, &posName,
                              /*transmog=*/true, /*reddot=*/false, /*guar=*/true);
        CHECK(g_cellCalls == 1 && g_cellState == kCellStateENone,
              "M11 ON: a locked slot reaches the game as unlocked");
        CHECK(g_cellWeapon == 0x1122 && g_cellType == 4 &&
              g_cellAttachItem == &attachItem && g_cellCallback == &callback &&
              g_cellConflict == &conflict && g_cellPosName == &posName,
              "M11 ON: every argument before cellState arrives unchanged");
        CHECK(g_cellTransmog && !g_cellReddot && g_cellGuar,
              "M11 ON: the two defaulted bools at the end arrive unchanged too");
        CHECK(g_diag.asked[GateCell] == 1 && g_diag.granted[GateCell] == 1,
              "M11 ON: cell gate counts one ask and one grant");

        AttachmentCellSetData(&g_instance, 0x1122u, 4, &attachItem, &callback,
                              1 /*EConflict*/, &conflict, &posName, false, true, false);
        CHECK(g_cellState == 1,
              "M11 ON: a conflicting slot keeps its state - only ELocked is rewritten");
        AttachmentCellSetData(&g_instance, 0x1122u, 4, &attachItem, &callback,
                              2 /*EInvalid*/, &conflict, &posName, false, true, false);
        CHECK(g_cellState == 2,
              "M11 ON: an invalid slot keeps its state too");

        setFlags(false, false);
        memset(&g_diag, 0, sizeof(g_diag));
        g_cellCalls = 0;
        AttachmentCellSetData(&g_instance, 0x1122u, 4, &attachItem, &callback,
                              kCellStateELocked, &conflict, &posName, false, true, false);
        CHECK(g_cellState == kCellStateELocked && g_cellCalls == 1,
              "M12 OFF: the locked state reaches the game unchanged");
        CHECK(g_diag.asked[GateCell] == 1 && g_diag.granted[GateCell] == 0,
              "M12 OFF: asked, not granted");
    }

    // ---- M13: a malformed list stops instead of walking off the heap -------
    setFlags(true, false);
    memset(&g_diag, 0, sizeof(g_diag));
    {
        CHECK(UnlockOwnStateInGroupList(nullptr) == 0,
              "M13 a null list rewrites nothing and does not crash");
        CHECK(UnlockOwnStateGroup(nullptr) == false,
              "M13 a null row is never written");
        CHECK(g_diag.granted[GateList] == 0 && g_diag.granted[GateItem] == 0,
              "M13 ... and nothing is counted as granted");

        // size larger than the backing array's own length -> refused outright.
        char *group = make_group(kOwnStateELock);
        char *bad[1] = {group};
        char *list = make_list(bad, /*count=*/5, /*backingCapacity=*/2);
        CHECK(UnlockOwnStateInGroupList(list) == 0 && own_state(group) == kOwnStateELock,
              "M13 a size the backing array cannot hold is refused before any write");
        free_list(list);

        // an absurd capacity is refused too.
        char *list2 = make_list(bad, 1, 1);
        *(uint64_t *)((char *)*(void **)((char *)list2 + kList_Items) + kArray_MaxLen) = 100000;
        CHECK(UnlockOwnStateInGroupList(list2) == 0 && own_state(group) == kOwnStateELock,
              "M13 ... and so is a capacity no attachment list can have");
        free_list(list2);
        free(group);
    }

    // ---- M14: before the hooks install, layer 2 must not dereference null ---
    orig_GetAttachmentList     = nullptr;
    orig_AttachmentItemSetData = nullptr;
    orig_AttachmentCellSetData = nullptr;
    setFlags(true, false);
    {
        g_listCalls = 0; g_itemCalls = 0; g_cellCalls = 0;
        CHECK(GetAttachmentList(&g_instance, 1) == nullptr && g_listCalls == 0,
              "M14 with no original the list hook returns null instead of crashing");
        char *group = make_group(kOwnStateELock);
        int panel = 0, attach = 0, cb = 0;
        AttachmentItemSetData(&g_instance, group, &panel);
        CHECK(g_itemCalls == 0, "M14 ... the item hook calls nothing");
        AttachmentCellSetData(&g_instance, 1u, 1, &attach, &cb, kCellStateELocked,
                              nullptr, nullptr, false, false, false);
        CHECK(g_cellCalls == 0, "M14 ... and the cell hook calls nothing");
        free(group);
    }
    installLayer2Fakes();

    // ---- M15: the landed flags the SKINS tab prints -------------------------
    memset(g_landed, 0, sizeof(g_landed));
    NoteLanded(GateList, true);
    NoteLanded(GateCell, false);
    NoteLanded((Gate)99, true);          // out of range is ignored, not written
    bool otherFlagSet = false;
    for (int gi = 0; gi < GateCount; gi++)
        if (gi != GateList && g_landed[gi]) otherFlagSet = true;
    CHECK(g_landed[GateList] && !g_landed[GateCell],
          "M15 a landed hook is reported, a failed one is not");
    CHECK(!otherFlagSet,
          "M15 an out-of-range gate leaves every other flag clear");

    // Leave the global config as the rest of the session would find it.
    setFlags(true, false);
    installFakes();

    if (g_failures == 0)
        printf("OK: all mythic-attachment checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
