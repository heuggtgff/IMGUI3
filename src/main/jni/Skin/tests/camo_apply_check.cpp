// Host test for the Camo tab - the work behind "add every camo in the game,
// searchable, and make a pick replace the equipped camo on every gun".
//
// It drives the REAL InsertCamo() (the de-duping list builder LoadCamos() feeds
// from the game's WeaponCamosConfig table) and the REAL ApplyCamoToEveryGun()
// the tab's rows call, over fake WeaponConfConfig rows written at the exact
// offsets Skin/Fields.h reads (cast base +0x20: DefWeaponSkinID @0x38, ID @0x40).
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests, __fastcall is elided like the real build):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/camo_apply_check.cpp \
//       src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread \
//       -o /tmp/camo_apply_check && /tmp/camo_apply_check
// Exits 0 and prints "OK: all camo-apply checks passed" on success.
// Android.mk does not wildcard Skin/tests, so it never reaches the APK.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <utility>

#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
// ApplyCamoToEveryGun() validates each conf with Tools::IsPtrValid, so the real
// Tools.cpp is linked into this test (see the command above) rather than stubbed.
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "Skin/Fields.h"

// The tab's camo list. Fields.h only declares it; in the app Skin/Thread.h owns
// the definition, and this test is the only other place it lives.
std::vector<camoInfo> camoData;

// ---------------------------------------------------------------------------
// Host-only link stubs for the real Tools.cpp: it references the inline-hook
// engine and OpenSSL's MD5, and this test reaches neither. The validity check
// this test does rely on (Tools::IsPtrValid) is the real implementation.
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
// Fake WeaponConfConfig rows, read through the same +0x20 cast as the appliar.
// ---------------------------------------------------------------------------
static const size_t OBJ_SIZE = 0x200;

struct ConfObj { alignas(8) unsigned char b[OBJ_SIZE]; };

template <typename T>
static void put(unsigned char *o, size_t off, T v) { std::memcpy(o + off, &v, sizeof v); }
template <typename T>
static T get(const unsigned char *o, size_t off) { T v; std::memcpy(&v, o + off, sizeof v); return v; }

// Conf field offsets, relative to the +0x20 cast base.
static const size_t CF_ID     = 0x20;   // real 0x40
static const size_t CF_DEFSKIN = 0x18;  // real 0x38

static std::vector<std::unique_ptr<ConfObj>> g_store;

static ConfObj *makeConf(uint id, int defSkin)
{
    auto c = std::make_unique<ConfObj>();
    std::memset(c->b, 0, OBJ_SIZE);
    put<uint>(c->b + 0x20, CF_ID, id);
    put<int>(c->b + 0x20, CF_DEFSKIN, defSkin);
    ConfObj *raw = c.get();
    g_store.push_back(std::move(c));
    return raw;
}

static int  confSkin(const ConfObj *c) { return get<int>(c->b + 0x20, CF_DEFSKIN); }
static uint confID(const ConfObj *c)   { return get<uint>(c->b + 0x20, CF_ID); }

// The proven constants the original template shipped (Skin/Fields.h).
static const int ID_GOLD     = 0x1D37F755;
static const int ID_REDSPR   = 0x1D37F77E;
static const int ID_DIAMOND  = 0x1D37F758;

static const int CAMO_GOLD_ID = 131071;   // a table row for Gold

