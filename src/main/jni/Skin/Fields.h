#include <cstdint>
#include <cctype>
#include <map>
#include <string>
#include <array>
#include <unordered_map>
#include <vector>

extern uintptr_t Item2InventoryAddress;
extern uintptr_t WeaponConfAddress;
extern uintptr_t WeaponConfExtraAddress;
extern uintptr_t WeaponFireEffectAddress;
extern uintptr_t CharacterModelAddress;
extern uintptr_t BRDeadboxSkinAddress;
extern uintptr_t BRDropPlaneSkinAddress;
extern uintptr_t WeaponAssetGroupAddress;
extern uintptr_t MythicArmorConfigAddress;
extern uintptr_t MythicSightConfigAddress;
extern uintptr_t KillEffectItemConfConfigAddress;
extern uintptr_t WeaponSkinAddress;
extern uintptr_t WeaponCamosAddress;
extern uintptr_t ItemResourceAddress;
extern uintptr_t RoleConfAddress;
extern uintptr_t RoleSkinAddress;
extern uintptr_t RolePackConfAddress;
extern uintptr_t WeaponConfName;
extern uintptr_t GetDropPlaneName;
extern uintptr_t GetVehicleSkinName;
extern uintptr_t GetNameRoleSkin;
extern uintptr_t GetRoleConfName;
extern uintptr_t GetDeadBoxIDAddress;
extern uintptr_t UnlockedCamo;
extern uintptr_t VehicleSkinConfAddress;
extern uintptr_t VehicleItemConfAddress;
extern uintptr_t VehicleSkinHelperChangeVehicleSkinIdAddress;
extern uintptr_t VehicleSkinHelperChangeVehicleSkinConfAddress;
extern uintptr_t VehicleSkisSetupSkinAddress;
extern uintptr_t VehicleSkisChangeSkinIdAddress;
extern uintptr_t VehicleSkisChangeSkinConfAddress;
extern uintptr_t VehicleSkisBuildVehicleModelAddress;
extern uintptr_t VehicleSkisLoadModelCompleteAddress;
extern uintptr_t VehicleSkisApplySkinAddress;
extern std::vector<void *> itemInventoryInstance;
extern std::vector<void *> weaponExtraInstance;
extern std::vector<void *> weaponFireEffectInstance;
extern std::vector<void *> weaponConfInstance;
extern std::vector<void *> weaponAssetGroupInstance;
extern std::vector<void *> mythicArmorInstance;
extern std::vector<void *> mythicSightInstance;
extern std::vector<void *> killEffectItemInstance;
extern std::vector<void *> weaponSkinConfigInstance;
extern std::vector<void *> weaponCamosConfigInstance;
extern std::vector<void *> itemResourceConfigInstance;
extern std::vector<void *> CharacterModelConfigInstance;
extern std::vector<void *> RoleConfConfigInstance;
extern std::vector<void *> RoleSkinConfigInstance;
extern std::vector<void *> RolePackConfConfigInstance;
extern std::vector<void *> BRDeadboxSkinConfigInstance;
extern std::vector<void *> BRDropPlaneSkinConfigInstance;
extern std::vector<void *> vehicleSkinConfigInstance;
extern std::vector<void *> vehicleItemConfigInstance;
extern std::unordered_map<int, int> activeVehicleSkins;
extern std::unordered_map<int, int> activeVehicleSkinsById;
extern std::unordered_map<int, void*> activeVehicleSkinConfs;
extern std::unordered_map<std::string, bool> sBool;
extern std::unordered_map<int, int> activeKillEffects;
extern std::unordered_map<int, int> activeBulletTrackEffects;
extern std::unordered_map<int, int> activeWeaponFireEffects;
extern std::unordered_map<int, int> activeWeaponBrocast;
extern int g_selectedTargetCharIndex;

struct TargetChar {
    std::string name;
    int traitor1p;
    int traitor3p;
    int itemID;
    int roleID;
    int rolepackID;
};

extern std::vector<TargetChar> g_targetCharacters;

struct itemInfo
{
    std::string itemName;
    std::array<int, 4> Item2Inventory;
    std::array<int, 5> WeaponConf;
    std::array<int, 5> WeaponExtra;
    std::array<int, 3> WeaponAsset;
    std::array<int, 3> ItemResInt;
    std::array<uint16_t, 4> ItemRes;
};
extern std::vector<itemInfo> itemData;

struct charInfo
{
    std::string charName;
    std::array<int, 12> charModel;
    std::array<int, 3> charRes;
    std::array<uint16_t, 4> ItemRes2;
    std::array<int, 8> charRole;
    std::array<void *, 1> charRole2;
    std::array<int, 6> charPack;
};
extern std::vector<charInfo> charData;

struct watcher
{
    std::string watchname;
    int watchvalue;
};
extern std::vector<watcher> watch;

struct deadbox
{
    std::string deadname;
    std::array<int, 6> dead;
};
extern std::vector<deadbox> deadboxF;

struct planeID
{
    std::string planename;
    std::array<int, 6> plane;
};
extern std::vector<planeID> dropplane;

struct snowboardInfo
{
    std::string skinName;
    int skinId;
};
extern std::vector<snowboardInfo> snowboardData;

// ── Camo catalogue ──────────────────────────────────────────────────────────
// One entry per camo the GAME has, not a hand-written list: LoadCamos() in
// Skin/Thread.h walks every WeaponCamosConfig row the ctor hook captured and
// inserts each distinct camo here.
//
// WeaponCamosConfig (dump 823828) is a config table of its own:
//     CamoType 0x10, ColorID 0x11, NeedShowInEndGame 0x12, BelongWeaponID 0x14,
//     CamoItemID 0x18, ConceptID 0x1C, DMZseason 0x20, KeepInCatalog 0x24,
//     TextureAssetID 0x28, TextureSpecialAssetID 0x2C, LOCID_Describe 0x30,
//     LOCID_Name 0x38, Mission_id 0x40, Personal_mission_id 0x48,
//     SmallIconSpriteName 0x50
// (the loader's +0x10 cast, exactly what Skin/Camo.h already uses).
//
// The table carries one row per (camo, weapon) pair - that is why the game's own
// lookup is `WeaponCamosConfig.GetCamosConfig(uint camoID, int weaponID = 0)` -
// so entries are de-duplicated by CamoItemID. That id is what the game equips:
// it is the value the Gold/Diamond/... checkboxes have always written into
// WeaponConfConfig.DefWeaponSkinID, and it is what ApplyCamoToEveryGun() writes
// for every weapon conf now.
struct camoInfo
{
    std::string camoName;   // the game's own name; "Camo <id>" when it is not ready
    int camoItemID;         // CamoItemID - the id the game equips
    int applyID;            // what DefWeaponSkinID is set to (see below)
    int textureAssetID;     // TextureAssetID - identifies the 7 proven camos
    int belongWeaponID;     // 0 = usable on every weapon
};
extern std::vector<camoInfo> camoData;

