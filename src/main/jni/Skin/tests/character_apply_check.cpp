// Host test for the character-skin apply path - the fix for
// "I picked Ghost - Stealth in the Target Character dropdown and the skin
// applied on Charly" and for the blank character card.
//
// It builds fake CharacterModelConfig / ItemResourceConfig / RoleConfConfig /
// RolePackConfConfig rows (sentinel values at the exact offsets the loaders and
// Skin/Fields.h use) and drives the REAL ApplyCharacterSkin() the character
// list button calls, then asserts what was written.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests, __fastcall is elided like the real build):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/character_apply_check.cpp \
//       src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread \
//       -o /tmp/character_apply_check && /tmp/character_apply_check
// Exits 0 and prints "OK: all character-apply checks passed" on success.
// Android.mk does not wildcard Skin/tests, so it never reaches the APK.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <utility>

#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
// Skin/Fields.h's inline helpers guard their rows with Tools::IsPtrValid, so
// the declaration has to be visible here; the real Tools.cpp is linked into
// this test (see the command above) rather than stubbed.
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "Skin/Fields.h"

// ---------------------------------------------------------------------------
// Host-only link stubs for the real Tools.cpp: it references the inline-hook
// engine and OpenSSL's MD5, and this test reaches neither. The check the test
// does rely on (Tools::IsPtrValid) is the real implementation.
// ---------------------------------------------------------------------------
#include <openssl/md5.h>

extern "C" int DobbyHook(void *address, void *replace_call, void **origin_call)
{
    (void)address; (void)replace_call; (void)origin_call;
    return 0;
}
extern "C" int MD5_Init(MD5_CTX *) { return 0; }
extern "C" int MD5_Update(MD5_CTX *, const void *, size_t) { return 0; }
extern "C" int MD5_Final(unsigned char *, MD5_CTX *) { return 0; }

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
// Fake config rows. Everything is read through the same +0x10 cast the loaders
// use, so the buffer only has to be big enough for the field offsets.
// ---------------------------------------------------------------------------
static const size_t OBJ_SIZE = 0x200;

struct FakeObj { alignas(8) unsigned char b[OBJ_SIZE]; };

template <typename T>
static void put(unsigned char *o, size_t off, T v) { std::memcpy(o + off, &v, sizeof v); }

template <typename T>
static T get(const unsigned char *o, size_t off) { T v; std::memcpy(&v, o + off, sizeof v); return v; }

// The ids the shipped config data uses for the default character.
static const int CHARLY_1P   = 710001101;
static const int CHARLY_3P   = 710001102;
static const int CHARLY_ITEM = 100301208;
static const int CHARLY_ROLE = 100301208;

static const int GHOST_1P   = 900000001;
static const int GHOST_3P   = 900000002;
static const int GHOST_ITEM = 500000123;
static const int GHOST_ROLE = 600000234;
static const int GHOST_PACK = 700000345;

// Sentinels so "was this row touched at all" is answerable.
static const int   SENT_MODEL = 111111;
static const uint16_t SENT_SPR = 22222;
static const int   SENT_ASSET = 3333333;

static FakeObj g_charlyModel, g_ghostModel;
static FakeObj g_charlyRes,   g_ghostRes;
static FakeObj g_charlyRole,  g_ghostRole;
static FakeObj g_pack, g_packNoFrame;

static void setModel(FakeObj &o, int t1p, int t3p)
{
    std::memset(o.b, 0, sizeof o.b);
    // Offsets below are struct-relative: every reader casts the object at +0x10
    // first, so field 0x18 of the object is 0x08 inside the struct.
    unsigned char *f = o.b + 0x10;                 // CharacterModelFields
    for (int i = 0; i < 12; i++) put<int>(f, 0x08 + 4 * i, SENT_MODEL);  // BRBag..GuarderLobby
    put<int>(f, 0x40, t1p);                        // Traitor1P (object 0x50)
    put<int>(f, 0x44, t3p);                        // Traitor3P (object 0x54)
}