// ---------------------------------------------------------------------------
int main()
{
    // --- T1: one entry per camo, not one per (camo, weapon) row ------------
    {
        camoData.clear();
        InsertCamo(0x1111, "Zulu Camo", 10, 0);
        InsertCamo(0x2222, "alpha camo", 20, 0);
        InsertCamo(0x1111, "Zulu Camo", 10, 955);   // same camo, another weapon row
        InsertCamo(0x2222, "alpha camo", 20, 0);
        CHECK(camoData.size() == 2, "T1 a camo repeated once per weapon appears once");
        CHECK(camoData.size() == 2 && camoData[0].camoItemID == 0x2222 &&
              camoData[1].camoItemID == 0x1111,
              "T1 the list is ordered by name, case-insensitively");
    }

    // --- T2: a row whose name table was not ready is filled in later -------
    {
        camoData.clear();
        InsertCamo(CAMO_GOLD_ID, "Camo 131071", 0, 955);
        CHECK(camoData.size() == 1 && camoData[0].applyID == CAMO_GOLD_ID,
              "T2 an unresolved name applies the row's own id");
        CHECK(camoData.size() == 1 && camoData[0].belongWeaponID == 955,
              "T2 a per-weapon row is kept as it came");

        InsertCamo(CAMO_GOLD_ID, "Gold", 4242, 0);   // the real row, name ready
        CHECK(camoData.size() == 1, "T2 the second row does not add an entry");
        CHECK(camoData.size() == 1 && camoData[0].camoName == "Gold",
              "T2 the real name replaces the placeholder");
        CHECK(camoData.size() == 1 && camoData[0].applyID == ID_GOLD,
              "T2 Gold applies the proven constant, not the table id");
        CHECK(camoData.size() == 1 && camoData[0].textureAssetID == 4242,
              "T2 the texture id is upgraded too");
        CHECK(camoData.size() == 1 && camoData[0].belongWeaponID == 0,
              "T2 a row usable on every weapon wins over a per-weapon one");
    }

    // --- T2b: a name that merely contains a keyword is not that camo -------
    {
        camoData.clear();
        InsertCamo(5001, "Golden Emerald", 1, 0);
        InsertCamo(5002, "Gold", 1, 0);
        InsertCamo(5003, "krm gold", 1, 0);      // per-weapon prefix form
        CHECK(camoData.size() == 3, "T2b the three rows stay three camos");
        const camoInfo *ge = nullptr, *gold = nullptr, *prefixed = nullptr;
        for (const auto &c : camoData) {
            if (c.camoName == "Golden Emerald") ge = &c;
            if (c.camoName == "Gold") gold = &c;
            if (c.camoName == "krm gold") prefixed = &c;
        }
        CHECK(ge && ge->applyID == 5001,
              "T2b \"Golden Emerald\" is not read as Gold");
        CHECK(gold && gold->applyID == ID_GOLD, "T2b an exact \"Gold\" is");
        CHECK(prefixed && prefixed->applyID == ID_GOLD,
              "T2b a \"<weapon> gold\" row is too");
    }

    // --- T2c: a name with the id form still resolves once the real one lands
    {
        camoData.clear();
        InsertCamo(6001, "Camo 6001", 0, 0);
        InsertCamo(6001, "Aether Crystal", 7, 0);
        CHECK(camoData.size() == 1 && camoData[0].applyID == 0x1D37F77D,
              "T2c Aether Crystal applies its proven constant");
        camoData.clear();
        InsertCamo(6002, "sakura red sprite", 0, 0);
        CHECK(camoData.size() == 1 && camoData[0].applyID == 0x1D37F77E,
              "T2c a prefixed Red Sprite row applies its proven constant");
    }

    // --- T3: a pick replaces whatever every gun had ------------------------
    {
        std::unordered_map<uint, int> originals;
        ConfObj *krm  = makeConf(100, 555);   // "Gold on the KRM"
        ConfObj *m4   = makeConf(200, 0);     // no camo
        ConfObj *ak   = makeConf(300, 777);
        std::vector<void *> confs = { krm, m4, ak };

        int written = ApplyCamoToEveryGun(ID_DIAMOND, confs, originals);
        CHECK(written == 3, "T3 every conf is written, not only those with a skin row");
        CHECK(confSkin(krm) == ID_DIAMOND && confSkin(m4) == ID_DIAMOND &&
              confSkin(ak) == ID_DIAMOND,
              "T3 the pick replaced the equipped camo on every gun");
        CHECK(originals.size() == 3 && originals[100] == 555 && originals[200] == 0 &&
              originals[300] == 777,
              "T3 each conf's own previous value was remembered once");

        // Switching camo replaces the previous pick, it does not stack.
        ApplyCamoToEveryGun(ID_REDSPR, confs, originals);
        CHECK(confSkin(krm) == ID_REDSPR && confSkin(m4) == ID_REDSPR &&
              confSkin(ak) == ID_REDSPR,
              "T3 a second pick replaces the first on every gun");
        CHECK(originals[100] == 555 && originals[200] == 0 && originals[300] == 777,
              "T3 the remembered originals are still the game's own values");

        // A conf the game creates later (another weapon) is picked up too.
        ConfObj *newGun = makeConf(400, 99);
        confs.push_back(newGun);
        ApplyCamoToEveryGun(ID_REDSPR, confs, originals);
        CHECK(confSkin(newGun) == ID_REDSPR,
              "T3 a conf added after the first pick is written on the next one");

        // OFF puts the remembered values back and leaves untouched confs alone.
        ApplyCamoToEveryGun(0, confs, originals);
        CHECK(confSkin(krm) == 555 && confSkin(m4) == 0 && confSkin(ak) == 777,
              "T3 OFF restores each gun's own camo");
        CHECK(confSkin(newGun) == 99,
              "T3 OFF leaves a conf it never wrote completely alone");
    }

    // --- T4: a malformed conf is skipped, not written ----------------------
    {
        std::unordered_map<uint, int> originals;
        ConfObj *noId = makeConf(0, 1234);    // ID == 0: not a real weapon row
        ConfObj *ok   = makeConf(500, 4321);
        std::vector<void *> confs = { nullptr, noId, ok };

        int written = ApplyCamoToEveryGun(ID_GOLD, confs, originals);
        CHECK(written == 1, "T4 a null conf and an id==0 conf are skipped");
        CHECK(confSkin(noId) == 1234, "T4 an id==0 conf is not written");
        CHECK(confSkin(ok) == ID_GOLD, "T4 the real conf in the same list is written");
        CHECK(originals.size() == 1 && originals.count(500) == 1,
              "T4 no bogus original is stored under id 0");
    }

    // --- T5: the search filter the tab applies over camoData ---------------
    {
        camoData.clear();
        InsertCamo(7001, "Gold", 0, 0);
        InsertCamo(7002, "Diamond", 0, 0);
        InsertCamo(7003, "Aether Crystal", 0, 0);
        InsertCamo(7004, "Red Sprite", 0, 0);

        auto matches = [](const char *query) {
            const std::string lower = CamoNameLower(query);
            int n = 0;
            for (const auto &c : camoData)
                if (CamoNameLower(c.camoName).find(lower) != std::string::npos)
                    n++;
            return n;
        };

        CHECK(matches("gold") == 1, "T5 a search for gold finds exactly the Gold camo");
        CHECK(matches("GOLD") == 1, "T5 the search is case-insensitive");
        CHECK(matches("crystal") == 1, "T5 a mid-name search finds Aether Crystal");
        // Diamond and Aether Crystal contain an "a"; Gold and Red Sprite do not.
        CHECK(matches("a") == 2, "T5 a one-letter search matches mid-name characters");
        CHECK(matches("zzz") == 0, "T5 a search that matches nothing returns nothing");
    }

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all camo-apply checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("FAILED: %d of %d checks\n", g_failures, g_checks);
    return 1;
}