// Case-insensitive lower of a name, used for matching and for the ordering.
inline std::string CamoNameLower(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char ch : s)
        out.push_back((char)::tolower((unsigned char)ch));
    return out;
}

// "Camo 1234" is what a row is called until the game's name table resolves.
inline bool IsPlaceholderCamoName(const std::string &name)
{
    return name.rfind("Camo ", 0) == 0;
}

// The camo ids this build has PROVEN: they are the constants the original
// template shipped (Gold 0x1D37F755, Platinum 0x1D37F756, Damascus 0x1D37F757,
// Diamond 0x1D37F758, Aether Crystal 0x1D37F77D, Red Sprite 0x1D37F77E,
// Glacial Ripple 0x1D37F77F). A camo whose name matches one of these keeps
// writing the id that already worked on device; every other camo uses the id
// from the game's own table. `src` in the Camo tab's diagnostic row says which
// one was used, and prints both ids for a known camo, so a mismatch is visible
// instead of guessed at.
inline int KnownCamoIdForName(const std::string &name)
{
    const std::string lower = CamoNameLower(name);

    // Whole-WORD match, not a substring one: the real names carry extra words
    // ("Aether Crystal", "Red Sprite", "Glacial Ripple", or a "<weapon> gold"
    // prefix), while the table also holds camos that merely start with a keyword
    // ("Golden Emerald"), which a plain substring test read as Gold.
    auto is = [&](const char *want) {
        const std::string w = want;
        for (size_t pos = lower.find(w); pos != std::string::npos;
             pos = lower.find(w, pos + 1)) {
            const size_t end = pos + w.size();
            const bool startsWord = (pos == 0) ||
                !std::isalnum((unsigned char)lower[pos - 1]);
            const bool endsWord = (end == lower.size()) ||
                !std::isalnum((unsigned char)lower[end]);
            if (startsWord && endsWord)
                return true;
        }
        return false;
    };
    if (is("gold"))       return 0x1D37F755;
    if (is("platinum"))   return 0x1D37F756;
    if (is("damascus"))   return 0x1D37F757;
    if (is("diamond"))    return 0x1D37F758;
    if (is("aether"))     return 0x1D37F77D;
    if (is("red sprite")) return 0x1D37F77E;
    if (is("glacial"))    return 0x1D37F77F;
    return 0;
}

// The display name of a camo id this build has PROVEN (the same seven constants
// KnownCamoIdForName() maps). The game's table names the SAME camo once per
// weapon - "Gold", "KRM Gold", "AK117 Gold", ... - and every one of those rows
// writes the SAME proven id, i.e. they are one row as far as the user is
// concerned. Naming them all canonically is what lets InsertCamo() collapse them
// into a single entry instead of the duplicate list rows the search used to show.
inline std::string KnownCamoNameForId(int id)
{
    switch ((unsigned)id) {
        case 0x1D37F755: return "Gold";
        case 0x1D37F756: return "Platinum";
        case 0x1D37F757: return "Damascus";
        case 0x1D37F758: return "Diamond";
        case 0x1D37F77D: return "Aether Crystal";
        case 0x1D37F77E: return "Red Sprite";
        case 0x1D37F77F: return "Glacial Ripple";
        default:         return std::string();
    }
}

// Add one camo row: ignored when it would be a duplicate, otherwise inserted in
// case-insensitive name order so the list stays stable while the table fills in.
//
// Two rows are the SAME row for the user when they would show the same name AND
// write the same id:
//   * same CamoItemID      - the table's own repeat (one row per weapon);
//   * same (name, applyID) - two different table rows of one proven camo family
//                            ("Gold" + "KRM Gold"), which the canonical name above
//                            turns into one name so they collide here.
// Rows that only LOOK similar stay separate: "Golden Emerald" resolves to its own
// id, so it shares neither name nor id with Gold.
//
// A second row for a known id upgrades what the first one could not answer yet
// (its real name, its texture asset id, and therefore whether a proven id applies).
inline void InsertCamo(int camoItemID, const std::string &name,
                       int textureAssetID, int belongWeaponID)
{
    if (camoItemID <= 0 || name.empty())
        return;

    const int known = KnownCamoIdForName(name);
    const int applyID = known != 0 ? known : camoItemID;
    // A proven camo always shows its own name (Gold, Diamond, ...), never the
    // per-weapon spelling the table happens to carry.
    const std::string displayName = known != 0 ? KnownCamoNameForId(known) : name;
    const std::string key = CamoNameLower(displayName);

    for (auto &existing : camoData) {
        const bool sameRow = existing.camoItemID == camoItemID;
        const bool sameThing = existing.applyID == applyID &&
                               CamoNameLower(existing.camoName) == key;
        if (!sameRow && !sameThing)
            continue;
        if (existing.belongWeaponID != 0 && belongWeaponID == 0)
            existing.belongWeaponID = 0;
        if (existing.textureAssetID == 0 && textureAssetID != 0)
            existing.textureAssetID = textureAssetID;
        if (IsPlaceholderCamoName(existing.camoName) && !IsPlaceholderCamoName(displayName)) {
            existing.camoName = displayName;
            existing.applyID = applyID;
        }
        return;
    }

    camoInfo entry;
    entry.camoName = displayName;
    entry.camoItemID = camoItemID;
    entry.textureAssetID = textureAssetID;
    entry.belongWeaponID = belongWeaponID;
    entry.applyID = applyID;

    auto at = camoData.begin();
    while (at != camoData.end() && CamoNameLower(at->camoName) <= key)
        ++at;
    camoData.insert(at, entry);
}

extern char searchQuery[256];
extern std::string ToLower(std::string str);
extern uintptr_t getRealOffset(uintptr_t offset);

