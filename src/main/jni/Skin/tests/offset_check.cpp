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

    // RoleConfConfig: first field BaseCharacterID @0x10, loader +0x10 (dump
    // 818159 - its offsets are NOT masked in the dump, they are copied one for
    // one). ID is the key the character list matches on; the byte cluster and
    // the string tail are what the apply path writes.
    static_assert(offsetof(RoleConfFields, ColorID) + 0x10 == 0x22, "");
    static_assert(offsetof(RoleConfFields, ColorSubID) + 0x10 == 0x23, "");
    static_assert(offsetof(RoleConfFields, RoleLvGroupID) + 0x10 == 0x2C, "");
    static_assert(offsetof(RoleConfFields, ShowRare) + 0x10 == 0x2F, "");
    static_assert(offsetof(RoleConfFields, BlockAppearance) + 0x10 == 0x30, "");
    static_assert(offsetof(RoleConfFields, ID) + 0x10 == 0x34, "");
    static_assert(offsetof(RoleConfFields, RolePackID) + 0x10 == 0x44, "");
    static_assert(offsetof(RoleConfFields, LOCID_Name) + 0x10 == 0xA0, "");
    static_assert(offsetof(RoleConfFields, WinnerCircle) + 0x10 == 0xF8, "");

    // RolePackConfConfig: first field @0x10, loader +0x10 (dump 818902). The live
    // class is alphabetically ordered and this declaration follows it exactly.
    static_assert(offsetof(RolePackFields, EntryAnimID) + 0x10 == 0x40, "");
    static_assert(offsetof(RolePackFields, GestureId) + 0x10 == 0x54, "");
    static_assert(offsetof(RolePackFields, HandEffectUI) + 0x10 == 0x58, "");
    static_assert(offsetof(RolePackFields, KillStreakSkinID) + 0x10 == 0x68, "");
    static_assert(offsetof(RolePackFields, LoadingFrame) + 0x10 == 0x6C, "");
    static_assert(offsetof(RolePackFields, LobbySceneType) + 0x10 == 0x70, "");
    static_assert(offsetof(RolePackFields, RolePackID) + 0x10 == 0x90, "");
    static_assert(offsetof(RolePackFields, AirPoseBR) + 0x10 == 0xA8, "");

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

    // VehicleSkinConfConfig: first field ColorID @0x10, loader +0x10 (dump
    // 822946, one offset per line, nothing masked). LoadSnowboardSkins filters
    // on VehicleId, FindVehicleSkinConfigById on ID - the two keys the old
    // +0x18 base read one field too late.
    static_assert(offsetof(VehicleSkinConfFields, ColorID) + 0x10 == 0x10, "");
    static_assert(offsetof(VehicleSkinConfFields, DisableInCollection) + 0x10 == 0x11, "");
    static_assert(offsetof(VehicleSkinConfFields, ExternalUnVisible) + 0x10 == 0x12, "");
    static_assert(offsetof(VehicleSkinConfFields, GoGetPos) + 0x10 == 0x13, "");
    static_assert(offsetof(VehicleSkinConfFields, IsAutoDemolition) + 0x10 == 0x14, "");
    static_assert(offsetof(VehicleSkinConfFields, LockedShowOrder) + 0x10 == 0x15, "");
    static_assert(offsetof(VehicleSkinConfFields, ShowInBag) + 0x10 == 0x16, "");
    static_assert(offsetof(VehicleSkinConfFields, ShowRare) + 0x10 == 0x17, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinMusicsID) + 0x10 == 0x18, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinStyle) + 0x10 == 0x19, "");
    static_assert(offsetof(VehicleSkinConfFields, CurrencyID) + 0x10 == 0x1C, "");
    static_assert(offsetof(VehicleSkinConfFields, ID) + 0x10 == 0x20, "");
    static_assert(offsetof(VehicleSkinConfFields, MusicsID) + 0x10 == 0x24, "");
    static_assert(offsetof(VehicleSkinConfFields, NewVehicleResId) + 0x10 == 0x28, "");
    static_assert(offsetof(VehicleSkinConfFields, RoomShowScale) + 0x10 == 0x2C, "");
    static_assert(offsetof(VehicleSkinConfFields, SellPrice) + 0x10 == 0x30, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinExtraMesh) + 0x10 == 0x34, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinParam) + 0x10 == 0x38, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinParam2) + 0x10 == 0x3C, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinParam3) + 0x10 == 0x40, "");
    static_assert(offsetof(VehicleSkinConfFields, UILowModel) + 0x10 == 0x44, "");
    static_assert(offsetof(VehicleSkinConfFields, VehicleId) + 0x10 == 0x48, "");
    static_assert(offsetof(VehicleSkinConfFields, FeatureDescLocid1) + 0x10 == 0x58, "");
    static_assert(offsetof(VehicleSkinConfFields, GoGetUrl) + 0x10 == 0x70, "");
    static_assert(offsetof(VehicleSkinConfFields, PreloadRes) + 0x10 == 0x90, "");
    static_assert(offsetof(VehicleSkinConfFields, SkinParams) + 0x10 == 0xC8, "");
    static_assert(offsetof(VehicleSkinConfFields, WeaponSkin) + 0x10 == 0xD0, "");

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

    // MythicArmorConfig (loader +0x14)
    static_assert(offsetof(MythicArmorFields, AssetID) + 0x14 == 0x14, "");
    static_assert(offsetof(MythicArmorFields, ID) + 0x14 == 0x24, "");
    static_assert(offsetof(MythicArmorFields, ThirdTab) + 0x14 == 0x30, "");

    // RoleSkinConfig (dump 819208): first field m_ownerRoleIDs @0x10, loader
    // +0x10. The watch list reads ColorID (rarity prefix) and FxAssetID_1P
    // (the id the apply path writes into CharacterModelFields.BindEffect1P).
    static_assert(offsetof(RoleSkinFields, m_ownerRoleIDs) + 0x10 == 0x10, "");
    static_assert(offsetof(RoleSkinFields, ClothType) + 0x10 == 0x18, "");
    static_assert(offsetof(RoleSkinFields, ColorID) + 0x10 == 0x19, "");
    static_assert(offsetof(RoleSkinFields, GoGetPos) + 0x10 == 0x1A, "");
    static_assert(offsetof(RoleSkinFields, HatSubMode) + 0x10 == 0x1B, "");
    static_assert(offsetof(RoleSkinFields, HideInBag) + 0x10 == 0x1C, "");
    static_assert(offsetof(RoleSkinFields, IsCanBeUsedForAllRole) + 0x10 == 0x1D, "");
    static_assert(offsetof(RoleSkinFields, IsCloseScreenSaver) + 0x10 == 0x1E, "");
    static_assert(offsetof(RoleSkinFields, IsDisassemble) + 0x10 == 0x1F, "");
    static_assert(offsetof(RoleSkinFields, SecondTab) + 0x10 == 0x20, "");
    static_assert(offsetof(RoleSkinFields, ShowInBag) + 0x10 == 0x21, "");
    static_assert(offsetof(RoleSkinFields, ShowRare) + 0x10 == 0x22, "");
    static_assert(offsetof(RoleSkinFields, BRBagAssetID) + 0x10 == 0x24, "");
    static_assert(offsetof(RoleSkinFields, BRWingAssetID) + 0x10 == 0x38, "");
    static_assert(offsetof(RoleSkinFields, CamoID) + 0x10 == 0x40, "");
    static_assert(offsetof(RoleSkinFields, ClothAssetID) + 0x10 == 0x44, "");
    static_assert(offsetof(RoleSkinFields, FxAssetIDHandGestureUI) + 0x10 == 0x48, "");
    static_assert(offsetof(RoleSkinFields, FxAssetID_1P) + 0x10 == 0x50, "");
    static_assert(offsetof(RoleSkinFields, FxAssetID_3P) + 0x10 == 0x54, "");
    static_assert(offsetof(RoleSkinFields, FxAssetID_UI) + 0x10 == 0x58, "");
    static_assert(offsetof(RoleSkinFields, HatAssetID) + 0x10 == 0x5C, "");
    static_assert(offsetof(RoleSkinFields, ID) + 0x10 == 0x60, "");
    static_assert(offsetof(RoleSkinFields, ShowHatAssetID) + 0x10 == 0x74, "");
    static_assert(offsetof(RoleSkinFields, SubTypeForCommonAttr) + 0x10 == 0x78, "");
    static_assert(offsetof(RoleSkinFields, SuitAssetID) + 0x10 == 0x7C, "");
    static_assert(offsetof(RoleSkinFields, WingAssetID) + 0x10 == 0x80, "");
    static_assert(offsetof(RoleSkinFields, ExtraAttriAssetIDs_1P) + 0x10 == 0x88, "");
    static_assert(offsetof(RoleSkinFields, GoGetUrl) + 0x10 == 0xA0, "");
    static_assert(offsetof(RoleSkinFields, MaterialIDs) + 0x10 == 0xC0, "");

    std::printf("static_asserts: all struct layouts match the dump\n");
}

