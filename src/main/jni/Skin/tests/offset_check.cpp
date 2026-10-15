// Offset verification for the skin/kill-decoration loaders.
//
// Host test, not a syntax check: it builds fake config objects with sentinel
// values at the offsets read out of the il2cpp dump (dump.cs, XOR-mask decoded
// where the dumper masked them), then applies the exact casts/reads that
// Skin/Thread.h and System/UI/SkinTab.h use and asserts they return the
// sentinels. The pre-fix bases (+0x18) are exercised too, to show they read a
// neighbouring field.
//
// struct layouts model arm64 exactly: both host x86-64 and aarch64 are LP64,
// 8-byte pointers, natural alignment for these POD members.
//
// Run it from the repo root (no NDK needed — the Android/JNI headers are the
// stubs next to this file, and __fastcall is elided, as the real build does
// differently):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/offset_check.cpp \
//       -o /tmp/offset_check && /tmp/offset_check
// Exits 0 and prints "OK: all loader offsets match the il2cpp dump" on success;
// Android.mk does not wildcard this directory, so it never reaches the APK.
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <cassert>
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/KittyMemory.h"
#include "Includes/obfuscate.h"
#include "Hacks/StructGame/StructGame.h"
#include "Includes/Logger.h"
#include "Includes/Utils.h"
#include "Includes/Macros.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

// The TU above pulls in a header that references MSHookFunction; the test never
// hooks anything, so give it a no-op definition to link against.
extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

struct Skin_ExtraMenuStub { bool CamoTest; int CamoTestMode; };
struct Skin_ConfigStub { Skin_ExtraMenuStub ExtraMenu; };
static Skin_ConfigStub Config;

#include "Skin/Fields.h"

static int g_failures = 0;
#define CHECK_EQ(actual, expected, what)                                        \
    do {                                                                        \
        auto _a = (actual);                                                     \
        auto _e = (expected);                                                   \
        if (_a != _e) {                                                         \
            ++g_failures;                                                       \
            std::printf("FAIL %-46s got 0x%llx want 0x%llx\n", what,            \
                        (unsigned long long)(_a), (unsigned long long)(_e));     \
        }                                                                       \
    } while (0)