// RoleSkinConfig (dump 819208): first field m_ownerRoleIDs @0x10, so the loader
// base is +0x10. The old declaration started at ClothType with a +0x18 cast:
// that happened to line up for the byte cluster and most asset ids, but FxAssetID_1P
// landed on the real FxAssetID_UI (0x58 instead of 0x50), so the Watch tab stored
// the UI effect id and the apply path wrote it into BindEffect1P. The old decl was
// also missing HideInBag + IsDisassemble, which shifted every flag from
// IsCanBeUsedForAllRole on.
struct RoleSkinFields
{
    void *m_ownerRoleIDs;          // 0x10 @0x00 (List<uint>)
    uint8_t ClothType;             // 0x18 @0x08
    uint8_t ColorID;               // 0x19 @0x09
    uint8_t GoGetPos;              // 0x1A @0x0A
    uint8_t HatSubMode;            // 0x1B @0x0B
    bool HideInBag;                // 0x1C @0x0C
    uint8_t IsCanBeUsedForAllRole; // 0x1D @0x0D
    bool IsCloseScreenSaver;       // 0x1E @0x0E
    uint8_t IsDisassemble;         // 0x1F @0x0F
    uint8_t SecondTab;             // 0x20 @0x10
    bool ShowInBag;                // 0x21 @0x11
    bool ShowRare;                 // 0x22 @0x12
    char pad_23[1];                // 0x23 (@0x13)
    int BRBagAssetID;              // 0x24 @0x14
    int BRClothAssetID;            // 0x28 @0x18
    int BRHatAssetID;              // 0x2C @0x1C
    int BRLeftArmAssetID;          // 0x30 @0x20
    int BRSuitAssetID;             // 0x34 @0x24
    int BRWingAssetID;             // 0x38 @0x28
    int BagAssetID;                // 0x3C @0x2C
    int CamoID;                    // 0x40 @0x30
    int ClothAssetID;              // 0x44 @0x34
    int FxAssetIDHandGestureUI;    // 0x48 @0x38
    int FxAssetIDShow;             // 0x4C @0x3C
    int FxAssetID_1P;              // 0x50 @0x40 (the Watch tab's watchvalue)
    int FxAssetID_3P;              // 0x54 @0x44
    int FxAssetID_UI;              // 0x58 @0x48 (what the +0x18 cast used to return)
    int HatAssetID;                // 0x5C @0x4C
    uint ID;                       // 0x60 @0x50
    int LeftArm1PAssetID;          // 0x64 @0x54
    int LeftArmAssetID;            // 0x68 @0x58
    int ShowBagAssetID;            // 0x6C @0x5C
    int ShowClothAssetID;          // 0x70 @0x60
    int ShowHatAssetID;            // 0x74 @0x64
    int SubTypeForCommonAttr;      // 0x78 @0x68
    int SuitAssetID;               // 0x7C @0x6C
    int WingAssetID;               // 0x80 @0x70
    char pad_84[4];                // 0x84 (@0x74) - aligns the arrays to 8
    void *ExtraAttriAssetIDs_1P;   // 0x88 @0x78 (int[])
    void *ExtraAttriAssetIDs_3P;   // 0x90 @0x80 (int[])
    void *ExtraAttriAssetIDs_UI;   // 0x98 @0x88 (int[])
    void *GoGetUrl;                // 0xA0 @0x90
    char pad_98[24];               // 0xA8..0xB7 (@0x98) - LOCID_* are static
    void *MaterialIDs;             // 0xC0 @0xB0 (int[])
};
extern RoleSkinFields *roleskinFields;

struct CharacterModelFields
{
    bool Female;               // 0x10
    uint AvatarAnimationType;  // 0x14
    int BRBagModel;            // 0x18
    int BRHeadModel;           // 0x1C
    int BRLobby;               // 0x20
    int BRModel;               // 0x24
    int BindEffect1P;          // 0x28
    int ChangeClipEffect1P;    // 0x2C
    int DefaultModelID;        // 0x30
    int Guarder1P;             // 0x34
    int Guarder3P;             // 0x38
    int GuarderBagModel;       // 0x3C
    int GuarderHeadModel;      // 0x40
    int GuarderLobby;          // 0x48
    int HeadIgnoreGUPSkin;     // 0x48
    int SpReceiveAnimType;     // 0x4C
    int Traitor1P;             // 0x50
    int Traitor3P;             // 0x54
    float *BagOffset;          // 0x58
    string *Desc;              // 0x60
    float *VehicleOffset;      // 0x68
    int *VestOtherModelID;     // 0x70
    uintptr_t ItemID;          // 0x7C
    uintptr_t LitePackDefault; // 0x80
};
extern CharacterModelFields *characterfields;

// RolePackConfConfig (dump 818902, TypeDefIndex 17228). The live class is
// ALPHABETICALLY ordered, and the struct has to follow that order exactly:
// member offsets are what the compiler produces, so a wrong order (or a missing
// field) silently shifts every member after it. Cast base is 0x10 (the class'
// first field), so the comments below are the declared offsets inside the
// struct while the trailing number is the live offset in the game object.
struct RolePackFields
{
    int mMatchRoomEasterEggType;           // 0x10 @0x10
    uint mMatchRoomEasterEggCharacterID;   // 0x14 @0x14
    String *mMatchRoomEasterEggActionName; // 0x18 @0x18
    bool DisableCustomize;                 // 0x20 @0x20
    bool EntryDefaultAppearance;           // 0x21 @0x21
    bool SkipWinnerCircleAnimation;        // 0x22 @0x22
    bool UseDynamicLobbyScene;             // 0x23 @0x23
    bool UseVFWFeature;                    // 0x24 @0x24
    int AirItem1;                          // 0x28 @0x28
    int AnimationWhenSell;                 // 0x2C @0x2C
    int CabinIndivID;                      // 0x30 @0x30
    int DeadBoxID;                         // 0x34 @0x34
    int DynamicRoomAssetID;                // 0x38 @0x38
    int DynamicRoomUnlockItemID;           // 0x3C @0x3C
    int EntryAnimID;                       // 0x40 @0x40
    int EntryUIEffectAssetID;              // 0x44 @0x44
    int EntryVideoAnimID;                  // 0x48 @0x48
    float EntryVideoPlayTime;              // 0x4C @0x4C
    int EntryVideoTagAnimID;               // 0x50 @0x50
    int GestureId;                         // 0x54 @0x54
    int HandEffectUI;                      // 0x58 @0x58
    int HomeEasterEggAssetID;              // 0x5C @0x5C
    float HomeEasterEggCoolingTime;        // 0x60 @0x60
    int HomeEasterEggType;                 // 0x64 @0x64
    int KillStreakSkinID;                  // 0x68 @0x68
    int LoadingFrame;                      // 0x6C @0x6C
    int LobbySceneType;                    // 0x70 @0x70
    int MVPIndivID;                        // 0x74 @0x74
    int MatchRoomSort;                     // 0x78 @0x78
    int OpenSceneIndivID;                  // 0x7C @0x7C
    int PendantShowID;                     // 0x80 @0x80
    int PetID;                             // 0x84 @0x84
    int PetSwitchEffectAssetID;            // 0x88 @0x88
    int RoleFeatureType;                   // 0x8C @0x8C
    uint RolePackID;                       // 0x90 @0x90
    int SkillSkinID;                       // 0x94 @0x94
    int SkillSkinUI;                       // 0x98 @0x98
    int SpecialSceneFrameGroupAssetID;     // 0x9C @0x9C
    uint TODSceneID;                       // 0xA0 @0xA0
    int VTOLShowAssetID;                   // 0xA4 @0xA4
    String *AirPoseBR;                     // 0xA8 @0xA8
    String *EntryAnim;                     // 0xB0 @0xB0
    String *EntryIdleAnim;                 // 0xB8 @0xB8
    void *EntryIdleItems;                  // 0xC0 @0xC0
    void *EntryItems;                      // 0xC8 @0xC8
    void *EntryItemsAnim;                  // 0xD0 @0xD0
    void *EntryItemsIdleAnim;              // 0xD8 @0xD8
    String *EntryUIAnimationStateName;     // 0xE0 @0xE0
};
extern RolePackFields *packfields;