static void setRes(FakeObj &o, int id)
{
    std::memset(o.b, 0, sizeof o.b);
    unsigned char *f = o.b + 0x10;                 // ItemResourceFields
    put<uint16_t>(f, 0x00, SENT_SPR);              // UIMiniSpriteNameIndex
    put<uint16_t>(f, 0x02, SENT_SPR);              // UISmallSpriteNameIndex
    put<uint16_t>(f, 0x04, SENT_SPR);              // UISpriteNameIndex
    put<uint16_t>(f, 0x06, SENT_SPR);              // UISquareSpriteNameIndex
    put<int32_t>(f, 0x08, SENT_ASSET);             // FxAssetID
    put<int32_t>(f, 0x0C, id);                     // ID
    put<int32_t>(f, 0x10, SENT_ASSET);             // InventoryModelID
    put<int32_t>(f, 0x14, SENT_ASSET);             // ModelAssetIDRaw
}

static void setRole(FakeObj &o, int id, int packId)
{
    std::memset(o.b, 0, sizeof o.b);
    unsigned char *f = o.b + 0x10;                 // RoleConfFields
    put<uint32_t>(f, 0x24, (uint32_t)id);          // ID @0x34
    put<int32_t>(f, 0x34, packId);                 // RolePackID @0x44
}

static void setPack(FakeObj &o, int packId, int loadingFrame)
{
    std::memset(o.b, 0, sizeof o.b);
    unsigned char *f = o.b + 0x10;                 // RolePackFields
    put<int32_t>(f, 0x5C, loadingFrame);           // LoadingFrame @0x6C
    put<int32_t>(f, 0x60, 0);                      // LobbySceneType @0x70
    put<uint32_t>(f, 0x80, (uint32_t)packId);      // RolePackID @0x90
}

// The values of the character whose skin was clicked ("Ghost - Stealth"), all
// distinct so every write is attributable.
static charInfo makeGhostSkin()
{
    charInfo c{};
    c.charName = "Ghost - Stealth";
    for (int i = 0; i < 12; i++) c.charModel[i] = 1001 + i;
    c.charRes[0] = 7000001;
    c.charRes[1] = 7000002;
    c.charRes[2] = 7000003;
    c.ItemRes2[0] = 11;
    c.ItemRes2[1] = 22;
    c.ItemRes2[2] = 33;
    c.ItemRes2[3] = 44;
    // ColorID / ColorSubID / ShowRare / RoleLvGroupID are byte fields, so keep
    // these small enough to survive the store.
    for (int i = 0; i < 8; i++) c.charRole[i] = 11 + i;
    c.charRole2[0] = (void *)0x1234;
    for (int i = 0; i < 6; i++) c.charPack[i] = 3001 + i;
    return c;
}

static std::vector<void *> models()  { return { &g_charlyModel, &g_ghostModel }; }
static std::vector<void *> res()     { return { &g_charlyRes,   &g_ghostRes }; }
static std::vector<void *> roles()   { return { &g_charlyRole,  &g_ghostRole }; }
static std::vector<void *> packs()   { return { &g_pack, &g_packNoFrame }; }

static void resetWorld()
{
    setModel(g_charlyModel, CHARLY_1P, CHARLY_3P);
    setModel(g_ghostModel,  GHOST_1P,  GHOST_3P);
    setRes(g_charlyRes, CHARLY_ITEM);
    setRes(g_ghostRes,  GHOST_ITEM);
    setRole(g_charlyRole, CHARLY_ROLE, GHOST_PACK);
    setRole(g_ghostRole,  GHOST_ROLE,  GHOST_PACK);
    setPack(g_pack, GHOST_PACK, 99);               // usable pack
    setPack(g_packNoFrame, GHOST_PACK, 0);         // no loading frame -> must be left alone
}

// ---- read-back helpers ----------------------------------------------------
// Struct-relative, matching the applier's +0x10 cast.
static int  modelField(const FakeObj &o, int i) { return get<int>(o.b + 0x10, 0x08 + 4 * i); }
static uint16_t resSpr(const FakeObj &o, int i) { return get<uint16_t>(o.b + 0x10, 2 * i); }
static int  resAsset(const FakeObj &o, int i)
{
    // FxAssetID 0x08, ID 0x0C (skipped), InventoryModelID 0x10, ModelAssetIDRaw 0x14.
    static const size_t off[3] = { 0x08, 0x10, 0x14 };
    return get<int32_t>(o.b + 0x10, off[i]);
}
static int  roleColor(const FakeObj &o)         { return get<uint8_t>(o.b + 0x10, 0x12); }
static int  packLoading(const FakeObj &o)       { return get<int32_t>(o.b + 0x10, 0x5C); }