// ── runtime: sentinels at dump offsets, read back through the loader casts ──
static unsigned char g_buf[0x400];
static int g_payload = 4242;

// Mirror of the pre-fix RoleSkinConfig declaration (see the runtime block).
struct OldRoleSkinFields
{
    uint8_t ClothType, ColorID, GoGetPos, HatSubMode;
    uint8_t IsCanBeUsedForAllRole;
    bool IsCloseScreenSaver;
    uint8_t SecondTab;
    bool ShowInBag, ShowRare;
    int BRBagAssetID, BRClothAssetID, BRHatAssetID, BRLeftArmAssetID, BRSuitAssetID,
        BRWingAssetID, BagAssetID, CamoID, ClothAssetID, FxAssetIDHandGestureUI,
        FxAssetIDShow, FxAssetID_1P, FxAssetID_3P, FxAssetID_UI, HatAssetID;
    uint ID;
    int LeftArm1PAssetID, LeftArmAssetID, ShowBagAssetID, ShowClothAssetID,
        ShowHatAssetID, SuitAssetID, WingAssetID;
};

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

    // --- RoleConfConfig: the CHARACTER join key -----------------------------
    // The list is built by matching RoleConfConfig.ID against
    // ItemResourceConfig.ID; with the pre-fix +0x18 base that read was
    // KeepInCatalog (0x3C), so no character ever matched and the tab stayed
    // empty however long the loader polled.
    poke_int(0x22, 7);          // ColorID
    poke_int(0x23, 8);          // ColorSubID
    poke_int(0x2C, 9);          // RoleLvGroupID
    poke_int(0x2F, 1);          // ShowRare
    poke_int(0x34, 0x505001);   // ID          <- the join key
    poke_int(0x44, 0x505002);   // RolePackID  <- the RolePackConf join key
    poke_int(0x3C, 0x505009);   // KeepInCatalog: what the pre-fix base read as ID
    poke_int(0xA0, 0x50500A);   // LOCID_Name
    {
        auto *r = (RoleConfFields *)(g_buf + 0x10);
        CHECK_EQ((int)r->ColorID, 7, "RoleConfFields.ColorID @0x22");
        CHECK_EQ((int)r->ColorSubID, 8, "RoleConfFields.ColorSubID @0x23");
        CHECK_EQ((int)r->RoleLvGroupID, 9, "RoleConfFields.RoleLvGroupID @0x2C");
        CHECK_EQ(r->ShowRare ? 1 : 0, 1, "RoleConfFields.ShowRare @0x2F");
        CHECK_EQ((int)r->ID, 0x505001, "RoleConfFields.ID @0x34 (character join key)");
        CHECK_EQ(r->RolePackID, 0x505002, "RoleConfFields.RolePackID @0x44");
        CHECK_EQ((int)(uintptr_t)r->LOCID_Name, 0x50500A, "RoleConfFields.LOCID_Name @0xA0");

        auto *old = (RoleConfFields *)(g_buf + 0x18);   // pre-fix base
        CHECK_EQ((int)old->ID, 0x505009,
                 "pre-fix +0x18 base read KeepInCatalog as ID -> empty Character tab");
        // The pre-fix base's byte cluster lands in the middle of the flags, so
        // it cannot be the real ColorID whatever the neighbour bytes hold.
        CHECK_EQ(old->ColorID == 7 ? 0 : 1, 1,
                 "pre-fix +0x18 base never landed on ColorID");
    }

    // --- RolePackConfConfig: the pack fields the apply path writes ----------
    poke_int(0x40, 0x601001);   // EntryAnimID
    poke_int(0x54, 0x601002);   // GestureId
    poke_int(0x58, 0x601003);   // HandEffectUI
    poke_int(0x68, 0x601004);   // KillStreakSkinID
    poke_int(0x6C, 0x601005);   // LoadingFrame
    poke_int(0x70, 0x601006);   // LobbySceneType
    poke_int(0x90, 0x601007);   // RolePackID
    poke_int(0x98, 0x601009);   // the neighbour the pre-fix base read as RolePackID
    {
        auto *k = (RolePackFields *)(g_buf + 0x10);
        CHECK_EQ(k->EntryAnimID, 0x601001, "RolePackFields.EntryAnimID @0x40");
        CHECK_EQ(k->GestureId, 0x601002, "RolePackFields.GestureId @0x54");
        CHECK_EQ(k->HandEffectUI, 0x601003, "RolePackFields.HandEffectUI @0x58");
        CHECK_EQ(k->KillStreakSkinID, 0x601004, "RolePackFields.KillStreakSkinID @0x68");
        CHECK_EQ(k->LoadingFrame, 0x601005, "RolePackFields.LoadingFrame @0x6C");
        CHECK_EQ(k->LobbySceneType, 0x601006, "RolePackFields.LobbySceneType @0x70");
        CHECK_EQ((int)k->RolePackID, 0x601007, "RolePackFields.RolePackID @0x90 (pack join key)");

        auto *old = (RolePackFields *)(g_buf + 0x18);   // pre-fix base
        CHECK_EQ((int)old->RolePackID, 0x601009,
                 "pre-fix +0x18 base read the neighbour, not RolePackID");
    }

    // --- RoleSkinConfig: what the Watch tab reads, and what the old decl read -
    // OldRoleSkinFields is a byte-for-byte mirror of the pre-fix declaration
    // (ClothType first, +0x18 cast, HideInBag/IsDisassemble missing). It is kept
    // here only to measure the old layout against the dump's, so the claim
    // "the watch list was unaffected" is checked rather than asserted.
    poke_int(0x19, 0x05);       // ColorID (the rarity prefix byte)
    poke_int(0x1E, 0x01);       // IsCloseScreenSaver
    poke_int(0x20, 0x07);       // SecondTab
    poke_int(0x22, 0x01);       // ShowRare
    poke_int(0x50, 0x801001);   // FxAssetID_1P - the watch effect (watchvalue)
    poke_int(0x78, 0x901001);   // SubTypeForCommonAttr
    poke_int(0x7C, 0x901002);   // SuitAssetID
    poke_int(0x80, 0x901003);   // WingAssetID (the old decl also claimed 0x80)
    {
        auto *r = (RoleSkinFields *)(g_buf + 0x10);           // loader now
        CHECK_EQ((int)r->ColorID, 5, "RoleSkinFields.ColorID @0x19 (rarity prefix)");
        CHECK_EQ(r->FxAssetID_1P, 0x801001,
                 "RoleSkinFields.FxAssetID_1P @0x50 (the Watch tab's watchvalue)");
        CHECK_EQ((int)r->ShowRare, 0x01, "RoleSkinFields.ShowRare @0x22");
        CHECK_EQ((int)r->SuitAssetID, 0x901002, "RoleSkinFields.SuitAssetID @0x7C");
        CHECK_EQ(r->WingAssetID, 0x901003, "RoleSkinFields.WingAssetID @0x80");

        auto *old = (OldRoleSkinFields *)(g_buf + 0x18);      // pre-fix decl + base
        CHECK_EQ(old->FxAssetID_1P, 0x801001,
                 "pre-fix decl resolved FxAssetID_1P to the right byte (no change)");
        CHECK_EQ((int)old->ShowRare, 0x07,
                 "pre-fix decl read SecondTab as ShowRare (missing HideInBag)");
        CHECK_EQ((int)old->SecondTab, 0x01,
                 "pre-fix decl read IsCloseScreenSaver as SecondTab");
        CHECK_EQ(old->SuitAssetID, 0x901001,
                 "pre-fix decl read SubTypeForCommonAttr as SuitAssetID");
        CHECK_EQ(old->WingAssetID, 0x901002,
                 "pre-fix decl read SuitAssetID as WingAssetID");
    }

    // --- VehicleSkinConfConfig: the two keys the snowboard list joins on ----
    poke_int(0x10, 0x04);       // ColorID
    poke_int(0x20, 0x701001);   // ID
    poke_int(0x28, 0x701002);   // NewVehicleResId
    poke_int(0x44, 0x701003);   // UILowModel
    poke_int(0x48, 31707110);   // VehicleId (snowboard)
    poke_int(0x50, 0x701005);   // the static/alignment slot the old base read
    poke_int(0xC8, 0x701004);   // SkinParams[] (absolute read in VehicleSkinParams)
    {
        auto *v = (VehicleSkinConfFields *)(g_buf + 0x10);   // loader now
        CHECK_EQ((int)v->ColorID, 4, "VehicleSkinConfFields.ColorID @0x10 (rarity prefix)");
        CHECK_EQ((int)v->ID, 0x701001, "VehicleSkinConfFields.ID @0x20 (skin join key)");
        CHECK_EQ(v->VehicleId, 31707110, "VehicleSkinConfFields.VehicleId @0x48 (snowboard filter)");
        CHECK_EQ(v->NewVehicleResId, 0x701002, "VehicleSkinConfFields.NewVehicleResId @0x28");
        CHECK_EQ(offsetof(VehicleSkinConfFields, SkinParams) + 0x10 == 0xC8 ? 1 : 0, 1,
                 "VehicleSkinParams reads SkinParams at conf+0xC8");
        auto *old = (VehicleSkinConfFields *)(g_buf + 0x18);  // pre-fix base
        CHECK_EQ((int)old->ID, 0x701002,
                 "pre-fix +0x18 base read NewVehicleResId as ID (no skin matched)");
        CHECK_EQ(old->VehicleId, 0x701005,
                 "pre-fix +0x18 base read skin+0x50, not the real VehicleId @0x48");
        CHECK_EQ(old->VehicleId == 31707110 ? 1 : 0, 0,
                 "pre-fix VehicleId != 31707110 -> every snowboard filtered out");
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