// RoleConfConfig (dump 818159, TypeDefIndex 17215). Its field offsets are NOT
// masked in the dump and are reproduced here one for one: the byte cluster
// (ArchiveSwitch..ShowRare) is unbroken, BlockAppearance is a 4-byte int (the
// old `bool BlockBack` was not in the class at all and pushed ColorID from
// 0x22 onto 0x23), and every string slot exists, so LOCID_Name really lands on
// 0xA0 instead of 0x98. The character list's join reads Z1->ID, so a wrong
// offset here emptied the whole Character tab.
struct RoleConfFields
{
    uint BaseCharacterID;           // 0x10 @0x10
    uint roleLeftArmID;             // 0x14 @0x14
    uint roleFinalSuitID;           // 0x18 @0x18
    uint roleBasicHologramID;       // 0x1C @0x1C
    uint8_t ArchiveSwitch;          // 0x20 @0x20
    uint8_t ArmorType;              // 0x21 @0x21
    uint8_t ColorID;                // 0x22 @0x22
    uint8_t ColorSubID;             // 0x23 @0x23
    uint8_t DefaultPerk_1;          // 0x24 @0x24
    uint8_t DefaultPerk_2;          // 0x25 @0x25
    uint8_t DefaultPerk_3;          // 0x26 @0x26
    bool DefaultShow;               // 0x27 @0x27
    uint8_t DefaultSkill_1;         // 0x28 @0x28
    bool ExternalUnVisible;         // 0x29 @0x29
    bool IsVFW;                     // 0x2A @0x2A
    uint8_t Loadout_Magnifier_Flag; // 0x2B @0x2B
    uint8_t RoleLvGroupID;          // 0x2C @0x2C
    bool ShowInBag;                 // 0x2D @0x2D
    uint8_t ShowOrder;              // 0x2E @0x2E
    bool ShowRare;                  // 0x2F @0x2F
    int BlockAppearance;            // 0x30 @0x30
    uint ID;                        // 0x34 @0x34
    int JetPackInfoID;              // 0x38 @0x38
    int KeepInCatalog;              // 0x3C @0x3C
    float RTPCMaxValue;             // 0x40 @0x40
    int RolePackID;                 // 0x44 @0x44
    void *AudioSwitch;              // 0x48 @0x48
    void *BRBankNameEX;             // 0x50 @0x50
    void *BasicAudioBank;           // 0x58 @0x58
    void *BasicAudioBank2;          // 0x60 @0x60
    void *BasicAudioBank3;          // 0x68 @0x68
    void *DefaultAudioBank;         // 0x70 @0x70
    void *DefaultAudioBank2;        // 0x78 @0x78
    void *DefaultAudioEventEx;      // 0x80 @0x80
    void *FoleyAudioBank;           // 0x88 @0x88
    void *FoleyAudioNameEx;         // 0x90 @0x90
    void *LOCID_Desc;               // 0x98 @0x98
    void *LOCID_Name;               // 0xA0 @0xA0
    void *LitePackageKeyName;       // 0xA8 @0xA8
    void *MPBankNameEx;             // 0xB0 @0xB0
    void *PVEBankNameEX;            // 0xB8 @0xB8
    void *RTPCName;                 // 0xC0 @0xC0
    void *RoleSkinBox;              // 0xC8 @0xC8
    void *SpecialAudioBank;         // 0xD0 @0xD0
    void *SpecialAudioBank2;        // 0xD8 @0xD8
    void *SpecialAudioEventEx;      // 0xE0 @0xE0
    void *SpecialTips;              // 0xE8 @0xE8
    void *SupportVoiceTypes;        // 0xF0 @0xF0
    uint64_t WinnerCircle;          // 0xF8 @0xF8
};
extern RoleConfFields *roleFields;

struct ItemResourceFields
{
    uint16_t UIMiniSpriteNameIndex;      // +0x10
    uint16_t UISmallSpriteNameIndex;     // +0x12
    uint16_t UISpriteNameIndex;          // +0x14
    uint16_t UISquareSpriteNameIndex;    // +0x16
    int32_t  FxAssetID;                  // +0x18
    int32_t  ID;                         // +0x1C
    int32_t  InventoryModelID;           // +0x20
    int32_t  ModelAssetIDRaw;            // +0x24
    uint64_t AvatarModelID;              // +0x28
};
extern ItemResourceFields *itemFields;

// ---------------------------------------------------------------------------
// Character skin application - one place the UI and the host test both drive.
//
// The character list used to carry this inline in System/UI/SkinTab.h, which is
// how the two bugs below stayed invisible. It is now a plain function over the
// character structs, so Skin/tests/character_apply_check.cpp can build fake
// config rows and assert what gets written.
// ---------------------------------------------------------------------------