static bool modelUnchanged(const FakeObj &o)
{
    for (int i = 0; i < 12; i++)
        if (modelField(o, i) != SENT_MODEL) return false;
    return true;
}
static bool resUnchanged(const FakeObj &o)
{
    for (int i = 0; i < 4; i++)
        if (resSpr(o, i) != SENT_SPR) return false;
    for (int i = 0; i < 3; i++)
        if (resAsset(o, i) != SENT_ASSET) return false;
    return true;
}

// ---------------------------------------------------------------------------
int main()
{
    const TargetChar ghost{"Ghost - Stealth", GHOST_1P, GHOST_3P,
                           GHOST_ITEM, GHOST_ROLE, GHOST_PACK};
    const TargetChar charly{"Charly", CHARLY_1P, CHARLY_3P,
                            CHARLY_ITEM, CHARLY_ROLE, GHOST_PACK};
    const charInfo skin = makeGhostSkin();

    // --- T1: ResolveTargetIds ---------------------------------------------
    {
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        ResolveTargetIds(ghost, a, b, c, d, e);
        CHECK(a == GHOST_1P && b == GHOST_3P && c == GHOST_ITEM &&
              d == GHOST_ROLE && e == GHOST_PACK,
              "T1 non-default target keeps its own ids");
        ResolveTargetIds(charly, a, b, c, d, e);
        CHECK(a == CHARLY_1P && b == CHARLY_3P && c == CHARLY_ITEM &&
              d == CHARLY_ROLE,
              "T1 Charly keeps the hardcoded default ids");
    }

    // --- T2: target = Ghost -> ONLY Ghost's rows are written ---------------
    {
        resetWorld();
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        ResolveTargetIds(ghost, a, b, c, d, e);
        ApplyCharacterSkin(skin, a, b, c, d, e, models(), res(), roles(), packs());

        bool modelOk = true;
        for (int i = 0; i < 12; i++)
            if (modelField(g_ghostModel, i) != 1001 + i) modelOk = false;
        CHECK(modelOk, "T2 Ghost's 12 model ids received the clicked skin");
        CHECK(modelUnchanged(g_charlyModel),
              "T2 Charly's model config was NOT touched (the reported bug)");

        CHECK(resSpr(g_ghostRes, 0) == 11 && resSpr(g_ghostRes, 1) == 22 &&
              resSpr(g_ghostRes, 2) == 33 && resSpr(g_ghostRes, 3) == 44,
              "T2 Ghost's four UI sprite indices came from ItemRes2");
        CHECK(resAsset(g_ghostRes, 0) == 7000001 &&
              resAsset(g_ghostRes, 1) == 7000002 &&
              resAsset(g_ghostRes, 2) == 7000003,
              "T2 Ghost's three asset ids came from charRes");
        CHECK(resUnchanged(g_charlyRes),
              "T2 Charly's item resource was NOT touched");

        // The blank-card defect: the sprite fields must NOT hold the asset ids.
        CHECK(resSpr(g_ghostRes, 0) != (uint16_t)7000001 &&
              resSpr(g_ghostRes, 1) != (uint16_t)7000002 &&
              resSpr(g_ghostRes, 2) != (uint16_t)7000003,
              "T2 the sprite fields are ItemRes2 values, not truncated charRes asset ids");
        CHECK(resSpr(g_ghostRes, 3) != SENT_SPR,
              "T2 the square sprite index is written too (the old code skipped it)");

        CHECK(roleColor(g_ghostRole) == 11 + 3,
              "T2 Ghost's RoleConf ColorID was written (charRole[3])");
        CHECK(get<uint32_t>(g_charlyRole.b + 0x10, 0x24) == (uint32_t)CHARLY_ROLE &&
              roleColor(g_charlyRole) == 0,
              "T2 Charly's RoleConf was NOT touched");

        CHECK(packLoading(g_pack) == 3001 + 4,
              "T2 the target's RolePack got the skin's loading frame");
        CHECK(packLoading(g_packNoFrame) == 0,
              "T2 a pack without a loading frame is left alone");
    }

    // --- T3: target = Charly -> Charly only, Ghost untouched ---------------
    {
        resetWorld();
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        ResolveTargetIds(charly, a, b, c, d, e);
        ApplyCharacterSkin(skin, a, b, c, d, e, models(), res(), roles(), packs());

        bool charlyOk = true;
        for (int i = 0; i < 12; i++)
            if (modelField(g_charlyModel, i) != 1001 + i) charlyOk = false;
        CHECK(charlyOk, "T3 selecting Charly still applies to Charly");
        CHECK(modelUnchanged(g_ghostModel), "T3 Ghost's model config was NOT touched");
        CHECK(resSpr(g_charlyRes, 0) == 11 && resSpr(g_charlyRes, 3) == 44,
              "T3 Charly's card sprites were written");
        CHECK(resUnchanged(g_ghostRes), "T3 Ghost's item resource was NOT touched");
    }

    // --- T4: a skin with zero sprite/asset values must not blank the card --
    {
        resetWorld();
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        ResolveTargetIds(ghost, a, b, c, d, e);

        charInfo blank = skin;
        blank.charRes = {0, 0, 0};
        blank.ItemRes2 = {0, 0, 0, 0};

        // Give the target real values first (as the game's own data has).
        setRes(g_ghostRes, GHOST_ITEM);
        put<uint16_t>(g_ghostRes.b + 0x10, 0x00, 777);
        put<int32_t>(g_ghostRes.b + 0x10, 0x08, 888888);

        ApplyCharacterSkin(blank, a, b, c, d, e, models(), res(), roles(), packs());

        CHECK(resSpr(g_ghostRes, 0) == 777, "T4 a zero sprite value does not blank the card");
        CHECK(resAsset(g_ghostRes, 0) == 888888, "T4 a zero asset id does not blank the model");
        bool modelOk = true;
        for (int i = 0; i < 12; i++)
            if (modelField(g_ghostModel, i) != 1000 + 1 + i) modelOk = false;
        CHECK(modelOk, "T4 the model ids are still copied (they are never zero)");
    }

    // --- T5: an unknown target id matches nothing -------------------------
    {
        resetWorld();
        ApplyCharacterSkin(skin, 12345, 54321, 999, 998, 997,
                           models(), res(), roles(), packs());
        CHECK(modelUnchanged(g_charlyModel) && modelUnchanged(g_ghostModel),
              "T5 an id that matches no row writes nothing");
        CHECK(resUnchanged(g_charlyRes) && resUnchanged(g_ghostRes),
              "T5 ... including the item resources");
    }

    // --- T6: a lobby with no RolePackConf rows ------------------------------
    // The list builder stopped waiting for the pack tier in Skin/Thread.h, but
    // the click handler still demanded all four vectors before it would apply
    // anything, and the game builds the pack configs lazily - so in a lobby a
    // click on a character did nothing at all. CharacterApplyReady() must not
    // look at the pack tier, and the apply must still write the tiers it has.
    {
        resetWorld();
        CHECK(CharacterApplyReady(models(), res(), roles()),
              "T6 a lobby without RolePackConf rows is still ready to apply");
        CHECK(!CharacterApplyReady({}, res(), roles()) &&
              !CharacterApplyReady(models(), {}, roles()) &&
              !CharacterApplyReady(models(), res(), {}),
              "T6 ... but a tier the apply really reads is required");

        std::vector<void *> noPacks;   // the game built none
        int a = 0, b = 0, c = 0, d = 0, e = 0;
        ResolveTargetIds(ghost, a, b, c, d, e);
        ApplyCharacterSkin(skin, a, b, c, d, e, models(), res(), roles(), noPacks);

        bool modelOk = true;
        for (int i = 0; i < 12; i++)
            if (modelField(g_ghostModel, i) != 1001 + i) modelOk = false;
        CHECK(modelOk, "T6 the model ids land with an empty pack list");
        CHECK(resSpr(g_ghostRes, 0) == 11,
              "T6 the card sprites land with an empty pack list");
        CHECK(roleColor(g_ghostRole) == 11 + 3,
              "T6 the role conf lands with an empty pack list");
    }

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all character-apply checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