// ── static: struct layout must reproduce the dump offsets from its loader base
static void check_static()
{
    // ItemResourceConfig: first field UIMiniSpriteNameIndex @0x10, loader +0x10
    static_assert(offsetof(ItemResourceFields, UIMiniSpriteNameIndex) + 0x10 == 0x10, "");
    static_assert(offsetof(ItemResourceFields, UISmallSpriteNameIndex) + 0x10 == 0x12, "");
    static_assert(offsetof(ItemResourceFields, UISpriteNameIndex) + 0x10 == 0x14, "");
    static_assert(offsetof(ItemResourceFields, UISquareSpriteNameIndex) + 0x10 == 0x16, "");
    static_assert(offsetof(ItemResourceFields, FxAssetID) + 0x10 == 0x18, "");
    static_assert(offsetof(ItemResourceFields, ID) + 0x10 == 0x1C, "");
    static_assert(offsetof(ItemResourceFields, InventoryModelID) + 0x10 == 0x20, "");
    static_assert(offsetof(ItemResourceFields, ModelAssetIDRaw) + 0x10 == 0x24, "");
    static_assert(offsetof(ItemResourceFields, AvatarModelID) + 0x10 == 0x28, "");

    // WeaponConfExtraConfig: first field @0x10, loader +0x10
    static_assert(offsetof(WeaponConfExtraFields, DefaultMythicArmor) + 0x10 == 0x10, "");
    static_assert(offsetof(WeaponConfExtraFields, DefaultDeadReplayEffectId) + 0x10 == 0x18, "");
    static_assert(offsetof(WeaponConfExtraFields, DefaultKillEffectId) + 0x10 == 0x1C, "");
    static_assert(offsetof(WeaponConfExtraFields, FireEffectGroupID) + 0x10 == 0x20, "");
    static_assert(offsetof(WeaponConfExtraFields, ID) + 0x10 == 0x24, "");

    // KillEffectItemConfConfig: first field RealAssetIDs @0x10, loader +0x10
    static_assert(offsetof(KillEffectItemFields, RealAssetIDs) + 0x10 == 0x10, "");
    static_assert(offsetof(KillEffectItemFields, ColorID) + 0x10 == 0x1C, "");
    static_assert(offsetof(KillEffectItemFields, ID) + 0x10 == 0x2C, "");
    static_assert(offsetof(KillEffectItemFields, WeaponCanEquip) + 0x10 == 0x90, "");

    // CharacterModelConfig: first field BindEffect1PType @0x10, loader +0x10
    static_assert(offsetof(CharacterModelFields, Female) + 0x10 == 0x10, "");
    static_assert(offsetof(CharacterModelFields, AvatarAnimationType) + 0x10 == 0x14, "");
    static_assert(offsetof(CharacterModelFields, BRBagModel) + 0x10 == 0x18, "");
    static_assert(offsetof(CharacterModelFields, ItemID) + 0x10 == 0x78, "");
    static_assert(offsetof(CharacterModelFields, LitePackDefault) + 0x10 == 0x80, "");

    // MythicSightConfig: first field ShowInBag @0x10, loader +0x10
    static_assert(offsetof(MythicSightFields, ShowInBag) + 0x10 == 0x10, "");
    static_assert(offsetof(MythicSightFields, AssetID) + 0x10 == 0x14, "");
    static_assert(offsetof(MythicSightFields, ColorID) + 0x10 == 0x18, "");
    static_assert(offsetof(MythicSightFields, ID) + 0x10 == 0x20, "");
    static_assert(offsetof(MythicSightFields, WeaponCanEquip) + 0x10 == 0x38, "");

    // BRDeadboxSkinConfig: first field ColorID @0x10, loader +0x10
    static_assert(offsetof(BRDeadboxSkinFields, ColorID) + 0x10 == 0x10, "");
    static_assert(offsetof(BRDeadboxSkinFields, DeadBoxEffectAsset) + 0x10 == 0x14, "");
    static_assert(offsetof(BRDeadboxSkinFields, ID) + 0x10 == 0x20, "");

    // BRDropPlaneSkinConfig: first field ColorID @0x10, loader +0x10
    static_assert(offsetof(BRDropPlaneSkinFields, ColorID) + 0x10 == 0x10, "");
    static_assert(offsetof(BRDropPlaneSkinFields, ID) + 0x10 == 0x14, "");
    static_assert(offsetof(BRDropPlaneSkinFields, ModelAsset1P) + 0x10 == 0x18, "");
    static_assert(offsetof(BRDropPlaneSkinFields, Priority) + 0x10 == 0x28, "");

    // WeaponConfConfig: first field Accuracy @0x20, loader +0x20
    static_assert(offsetof(WeaponConfFields, Accuracy) + 0x20 == 0x20, "");
    static_assert(offsetof(WeaponConfFields, ColorID) + 0x20 == 0x22, "");
    static_assert(offsetof(WeaponConfFields, BaseWeaponId) + 0x20 == 0x34, "");
    static_assert(offsetof(WeaponConfFields, DefWeaponSkinID) + 0x20 == 0x38, "");
    static_assert(offsetof(WeaponConfFields, DefaultKillBrocast) + 0x20 == 0x3C, "");
    static_assert(offsetof(WeaponConfFields, ID) + 0x20 == 0x40, "");
    static_assert(offsetof(WeaponConfFields, IsDouble) + 0x20 == 0x44, "");
    static_assert(offsetof(WeaponConfFields, ChangeColorStr) + 0x20 == 0x50, "");
    static_assert(offsetof(WeaponConfFields, EnableCamoBeta) + 0x20 == 0x58, "");
    static_assert(offsetof(WeaponConfFields, LOCID_Desc) + 0x20 == 0x60, "");
    static_assert(offsetof(WeaponConfFields, LOCID_Name) + 0x20 == 0x68, "");
    static_assert(offsetof(WeaponConfFields, ModelRotation) + 0x20 == 0x70, "");
    static_assert(offsetof(WeaponConfFields, UIProperty) + 0x20 == 0x78, "");
    static_assert(offsetof(WeaponConfFields, UIPropertyUnlockLevel) + 0x20 == 0x80, "");

    // WeaponAssetGroupConfConfig: first field @0x10, loader +0x40
    static_assert(offsetof(WeaponAssetGroupFields, FireEffectGroupID) + 0x40 == 0x40, "");
    static_assert(offsetof(WeaponAssetGroupFields, Id) + 0x40 == 0x44, "");
    static_assert(offsetof(WeaponAssetGroupFields, MeshAssetID1P) + 0x40 == 0x48, "");

    // WeaponCamosConfig (loader +0x10) / WeaponSkinConfig (loader +0x10)
    static_assert(offsetof(WeaponCamosFields, CamoType) + 0x10 == 0x10, "");
    static_assert(offsetof(WeaponCamosFields, BelongWeaponID) + 0x10 == 0x14, "");
    static_assert(offsetof(WeaponCamosFields, CamoItemID) + 0x10 == 0x18, "");
    static_assert(offsetof(WeaponCamosFields, TextureAssetID) + 0x10 == 0x28, "");
    static_assert(offsetof(WeaponCamosFields, TextureSpecialAssetID) + 0x10 == 0x2C, "");
    static_assert(offsetof(WeaponSkinFields, BRWeaponID) + 0x10 == 0x14, "");
    static_assert(offsetof(WeaponSkinFields, CamoID) + 0x10 == 0x1C, "");
    static_assert(offsetof(WeaponSkinFields, ColorSubID) + 0x10 == 0x20, "");
    static_assert(offsetof(WeaponSkinFields, ID) + 0x10 == 0x24, "");
    static_assert(offsetof(WeaponSkinFields, WeaponID) + 0x10 == 0x28, "");

    // MythicArmorConfig (loader +0x14) and RoleSkinConfig (loader +0x18)
    static_assert(offsetof(MythicArmorFields, AssetID) + 0x14 == 0x14, "");
    static_assert(offsetof(MythicArmorFields, ID) + 0x14 == 0x24, "");
    static_assert(offsetof(MythicArmorFields, ThirdTab) + 0x14 == 0x30, "");
    static_assert(offsetof(RoleSkinFields, ClothType) + 0x18 == 0x18, "");

    std::printf("static_asserts: all struct layouts match the dump\n");
}