// The ids the config rows of the SELECTED target character carry. Charly is the
// default character and the one the ids in the shipped config data disagree on,
// so its three ids stay hardcoded exactly as before.
inline void ResolveTargetIds(const TargetChar &t,
                             int &traitor1p, int &traitor3p,
                             int &itemId, int &roleId, int &packId)
{
    traitor1p = (t.name == "Charly") ? 710001101 : t.traitor1p;
    traitor3p = (t.name == "Charly") ? 710001102 : t.traitor3p;
    itemId    = (t.name == "Charly") ? 100301208 : t.itemID;
    roleId    = (t.name == "Charly") ? 100301208 : t.roleID;
    packId    = t.rolepackID;
}

// Can the character list apply a skin at all right now?
//
// ApplyCharacterSkin() walks each row list and skips the ones that are empty, so
// the apply only needs the tiers a click really reads: the CharacterModel rows
// it writes, plus the ItemResource / RoleConf rows it looks up by the selected
// target's ids. RolePackConf deliberately does NOT belong in this test. The game
// builds those pack configs lazily (a BR lobby usually has none), so requiring
// them meant every click on a character returned before the apply ran - the
// "I choose a character skin and nothing happens" report. Keeping the pack tier
// out of it leaves that list empty and simply skips the pack writes, exactly the
// way the list builder in Skin/Thread.h LoadCharacterSkins() already treats it.
inline bool CharacterApplyReady(const std::vector<void *> &charModels,
                                const std::vector<void *> &itemResources,
                                const std::vector<void *> &roleConfs)
{
    return !charModels.empty() && !itemResources.empty() && !roleConfs.empty();
}

// Copy `getchar`'s own config values onto the character selected in the Target
// Character dropdown.
//
//   getchar     - the values of the character whose skin was clicked (its own
//                 CharacterModelConfig / ItemResourceConfig / RoleConfConfig /
//                 RolePackConfConfig rows, joined by Skin/Thread.h)
//   sel*        - the ids of the SELECTED target, from ResolveTargetIds()
//
// ONLY rows belonging to the selected target are written. The old version OR-ed
// Charly's ids (710001101 / 100301208) into every match, so choosing any
// character still wrote Charly's config - "I picked Ghost - Stealth and it
// applied on Charly".
//
// Every write is guarded with != 0, the same discipline the weapon-skin path in
// SkinTab.h uses: these records belong to the game, and writing the zero of an
// unmatched value is what blanks a card in the loadout.
//
// The four UI sprite indices come from ItemRes2 - the uint16_t[4] Skin/Thread.h
// fills from this character's own ItemResourceConfig. charRes carries the three
// int ASSET ids; the old code wrote those into the uint16 sprite fields (and
// never wrote the square one), so every card got a sprite index that does not
// resolve - the blank character skin.
inline void ApplyCharacterSkin(const charInfo &getchar,
                               int selTraitor1P, int selTraitor3P,
                               int selItemID, int selRoleID, int selPackID,
                               const std::vector<void *> &charModels,
                               const std::vector<void *> &itemResources,
                               const std::vector<void *> &roleConfs,
                               const std::vector<void *> &rolePacks)
{
    for (auto charModel : charModels) {
        if (!charModel) continue;
        auto *cf = (CharacterModelFields *)((uintptr_t)charModel + 0x10);
        if (cf->Traitor1P != selTraitor1P && cf->Traitor3P != selTraitor3P)
            continue;

        cf->BRBagModel         = getchar.charModel[0];
        cf->BRHeadModel        = getchar.charModel[1];
        cf->BRLobby            = getchar.charModel[2];
        cf->BRModel            = getchar.charModel[3];
        cf->BindEffect1P       = getchar.charModel[4];
        cf->ChangeClipEffect1P = getchar.charModel[5];
        cf->DefaultModelID     = getchar.charModel[6];
        cf->Guarder1P          = getchar.charModel[7];
        cf->Guarder3P          = getchar.charModel[8];
        cf->GuarderBagModel    = getchar.charModel[9];
        cf->GuarderHeadModel   = getchar.charModel[10];
        cf->GuarderLobby       = getchar.charModel[11];
    }

    for (auto itemRes : itemResources) {
        if (!itemRes) continue;
        auto *itf = (ItemResourceFields *)((uintptr_t)itemRes + 0x10);
        if (itf->ID != selItemID) continue;

        if (getchar.charRes[0] != 0) itf->FxAssetID        = getchar.charRes[0];
        if (getchar.charRes[1] != 0) itf->InventoryModelID = getchar.charRes[1];
        if (getchar.charRes[2] != 0) itf->ModelAssetIDRaw  = getchar.charRes[2];

        if (getchar.ItemRes2[0] != 0) itf->UIMiniSpriteNameIndex   = getchar.ItemRes2[0];
        if (getchar.ItemRes2[1] != 0) itf->UISmallSpriteNameIndex  = getchar.ItemRes2[1];
        if (getchar.ItemRes2[2] != 0) itf->UISpriteNameIndex       = getchar.ItemRes2[2];
        if (getchar.ItemRes2[3] != 0) itf->UISquareSpriteNameIndex = getchar.ItemRes2[3];
    }

    for (auto roles : roleConfs) {
        if (!roles) continue;
        auto *rf = (RoleConfFields *)((uintptr_t)roles + 0x10);
        if (rf->ID != selRoleID) continue;

        rf->roleLeftArmID       = getchar.charRole[0];
        rf->roleFinalSuitID     = getchar.charRole[1];
        rf->roleBasicHologramID = getchar.charRole[2];
        rf->ColorID             = getchar.charRole[3];
        rf->ColorSubID          = getchar.charRole[4];
        rf->ShowRare            = getchar.charRole[5];
        rf->RoleLvGroupID       = getchar.charRole[6];
        rf->RolePackID          = getchar.charRole[7];
        rf->LOCID_Name          = getchar.charRole2[0];
    }

    for (auto pack : rolePacks) {
        if (!pack) continue;
        auto *pf = (RolePackFields *)((uintptr_t)pack + 0x10);
        if (pf->RolePackID != selPackID ||
            pf->LobbySceneType != 0 ||
            pf->LoadingFrame == 0)
            continue;

        pf->EntryAnimID      = getchar.charPack[1];
        pf->GestureId        = getchar.charPack[2];
        pf->HandEffectUI     = getchar.charPack[3];
        pf->LoadingFrame     = getchar.charPack[4];
        pf->KillStreakSkinID = getchar.charPack[5];
    }
}

