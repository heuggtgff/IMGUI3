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

extern char searchQuery[256];
extern std::string ToLower(std::string str);
extern uintptr_t getRealOffset(uintptr_t offset);

struct RoleSkinFields
{
    uint8_t ClothType;             // 0x18
    uint8_t ColorID;               // 0x19
    uint8_t GoGetPos;              // 0x1A
    uint8_t HatSubMode;            // 0x1B
    uint8_t IsCanBeUsedForAllRole; // 0x1C
    bool IsCloseScreenSaver;       // 0x1D
    uint8_t SecondTab;             // 0x1E
    bool ShowInBag;                // 0x1F
    bool ShowRare;                 // 0x20
    int BRBagAssetID;              // 0x24
    int BRClothAssetID;            // 0x28
    int BRHatAssetID;              // 0x2C
    int BRLeftArmAssetID;          // 0x30
    int BRSuitAssetID;             // 0x34
    int BRWingAssetID;             // 0x38
    int BagAssetID;                // 0x3C
    int CamoID;                    // 0x40
    int ClothAssetID;              // 0x48
    int FxAssetIDHandGestureUI;    // 0x48
    int FxAssetIDShow;             // 0x4C
    int FxAssetID_1P;              // 0x50
    int FxAssetID_3P;              // 0x54
    int FxAssetID_UI;              // 0x58
    int HatAssetID;                // 0x5C
    uint ID;                       // 0x60
    int LeftArm1PAssetID;          // 0x64
    int LeftArmAssetID;            // 0x68
    int ShowBagAssetID;            // 0x6C
    int ShowClothAssetID;          // 0x70
    int ShowHatAssetID;            // 0x74
    int SuitAssetID;               // 0x80
    int WingAssetID;               // 0x80
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

struct RolePackFields
{
    int mMatchRoomEasterEggType;           // 0x10
    uint mMatchRoomEasterEggCharacterID;   // 0x14
    String *mMatchRoomEasterEggActionName; // 0x18
    bool EntryDefaultAppearance;           // 0x20
    bool UseVFWFeature;                    // 0x21
    int AirItem1;                          // 0x24
    int AnimationWhenSell;                 // 0x28
    int CabinIndivID;                      // 0x2C
    int DeadBoxID;                         // 0x30
    int DynamicRoomAssetID;                // 0x34
    int EntryAnimID;                       // 0x38
    int EntryUIEffectAssetID;              // 0x3C
    int GestureId;                         // 0x40
    int HandEffectUI;                      // 0x48
    int KillStreakSkinID;                  // 0x48
    int LoadingFrame;                      // 0x4C
    int LobbySceneType;                    // 0x50
    int MVPIndivID;                        // 0x54
    int MatchRoomSort;                     // 0x58
    int OpenSceneIndivID;                  // 0x5C
    int PendantShowID;                     // 0x60
    int PetID;                             // 0x64
    int PetSwitchEffectAssetID;            // 0x68
    uint RolePackID;                       // 0x6C
    int SkillSkinID;                       // 0x70
    int SkillSkinUI;                       // 0x74
    int SpecialSceneFrameGroupAssetID;     // 0x7C
    int VTOLShowAssetID;                   // 0x80
    String *AirPoseBR;                     // 0x80
    String *EntryAnim;                     // 0x88
    String *EntryIdleAnim;                 // 0x90
    void *EntryIdleItems;                  // 0xC8
    void *EntryItems;                      // 0xC0
    void *EntryItemsAnim;                  // 0xC8
    void *EntryItemsIdleAnim;              // 0xC0
    String *EntryUIAnimationStateName;     // 0xC8
};
extern RolePackFields *packfields;

struct RoleConfFields
{
    uint BaseCharacterID;           // 0x10
    uint roleLeftArmID;             // 0x14
    uint roleFinalSuitID;           // 0x18
    uint roleBasicHologramID;       // 0x1C
    uint8_t ArchiveSwitch;          // 0x20
    uint8_t ArmorType;              // 0x21
    bool BlockBack;                 // 0x22
    uint8_t ColorID;                // 0x23
    uint8_t ColorSubID;             // 0x24
    uint8_t DefaultPerk_1;          // 0x25
    uint8_t DefaultPerk_2;          // 0x26
    uint8_t DefaultPerk_3;          // 0x21
    bool DefaultShow;               // 0x28
    uint8_t DefaultSkill_1;         // 0x29
    bool ExternalUnVisible;         // 0x2A
    bool IsVFW;                     // 0x2B
    uint8_t Loadout_Magnifier_Flag; // 0x2C
    uint8_t RoleLvGroupID;          // 0x2D
    bool ShowInBag;                 // 0x1C
    uint8_t ShowOrder;              // 0x1C
    bool ShowRare;                  // 0x30
    uint ID;                        // 0x34
    int JetPackInfoID;              // 0x38
    int KeepInCatalog;              // 0x3C
    float RTPCMaxValue;             // 0x40
    int RolePackID;                 // 0x48
    void *AudioSwitch;              // 0x48
    void *BRBankNameEX;             // 0x50
    void *BasicAudioBank;           // 0x58
    void *BasicAudioBank2;          // 0x60
    void *BasicAudioBank3;          // 0x68
    void *DefaultAudioBank;         // 0x70
    void *DefaultAudioBank2;        // 0x7C
    void *DefaultAudioEventEx;      // 0x80
    void *FoleyAudioBank;           // 0x88
    void *FoleyAudioNameEx;         // 0x90
    void *LOCID_Desc;               // 0xC8
    void *LOCID_Name;               // 0xC0
    void *LitePackageKeyName;       // 0xC8
    void *MPBankNameEx;             // 0xC0
    void *PVEBankNameEX;            // 0xC8
    void *RTPCName;                 // 0xC0
    void *RoleSkinBox;              // 0xC8
    void *SpecialAudioBank;         // 0xD0
    void *SpecialAudioBank2;        // 0xD8
    void *SpecialAudioEventEx;      // 0xE0
    void *SpecialTips;              // 0xE8
    uint64_t WinnerCircle;          // 0xF0
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
    uint BaseWeaponId;               // 0x34
    int DefWeaponSkinID;             // 0x38
    int DefaultKillBrocast;          // 0x3C
    uint ID;                         // 0x40
    int WinnerCircleWeaponMountType; // 0x48
    void *EnableCamoBeta;            // 0x48
    void *LOCID_Desc;                // 0x50
    void *LOCID_Name;                // 0x58
    void *ModelRotation;             // 0x60
    void *UIProperty;                // 0x68
    void *UIPropertyUnlockLevel;     // 0x70
};
extern WeaponConfFields *weaponconfFields;

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