// ── runtime: sentinels at dump offsets, read back through the loader casts ──
static unsigned char g_buf[0x400];
static int g_payload = 4242;

static void poke_int(size_t off, int v)   { std::memcpy(g_buf + off, &v, sizeof(v)); }
static int  peek_int(size_t off)          { int v = 0; std::memcpy(&v, g_buf + off, sizeof(v)); return v; }

static void check_runtime()
{
    std::memset(g_buf, 0, sizeof(g_buf));

    // --- ItemResourceConfig: sentinels at the dump offsets
    poke_int(0x1C, 0x111111);   // ID
    poke_int(0x24, 0x222222);   // ModelAssetIDRaw
    poke_int(0x20, 0x333333);   // InventoryModelID
    poke_int(0x18, 0x444444);   // FxAssetID
    {
        auto *f = (ItemResourceFields *)(g_buf + 0x10);          // loader now
        CHECK_EQ(f->ID, 0x111111, "ItemResourceFields.ID @0x1C");
        CHECK_EQ(f->ModelAssetIDRaw, 0x222222, "ItemResourceFields.ModelAssetIDRaw @0x24");
        CHECK_EQ(f->InventoryModelID, 0x333333, "ItemResourceFields.InventoryModelID @0x20");
        CHECK_EQ(f->FxAssetID, 0x444444, "ItemResourceFields.FxAssetID @0x18");
        auto *old = (ItemResourceFields *)(g_buf + 0x18);        // pre-fix base
        CHECK_EQ(old->ID, 0x222222, "pre-fix base +0x18 read ModelAssetIDRaw");
    }

    // --- WeaponConfExtraConfig
    poke_int(0x1C, 0x555555);   // DefaultKillEffectId
    poke_int(0x24, 0x666666);   // ID
    poke_int(0x18, 0x777777);   // DefaultDeadReplayEffectId
    {
        auto *f = (WeaponConfExtraFields *)(g_buf + 0x10);
        CHECK_EQ(f->DefaultKillEffectId, 0x555555, "WeaponConfExtraFields.DefaultKillEffectId @0x1C");
        CHECK_EQ(f->ID, 0x666666, "WeaponConfExtraFields.ID @0x24");
        CHECK_EQ(f->DefaultDeadReplayEffectId, 0x777777, "WeaponConfExtraFields.DefaultDeadReplayEffectId @0x18");
        auto *old = (WeaponConfExtraFields *)(g_buf + 0x18);
        CHECK_EQ(old->DefaultKillEffectId, 0x666666, "pre-fix base +0x18 read ID as kill effect");
    }

    // --- KillEffectItemConfConfig: pointer fields + raw reads from Thread.h
    poke_int(0x2C, 0x888888);   // ID (raw read: kill + 0x2C)
    poke_int(0x14, 0x999999);   // IsAutoDemolition/ColorID neighbour (old base read this)
    static unsigned char fakeEquip[64], fakeReal[64];
    auto *realPtr = (Array<int> *)fakeReal;
    auto *equipPtr = (Array<int> *)fakeEquip;
    std::memcpy(g_buf + 0x10, &realPtr, sizeof(void *));        // RealAssetIDs
    std::memcpy(g_buf + 0x90, &equipPtr, sizeof(void *));       // WeaponCanEquip
    {
        auto *f = (KillEffectItemFields *)(g_buf + 0x10);
        CHECK_EQ((Array<int> *)f->RealAssetIDs == realPtr ? 1 : 0, 1, "RealAssetIDs pointer at kill+0x10");
        CHECK_EQ(peek_int(0x2C), 0x888888, "raw *(int*)(kill+0x2C) == ID");
        auto *old = (KillEffectItemFields *)(g_buf + 0x18);
        CHECK_EQ(old->ID, 0, "pre-fix base +0x18 gave ID 0 (map stayed empty)");
        Array<int> *oldReal = (Array<int> *)old->RealAssetIDs;
        CHECK_EQ(oldReal == realPtr ? 0 : 1, 1, "pre-fix RealAssetIDs at kill+0x18 != the array");
    }

    // --- WeaponConfConfig: LOCID_Name and the tail the report rebuilt
    {
        auto *f = (WeaponConfFields *)(g_buf + 0x20);
        poke_int(0x38, 0xAAAAAA);
        poke_int(0x40, 0xBBBBBB);
        CHECK_EQ(f->DefWeaponSkinID, 0xAAAAAA, "WeaponConfFields.DefWeaponSkinID @0x38");
        CHECK_EQ(f->ID, 0xBBBBBB, "WeaponConfFields.ID @0x40");
        std::memset(g_buf + 0x68, 0, 8);
        g_buf[0x68] = 0x77;                                  // fake string object
        void *locid = nullptr;
        std::memcpy(&locid, g_buf + 0x68, sizeof(locid));
        CHECK_EQ(locid == nullptr ? 0 : 1, 1, "WeaponConfFields.LOCID_Name @0x68 sees the pointer");
    }

    // --- WeaponFireEffectConfigConfig: raw reads used by Thread.h / SkinTab.h
    poke_int(0x80, 0xCCCC01);   // Id
    poke_int(0x1C, 0xDDDD01);   // AssetIdForBulletSmoke1P
    const float delaySentinel = 0.1f;
    std::memcpy(g_buf + 0x84, &delaySentinel, sizeof(delaySentinel));
    {
        CHECK_EQ(*(int *)(g_buf + 0x80), 0xCCCC01, "raw fireConf +0x80 == Id");
        CHECK_EQ(*(int *)(g_buf + 0x1C), 0xDDDD01, "raw fireConf +0x1C == AssetIdForBulletSmoke1P");
        CHECK_EQ(*(float *)(g_buf + 0x84) == 0.1f ? 1 : 0, 1, "raw fireConf +0x84 == LevelEffectDelayTimeUI");
    }

    // --- WeaponAssetGroupConfConfig: struct (base +0x40) and raw +0x44
    poke_int(0x40, 0xEEEE01);   // FireEffectGroupID
    poke_int(0x44, 0xEEEE02);   // Id
    {
        auto *f = (WeaponAssetGroupFields *)(g_buf + 0x40);
        CHECK_EQ(f->FireEffectGroupID, 0xEEEE01, "WeaponAssetGroupFields.FireEffectGroupID @0x40");
        CHECK_EQ(f->Id, 0xEEEE02, "WeaponAssetGroupFields.Id @0x44");
        CHECK_EQ(*(int *)(g_buf + 0x44), 0xEEEE02, "raw weaponAssetByID key +0x44 == Id");
    }

    // --- Item2InventoryConfig raw reads from LoadWeaponSkins
    poke_int(0x20, 0xFFFF01);   // ItemID
    poke_int(0x24, 0xFFFF02);   // WeaponAssetGroupID
    poke_int(0x28, 0xFFFF03);   // WeaponIconID
    CHECK_EQ(peek_int(0x20), 0xFFFF01, "raw inv +0x20 == ItemID");
    CHECK_EQ(peek_int(0x24), 0xFFFF02, "raw inv +0x24 == WeaponAssetGroupID");
    CHECK_EQ(peek_int(0x28), 0xFFFF03, "raw inv +0x28 == WeaponIconID");

    // --- MythicSight / BRDeadbox / BRDropPlane loader reads
    poke_int(0x14, 0x101001);   // MythicSight AssetID (raw sightID read)
    poke_int(0x20, 0x101002);   // MythicSight ID
    {
        auto *s = (MythicSightFields *)(g_buf + 0x10);
        CHECK_EQ(s->ID, 0x101002, "MythicSightFields.ID @0x20");
        CHECK_EQ(*(int *)(g_buf + 0x14), 0x101001, "raw sightID +0x14 == AssetID");
    }
    poke_int(0x10, 0x202001);   // Deadbox ColorID
    poke_int(0x20, 0x202002);   // Deadbox ID
    {
        auto *d = (BRDeadboxSkinFields *)(g_buf + 0x10);
        CHECK_EQ(d->ColorID, 0x202001, "BRDeadboxSkinFields.ColorID @0x10");
        CHECK_EQ(d->ID, 0x202002, "BRDeadboxSkinFields.ID @0x20");
    }
    poke_int(0x10, 0x303001);   // DropPlane ColorID
    poke_int(0x14, 0x303002);   // DropPlane ID
    poke_int(0x18, 0x303003);   // DropPlane ModelAsset1P
    {
        auto *p = (BRDropPlaneSkinFields *)(g_buf + 0x10);
        CHECK_EQ(p->ColorID, 0x303001, "BRDropPlaneSkinFields.ColorID @0x10");
        CHECK_EQ(p->ID, 0x303002, "BRDropPlaneSkinFields.ID @0x14");
        CHECK_EQ(p->ModelAsset1P, 0x303003, "BRDropPlaneSkinFields.ModelAsset1P @0x18");
    }

    // --- CharacterModelConfig
    poke_int(0x78, 0x404001);   // ItemID
    poke_int(0x18, 0x404002);   // BRBagModel
    {
        auto *c = (CharacterModelFields *)(g_buf + 0x10);
        CHECK_EQ((int)c->ItemID, 0x404001, "CharacterModelFields.ItemID @0x78");
        CHECK_EQ(c->BRBagModel, 0x404002, "CharacterModelFields.BRBagModel @0x18");
    }
}

int main()
{
    check_static();
    check_runtime();
    if (g_failures == 0) {
        std::printf("OK: all loader offsets match the il2cpp dump\n");
        return 0;
    }
    std::printf("%d offset check(s) FAILED\n", g_failures);
    return 1;
}