struct WeaponConfFields
{
    uint8_t Accuracy;                // 0x20
    bool CanEquipOptic;              // 0x21
    uint8_t ColorID;                 // 0x22
    uint8_t ColorSubID;              // 0x23
    uint8_t Control;                 // 0x24
    uint8_t Damage;                  // 0x25
    bool ExternalUnVisible;          // 0x26
    uint8_t FireRate;                // 0x21
    uint8_t LOCID_HowToGet;          // 0x28
    uint8_t MaxAttach;               // 0x29
    uint8_t MaxSkill;                // 0x2A
    uint8_t Mobility;                // 0x2B
    uint8_t Range;                   // 0x2C
    bool ShowRare;                   // 0x2D
    uint8_t SuperShowGroup;          // 0x1C
    uint8_t WeaponBluePrintType;     // 0x1C
    uint8_t WinnerCircleAnimID;      // 0x30
    char pad_31[3];                  // 0x31 alignment
    uint BaseWeaponId;               // 0x34
    int DefWeaponSkinID;             // 0x38
    int DefaultKillBrocast;          // 0x3C
    uint ID;                         // 0x40
    int IsDouble;                    // 0x44
    int WinnerCircleWeaponMountType; // 0x48
    int pad_4C;                      // 0x4C
    void *ChangeColorStr;            // 0x50
    void *EnableCamoBeta;            // 0x58
    void *LOCID_Desc;                // 0x60
    void *LOCID_Name;                // 0x68
    void *ModelRotation;             // 0x70
    void *UIProperty;                // 0x78
    void *UIPropertyUnlockLevel;     // 0x80
};
extern WeaponConfFields *weaponconfFields;

// ---------------------------------------------------------------------------
// Camo application - "replace the equipped camo on every gun".
//
// WeaponConfConfig.DefWeaponSkinID @0x38 (dump 823910, cast base +0x20 like the
// rest of the file) is the field the weapon-skin path and the original
// Gold/Diamond/... checkboxes both write, so it is the lever the game already
// honours for "which camo is on this weapon". Every row of the table is written,
// not only the weapons that happen to have a skin entry in itemData - that join
// is what made the old checkboxes miss guns.
//
//   camoItemID > 0  - remember each conf's current value ONCE, then write the
//                     camo id, so switching camo simply replaces it.
//   camoItemID <= 0 - the tab's "Default / OFF": put the remembered values back.
//                     Confs that were never touched are left completely alone,
//                     which is why OFF can no longer blank a weapon's own
//                     default skin the way writing a literal 0 did.
//
// `originals` is the caller's store (the tab keeps one for the session), so this
// stays a pure function over the conf list and the host test can drive it.
inline int ApplyCamoToEveryGun(int camoItemID,
                               const std::vector<void *> &weaponConfs,
                               std::unordered_map<uint, int> &originals)
{
    int written = 0;
    for (void *conf : weaponConfs) {
        if (!conf || !Tools::IsPtrValid(conf))
            continue;
        auto *cf = (WeaponConfFields *)((uintptr_t)conf + 0x20);
        if (!Tools::IsPtrValid(cf) || cf->ID == 0)
            continue;

        if (camoItemID > 0) {
            // emplace keeps the FIRST value seen: after a few camo switches the
            // stored original is still the game's own, not the last pick.
            originals.emplace(cf->ID, cf->DefWeaponSkinID);
            cf->DefWeaponSkinID = camoItemID;
            written++;
        } else {
            auto it = originals.find(cf->ID);
            if (it == originals.end())
                continue;
            cf->DefWeaponSkinID = it->second;
            written++;
        }
    }
    return written;
}

struct WeaponCamosFields
{
    uint8_t CamoType;
    uint8_t ColorID;
    bool NeedShowInEndGame;
    int BelongWeaponID;
    uint CamoItemID;
    uint ConceptID;
    int DMZseason;
    int KeepInCatalog;
    int TextureAssetID;
    int TextureSpecialAssetID;
    void *LOCID_Describe;
    void *LOCID_Name;
    Array<uint> *Mission_id;
    Array<uint> *Personal_mission_id;
    void *SmallIconSpriteName;
};
extern WeaponCamosFields *weaponcamosFields;

struct WeaponSkinFields
{
    uint8_t ColorID;
    uint8_t GoGetPos;
    bool IsCleanCamo;
    uint8_t SkinType;
    uint BRWeaponID;
    int BrSkinWeaponID;
    int CamoID;
    int ColorSubID;
    uint ID;
    uint WeaponID;
    void *LOCID_Describe;
    void *LOCID_HowToGet;
    void *LOCID_Name;
    Array<int> *SkinID1P;
    Array<int> *SkinID3P;
};
extern WeaponSkinFields *weaponskinFields;

// ---------------------------------------------------------------------------
// Camo application - part TWO: the guns that already have a camo equipped.
//
// WeaponConfConfig.DefWeaponSkinID (ApplyCamoToEveryGun above) is a weapon's own
// DEFAULT skin - the value the game falls back to for a gun with no camo chosen.
// A gun that DID have a camo therefore kept showing its own, which is exactly the
// report "it puts the camo on every gun WITHOUT camo, not the guns with camo".
//
// The record that wins there is the weapon-skin row (WeaponSkinConfig, captured by
// the WeaponSkin ctor hook): dump 825712 gives ColorID 0x10, GoGetPos 0x11,
// IsCleanCamo 0x12, SkinType 0x13, BRWeaponID 0x14, BrSkinWeaponID 0x18,
// CamoID 0x1C, ColorSubID 0x20, ID 0x24, WeaponID 0x28 - one row per weapon skin,
// each carrying the camo it renders (CamoID). Writing the picked camo there as
// well as into DefWeaponSkinID is what reaches a gun whose camo is already set.
//
//   camoItemID > 0  - remember (CamoID, ColorSubID) ONCE per row, then write the
//                     pick; switching camo replaces it instead of stacking.
//   camoItemID <= 0 - put the remembered values back. Rows that were never touched
//                     are left completely alone.
struct WeaponSkinCamoOriginal
{
    int camoID;      // WeaponSkinConfig.CamoID @0x1C
    int colorSubID;  // WeaponSkinConfig.ColorSubID @0x20
};

inline int ApplyCamoToEveryWeaponSkin(int camoItemID,
                                      const std::vector<void *> &skinConfs,
                                      std::unordered_map<uint, WeaponSkinCamoOriginal> &originals)
{
    int written = 0;
    for (void *skin : skinConfs) {
        if (!skin || !Tools::IsPtrValid(skin))
            continue;
        auto *sf = (WeaponSkinFields *)((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(sf) || sf->ID == 0)
            continue;

        if (camoItemID > 0) {
            // emplace keeps the FIRST value seen, so after several camo switches
            // the stored original is still the game's own camo, not the last pick.
            originals.emplace(sf->ID, WeaponSkinCamoOriginal{sf->CamoID, sf->ColorSubID});
            sf->CamoID = camoItemID;
            sf->ColorSubID = 0;
            written++;
        } else {
            auto it = originals.find(sf->ID);
            if (it == originals.end())
                continue;
            sf->CamoID = it->second.camoID;
            sf->ColorSubID = it->second.colorSubID;
            written++;
        }
    }
    return written;
}

struct Item2InventoryFields
{
    int ItemID;                // 0x20
    int WeaponAssetGroupID;    // 0x24
    int WeaponIconID;          // 0x28
    void *AssetName;           // 0x30
    void *ConfigFilePathH;     // 0x38
    void *ConfigFilePath_BRH;  // 0x40
    void *ConfigFilePath_DMZH; // 0x48
    void *ConfigFilePath_PVEH; // 0x50
    void *WeaponNameH;         // 0x58
};
extern Item2InventoryFields *item2Fields;

struct WeaponAssetGroupFields
{
    int FireEffectGroupID;              // 0x40
    int Id;                             // 0x48
    int MeshAssetID1P;                  // 0x48
    int MeshAssetID3P;                  // 0x4C
    int MeshAssetID3P_L;                // 0x50
    int MeshAssetIDUI;                  // 0x54
    int ProjAssetID;                    // 0x58
    int ProjExplosionAssetID;           // 0x5C
    int ProjFlightAssetID;              // 0x60
    int ProjFlightAssetID3P;            // 0x64
    int SecendBRProjExplosionAssetID;   // 0x68
    int SecendProjAssetID;              // 0x6C
    int SecendProjExplosionAssetID;     // 0x70
    int SecendProjFlightAssetID;        // 0x74
    int SecendProjFlightAssetID3P;      // 0x7C
    int SecondaryFireEffectGroupID;     // 0x80
    int SpecialUnAimingEffectAssetID1P; // 0x80
    int UnusePupil;                     // 0x84
    int WeaponPickUpAssetID;            // 0x88
    void *LeftLaserRotation;            // 0x90
    void *RightLaserRotation;           // 0xC8
    void *ScopeDistancePartIDs;         // 0xC0
    void *ScopeDistanceRatios;          // 0xC8
};
extern WeaponAssetGroupFields *weaponAssetFields;

struct WeaponFireEffectFields
{
    int AssetIdFor1P;                     // 0x10
    int AssetIdFor3P;                     // 0x14
    int AssetIdForBulletSmoke1P;          // 0x18
    int AssetIdForBulletSmoke3P;          // 0x1C
    int AssetIdForBulletTrack1P;          // 0x20
    int AssetIdForBulletTrack3P;          // 0x24
    int AssetIdForCharging1P;             // 0x28
    int AssetIdForCrossHair;              // 0x2C
    int AssetIdForDecalEffect;            // 0x30
    int AssetIdForExtra1P;                // 0x34
    int AssetIdForExtraBulletSmoke1P;     // 0x38
    int AssetIdForExtraBulletSmoke3P;     // 0x3C
    int AssetIdForHip1P;                  // 0x40
    int AssetIdForHip3P;                  // 0x48
    int AssetIdForHitEnemy;               // 0x48
    int AssetIdForInspectionScreenEffect; // 0x4C
    int AssetIdForSecondBulletSmoke1P;    // 0x50
    int AssetIdForSecondBulletSmoke3P;    // 0x54
    int AssetIdForSecondHip1P;            // 0x58
    int AssetIdForSecondHip3P;            // 0x5C
    int AssetIdForShellDrop1P;            // 0x60
    int AssetIdForShellDrop3P;            // 0x64
    int AttachAssetIdFor1P;               // 0x68
    int AttachAssetIdFor3P;               // 0x6C
    int AttachAssetIdForHip1P;            // 0x70
    int AttachAssetIdForHip3P;            // 0x74
    int DefaultKillEffectLevelUI;         // 0x7C
    float HitEffectDelayDestroyTime;      // 0x80
    int HitEffectType;                    // 0x80
    int Id;                               // 0x84
    float LevelEffectDelayTimeUI;         // 0x88
    float LevelEffectDelayTimeUI_King;    // 0x8C
    void *AssetIdForInspectionEffect;     // 0x90
    void *AssetIdForLevelEffects;         // 0xC8
    void *AssetIdForLevelEffectsUI;       // 0xC0
    void *AssetIdsForCircleBluntDecal;    // 0xC8
    void *AssetIdsForCrustDecal;          // 0xC0
    void *AssetIdsForExtraEffects;        // 0xC8
    void *AssetIdsForExtraEffects3P;      // 0xC0
    void *AssetIdsForExtraEffectsUI;      // 0xC8
    void *AssetIdsForMeleeAttack;         // 0xD0
    void *AssetIdsForMetalDecal;          // 0xD8
    void *AssetIdsForShellDrop1P;         // 0xE0
    void *AssetIdsForShellDrop3P;         // 0xE8
    void *AssetIdsForStripBluntDecal;     // 0xF0
    void *BRKillCountTimePlus;            // 0xF8
    void *BRKillCountTimePlus_King;       // 0x100
    void *FireSoundExtend;                // 0x108
    void *HitEffectAssetIds;              // 0x110
    void *KillCountForLevelEffect;        // 0x118
    void *KillCountForLevelEffect_King;   // 0x120
    void *MPKillCountTimePlus;            // 0x128
    void *MPKillCountTimePlus_King;       // 0x130
    void *MeleeEffectDetachParent;        // 0x138
    void *SocketForInspectionEffect;      // 0x140
    void *SocketForLevelEffects;          // 0x148
    void *SocketForLevelEffectsUI;        // 0x150
    void *SocketNameForExtra1P;           // 0x158
    void *SocketsForExtraEffects;         // 0x160
    void *SocketsForExtraEffects3P;       // 0x168
    void *SocketsForExtraEffectsUI;       // 0x170
    void *SocketsForMeleeAttack;          // 0x178
    void *SocketsForShellDrop1P;          // 0x180
    void *SocketsForShellDrop3P;          // 0x188
};
extern WeaponFireEffectFields *weaponfireFields;

struct WeaponConfExtraFields
{
    uint8_t DefaultMythicArmor;     // 0x10
    uint8_t DefaultMythicSig;       // 0x11
    uint8_t GoGetPos;               // 0x12
    uint8_t SecondTab;              // 0x13
    uint8_t ThirdTab;               // 0x14
    int DefaultDeadReplayEffectId;  // 0x18
    int DefaultKillEffectId;        // 0x1C
    int FireEffectGroupID;          // 0x20
    uint ID;                        // 0x24
    void *GoGetUrl;                 // 0x28
    void *IdleAnimationIndex;       // 0x30
    void *OpenViewSkill;            // 0x38
    void *OpenViewSkillUnlockLevel; // 0x40
    void *PropertyModifys;          // 0x48
};
extern WeaponConfExtraFields *weaponextraFields;

struct MythicArmorFields
{
    int AssetID;              // 0x14
    int ColorID;              // 0x18
    int ColorSubID;           // 0x1C
    int GoGetPos;             // 0x20
    uint ID;                  // 0x24
    int MythicArmorLevel;     // 0x28
    int SecondTab;            // 0x2C
    int ThirdTab;             // 0x30
    void *EquippedWeaponIcon; // 0x38
    void *GoGetUrl;           // 0x40
    void *LOCID_Desc;         // 0x48
    void *LOCID_HowToGet;     // 0x50
    void *LOCID_Name;         // 0x58
    void *WeaponCanEquip;     // 0x60
};
extern MythicArmorFields *mythicarmorFields;

struct MythicSightFields
{
    bool ShowInBag;        // 0x10
    char pad_11[3];        // 0x11
    int AssetID;           // 0x14
    int ColorID;           // 0x18
    int ColorSubID;        // 0x1C
    uint ID;               // 0x20
    int RelateMythicSight; // 0x24
    void *LOCID_Desc;      // 0x28
    void *LOCID_Name;      // 0x30
    void *WeaponCanEquip;  // 0x38
};
extern MythicSightFields *mythicsightFields;

struct KillEffectItemFields
{
    void *RealAssetIDs;    // 0x10
    bool IsAutoDemolition; // 0x18
    bool ShowInBag;        // 0x19
    char pad_1A[2];        // 0x1A
    int ColorID;           // 0x1C
    int ColorSubID;        // 0x20
    uint CurrencyID;       // 0x24
    int GoGetPos;          // 0x28
    uint ID;               // 0x2C
    int SecondTab;         // 0x30
    int SellPrice;         // 0x34
    int SellProtection;    // 0x38
    int ThirdTab;          // 0x3C
    void *AssetID;         // 0x40
    void *AssetID_CN;      // 0x48
    void *AssetID_GA;      // 0x50
    void *AssetID_KR;      // 0x58
    void *AssetID_VNG;     // 0x60
    void *GoGetUrl;        // 0x68
    void *KillCount;       // 0x70
    void *LOCID_Desc;      // 0x7C
    void *LOCID_HowToGet;  // 0x80
    void *LOCID_Name;      // 0x88
    void *WeaponCanEquip;  // 0x90
};
extern KillEffectItemFields *killeffectFields;

struct BRDeadboxSkinFields
{
    int ColorID;            // 0x10
    int DeadBoxEffectAsset; // 0x14
    int Flag;               // 0x18
    int FlagAsset;          // 0x1C
    uint ID;                // 0x20
    int ModelAsset3P;       // 0x24
    int ModelAssetUI;       // 0x28
};
extern BRDeadboxSkinFields *deadboxFields;

struct BRDropPlaneSkinFields
{
    int ColorID;            // 0x10
    uint ID;                // 0x14
    int ModelAsset1P;       // 0x18
    int ModelAsset3P;       // 0x1C
    int ModelAssetCutScene; // 0x20
    int ModelAssetUI;       // 0x24
    int Priority;           // 0x28
};
extern BRDropPlaneSkinFields *dropplaneFields;

// VehicleSkinConfConfig (dump 822946). ColorID is the first field @0x10, so the
// offsets below are relative to a +0x10 loader base: the old +0x18 base made ID
// read NewVehicleResId and VehicleId read UILowModel, which is why the Snowboard
// tab could never match a skin (LoadSnowboardSkins filters on VehicleId, and
// FindVehicleSkinConfigById on ID). The dump's byte flags 0x11..0x19 are copied
// one for one - the old decl was missing DisableInCollection, which shifted
// every flag after ColorID by one slot.
struct VehicleSkinConfFields
{
    uint8_t ColorID;         // 0x10 @0x00
    bool DisableInCollection; // 0x11 @0x01
    bool ExternalUnVisible;  // 0x12 @0x02
    uint8_t GoGetPos;        // 0x13 @0x03
    bool IsAutoDemolition;   // 0x14 @0x04
    uint8_t LockedShowOrder; // 0x15 @0x05
    bool ShowInBag;          // 0x16 @0x06
    bool ShowRare;           // 0x17 @0x07
    uint8_t SkinMusicsID;    // 0x18 @0x08
    uint8_t SkinStyle;       // 0x19 @0x09
    char pad_1A[2];          // 0x1A (@0x0A) - aligns CurrencyID to 4
    uint CurrencyID;         // 0x1C @0x0C
    uint ID;                 // 0x20 @0x10 (skin join key)
    int MusicsID;            // 0x24 @0x14
    int NewVehicleResId;     // 0x28 @0x18 (mesh the apply path forces)
    float RoomShowScale;     // 0x2C @0x1C
    int SellPrice;           // 0x30 @0x20
    int SkinExtraMesh;       // 0x34 @0x24
    int SkinParam;           // 0x38 @0x28
    int SkinParam2;          // 0x3C @0x2C
    int SkinParam3;          // 0x40 @0x30
    int UILowModel;          // 0x44 @0x34
    int VehicleId;           // 0x48 @0x38 (31707110 == snowboard)
    char pad_3C[12];         // 0x4C (@0x3C) - AudioDownloadKey is static
    void *FeatureDescLocid1; // 0x58 @0x48
    void *FeatureDescLocid2; // 0x60 @0x50
    void *GameType;          // 0x68 @0x58 (byte[])
    void *GoGetUrl;          // 0x70 @0x60
    char pad_68[24];         // 0x78..0x8F (@0x68)
    void *PreloadRes;        // 0x90 @0x80
    char pad_88[48];         // 0x98..0xC7 (@0x88)
    void *SkinParams;        // 0xC8 @0xB8 (int[]) - what VehicleSkinParams reads
    void *WeaponSkin;        // 0xD0 @0xC0
};
extern VehicleSkinConfFields *vehicleSkinFields;