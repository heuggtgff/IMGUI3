std::unordered_map<std::string, bool> sBool;
std::unordered_map<int, int> activeKillEffects;
std::unordered_map<int, int> activeKillIcons;
// Same idea as activeKillIcons, but keyed to the KillEffectItemConfConfig ID that
// the client-side WeaponAttrData records (`m_KillEffectItem`): the in-match kill
// effect is resolved from the item ID, so both have to be surfaced.
std::unordered_map<int, int> activeKillEffectItems;
// Filled by LoadWeaponSkins: weapon id -> kill effect asset id, and
// kill effect asset id -> KillEffectItemConfConfig item id.
std::unordered_map<int, int> killEffectByWeapon;
std::unordered_map<int, int> killEffectItemByAssetId;
std::unordered_map<int, int> activeBulletTrackEffects;
std::unordered_map<int, int> activeWeaponFireEffects;
std::unordered_map<int, int> activeWeaponBrocast;
std::vector<TargetChar> g_targetCharacters;
int g_selectedTargetCharIndex = 0;

std::vector<void *> itemInventoryInstance;
std::vector<void *> weaponExtraInstance;
std::vector<void *> weaponFireEffectInstance;
std::vector<void *> weaponConfInstance;
std::vector<void *> weaponAssetGroupInstance;
std::vector<void *> mythicArmorInstance;
std::vector<void *> mythicSightInstance;
std::vector<void *> killEffectItemInstance;
std::vector<void *> killIconConfigInstance;
std::vector<void *> weaponSkinConfigInstance;
std::vector<void *> weaponCamosConfigInstance;
std::vector<void *> itemResourceConfigInstance;
std::vector<void *> CharacterModelConfigInstance;
std::vector<void *> RoleConfConfigInstance;
std::vector<void *> RoleSkinConfigInstance;
std::vector<void *> RolePackConfConfigInstance;
std::vector<void *> BRDeadboxSkinConfigInstance;
std::vector<void *> BRDropPlaneSkinConfigInstance;
std::vector<void *> vehicleSkinConfigInstance;
std::vector<void *> vehicleItemConfigInstance;
std::unordered_map<int, int> activeVehicleSkins;
std::unordered_map<int, int> activeVehicleSkinsById;
std::unordered_map<int, void*> activeVehicleSkinConfs;

std::vector<itemInfo> itemData;
std::vector<charInfo> charData;
std::vector<watcher> watch;
std::vector<deadbox> deadboxF;
std::vector<planeID> dropplane;
std::vector<snowboardInfo> snowboardData;
std::vector<camoInfo> camoData;

RoleSkinFields *roleskinFields;
CharacterModelFields *characterfields;
RolePackFields *packfields;
RoleConfFields *roleFields;
ItemResourceFields *itemFields;
WeaponConfFields *weaponconfFields;
WeaponCamosFields *weaponcamosFields;
WeaponSkinFields *weaponskinFields;
Item2InventoryFields *item2Fields;
WeaponAssetGroupFields *weaponAssetFields;
WeaponFireEffectFields *weaponfireFields;
WeaponConfExtraFields *weaponextraFields;
MythicArmorFields *mythicarmorFields;
MythicSightFields *mythicsightFields;
KillEffectItemFields *killeffectFields;

// SPKillIconItemConfConfig (dump.cs 819793): fields are dump-resolved here
// (first field @0x10), ColorID distinguishes the mythic kill icons.
struct SPKillIconItemFields
{
    bool IsAutoDemolition;             // 0x10
    bool ShowInBag;                    // 0x11
    char pad_12[2];                    // 0x12
    int AssetID;                       // 0x14
    int ColorID;                       // 0x18
    int ColorSubID;                    // 0x1C
    uint CurrencyID;                   // 0x20
    int EffectAssetID;                 // 0x24
    int GoGetPos;                      // 0x28
    uint ID;                           // 0x2C
    int SecondTab;                     // 0x30
    int SellPrice;                     // 0x34
    int SellProtection;                // 0x38
    int ThirdTab;                      // 0x3C
    void *BoardUIName;                 // 0x40
    void *GoGetUrl;                    // 0x48
    void *LOCID_Desc;                  // 0x50
    void *LOCID_HowToGet;              // 0x58
    void *LOCID_Name;                  // 0x60
    void *TieUIName;                   // 0x68
    void *WeaponCanEquip;              // 0x70 (int[])
};
SPKillIconItemFields *killiconFields;
BRDeadboxSkinFields *deadboxFields;
BRDropPlaneSkinFields *dropplaneFields;

VehicleSkinConfFields* vehicleSkinFields;   // struct lives in Skin/Fields.h (offset-tested)

uintptr_t Item2InventoryAddress = 0x6971D34;
uintptr_t WeaponConfAddress = 0xAF7E7FC;
uintptr_t WeaponConfExtraAddress = 0xAF80080;
uintptr_t WeaponFireEffectAddress = 0xAF81990;
uintptr_t CharacterModelAddress = 0xB1A9738;
uintptr_t BRDeadboxSkinAddress = 0x6FF3160;
uintptr_t BRDropPlaneSkinAddress = 0xB1981D4;
uintptr_t WeaponAssetGroupAddress = 0xAF7A080;
uintptr_t MythicArmorConfigAddress = 0x5C90E84;
uintptr_t MythicSightConfigAddress = 0x90DDEEC;
uintptr_t KillEffectItemConfConfigAddress = 0x90C54CC;
uintptr_t SPKillIconItemConfConfigAddress = 0xAF67F1C;   // SPKillIconItemConfConfig..ctor (dump 819836)
uintptr_t GetWeaponKillIconByExtraAddress = 0xC366318;   // GetWeaponKillIcon(SyncWeaponExtraInfo) (dump 1089440)
uintptr_t GetWeaponKillIconOwnAddress     = 0xC37F220;   // GetWeaponKillIcon(out object decorations) (dump 1089443)
uintptr_t WeaponSkinAddress = 0xAF8B4D4;
uintptr_t WeaponCamosAddress = 0xAF7E148;
uintptr_t ItemResourceAddress = 0x6985AB8;
uintptr_t RoleConfAddress = 0x90F20BC;
uintptr_t RoleSkinAddress = 0x90F95C8;
uintptr_t RolePackConfAddress = 0x90F784C;

uintptr_t WeaponConfName = 0xAF7FDC8;
uintptr_t GetDropPlaneName = 0xB198B08;
uintptr_t GetVehicleSkinName = 0xAF74B38;   // VehicleSkinConfConfig.get_Name (dump 823011)
uintptr_t GetNameRoleSkin = 0x90F98F4;
uintptr_t GetRoleConfName = 0x90F4214;
uintptr_t GetDeadBoxIDAddress = 0x90F2CA0;
uintptr_t UnlockedCamo = 0xAF7E580;

uintptr_t VehicleSkinConfAddress = 0xAF74090;
uintptr_t VehicleItemConfAddress = 0xAF73A18;
uintptr_t VehicleSkinHelperChangeVehicleSkinIdAddress = 0x9B84878;
uintptr_t VehicleSkinHelperChangeVehicleSkinConfAddress = 0x9B849EC;
uintptr_t VehicleSkisSetupSkinAddress = 0x9A9F668;
uintptr_t VehicleSkisChangeSkinIdAddress = 0x9A9F7DC;
uintptr_t VehicleSkisChangeSkinConfAddress = 0x9A9FAE0;
uintptr_t VehicleSkisBuildVehicleModelAddress = 0x9AA24A0;
uintptr_t VehicleSkisLoadModelCompleteAddress = 0x9AA26EC;
uintptr_t VehicleSkisApplySkinAddress = 0x9AA2AFC;

typedef void (*Item2InventoryCtor)(void*);
typedef void (*WeaponConfExtraCtor)(void*);
typedef void (*WeaponFireEffectCtor)(void*);
typedef void (*WeaponConfCtor)(void*);
typedef void (*WeaponAssetGroupCtor)(void*);
typedef void (*MythicArmorCtor)(void*);
typedef void (*MythicSightCtor)(void*);
typedef void (*WeaponSkinCtor)(void*);
typedef void (*WeaponCamosCtor)(void*);
typedef void (*KillEffectItemCtor)(void*);
typedef void (*SPKillIconCtor)(void*);
typedef void (*ItemResourceCtor)(void*);
typedef void (*CharacterModelCtor)(void*);
typedef void (*RoleConfCtor)(void*);
typedef void (*RoleSkinCtor)(void*);
typedef void (*RolePackConfCtor)(void*);
typedef void (*BRDeadboxSkinCtor)(void*);
typedef void (*BRDropPlaneSkinCtor)(void*);
typedef void (*VehicleSkinConfCtor)(void*);
typedef void (*VehicleItemConfCtor)(void*);

Item2InventoryCtor orig_Item2InventoryCtor = nullptr;
WeaponConfExtraCtor orig_WeaponConfExtraCtor = nullptr;
WeaponFireEffectCtor orig_WeaponFireEffectCtor = nullptr;
WeaponConfCtor orig_WeaponConfCtor = nullptr;
WeaponAssetGroupCtor orig_WeaponAssetGroupCtor = nullptr;
MythicArmorCtor orig_MythicArmorCtor = nullptr;
MythicSightCtor orig_MythicSightCtor = nullptr;
WeaponSkinCtor orig_WeaponSkinCtor = nullptr;
WeaponCamosCtor orig_WeaponCamosCtor = nullptr;
KillEffectItemCtor orig_KillEffectItemCtor = nullptr;
SPKillIconCtor orig_SPKillIconCtor = nullptr;
ItemResourceCtor orig_ItemResourceCtor = nullptr;
CharacterModelCtor orig_CharacterModelCtor = nullptr;
RoleConfCtor orig_RoleConfCtor = nullptr;
RoleSkinCtor orig_RoleSkinCtor = nullptr;
RolePackConfCtor orig_RolePackConfCtor = nullptr;
BRDeadboxSkinCtor orig_BRDeadboxSkinCtor = nullptr;
BRDropPlaneSkinCtor orig_BRDropPlaneSkinCtor = nullptr;
VehicleSkinConfCtor orig_VehicleSkinConfCtor = nullptr;
VehicleItemConfCtor orig_VehicleItemConfCtor = nullptr;

std::mutex g_dataMutex;

// Visible in the skin menu: which stage of the kill-effect / kill-icon data
// pipeline actually produced values. A zero here names the broken stage without
// another rebuild.
struct KillDecorationDiag {
    int killIconConfigs = 0;    // SPKillIconItemConfConfig instances captured
    int killIconWeapons = 0;    // weapon -> kill icon entries built
    int killEffectWeapons = 0;  // weapon -> kill effect asset entries built
    int killEffectItemIDs = 0;  // asset -> KillEffectItem item id entries built
    int iconHookCalls = 0;      // WeaponAttrData.get_KillIconItemID calls
    int iconHookHits = 0;       // ... that returned our kill icon
    int effectItemHookCalls = 0;
    int effectItemHookHits = 0;
    int effectGroupHookCalls = 0;
    int effectGroupHookHits = 0;
    int lastAttrKey = 0;        // last m_BlueprintWeaponId asked about
    // InventoryInfo writes (the copy the HUD actually reads): how many times the
    // local weapon's record had its kill icon / level filled in.
    int inventoryKillIconWrites = 0;
    int inventoryLevelWrites = 0;
};
KillDecorationDiag g_killDecorationDiag;

bool g_killIconHookInstalled = false;
bool g_killEffectItemHookInstalled = false;
bool g_killEffectGroupHookInstalled = false;

void my_Item2InventoryCtor(void* instance) {
    orig_Item2InventoryCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        itemInventoryInstance.push_back(instance);
    }
}

void my_WeaponConfExtraCtor(void* instance) {
    orig_WeaponConfExtraCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponExtraInstance.push_back(instance);
    }
}

void my_WeaponFireEffectCtor(void* instance) {
    orig_WeaponFireEffectCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponFireEffectInstance.push_back(instance);
    }
}

void my_WeaponConfCtor(void* instance) {
    orig_WeaponConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponConfInstance.push_back(instance);
    }
}

void my_WeaponAssetGroupCtor(void* instance) {
    orig_WeaponAssetGroupCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponAssetGroupInstance.push_back(instance);
    }
}

void my_MythicArmorCtor(void* instance) {
    orig_MythicArmorCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        mythicArmorInstance.push_back(instance);
    }
}

void my_MythicSightCtor(void* instance) {
    orig_MythicSightCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        mythicSightInstance.push_back(instance);
    }
}

void my_WeaponSkinCtor(void* instance) {
    orig_WeaponSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponSkinConfigInstance.push_back(instance);
    }
}

void my_WeaponCamosCtor(void* instance) {
    orig_WeaponCamosCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponCamosConfigInstance.push_back(instance);
    }
}

void my_KillEffectItemCtor(void* instance) {
    orig_KillEffectItemCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        killEffectItemInstance.push_back(instance);
    }
}

void my_SPKillIconCtor(void* instance) {
    orig_SPKillIconCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        killIconConfigInstance.push_back(instance);
    }
}

void my_ItemResourceCtor(void* instance) {
    orig_ItemResourceCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        itemResourceConfigInstance.push_back(instance);
    }
}

void my_CharacterModelCtor(void* instance) {
    orig_CharacterModelCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        CharacterModelConfigInstance.push_back(instance);
    }
}

void my_RoleConfCtor(void* instance) {
    orig_RoleConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RoleConfConfigInstance.push_back(instance);
    }
}

void my_RoleSkinCtor(void* instance) {
    orig_RoleSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RoleSkinConfigInstance.push_back(instance);
    }
}

void my_RolePackConfCtor(void* instance) {
    orig_RolePackConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RolePackConfConfigInstance.push_back(instance);
    }
}

void my_BRDeadboxSkinCtor(void* instance) {
    orig_BRDeadboxSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        BRDeadboxSkinConfigInstance.push_back(instance);
    }
}

void my_BRDropPlaneSkinCtor(void* instance) {
    orig_BRDropPlaneSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        BRDropPlaneSkinConfigInstance.push_back(instance);
    }
}

void my_VehicleSkinConfCtor(void* instance) {
    orig_VehicleSkinConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        vehicleSkinConfigInstance.push_back(instance);
    }
}

void my_VehicleItemConfCtor(void* instance) {
    orig_VehicleItemConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        vehicleItemConfigInstance.push_back(instance);
    }
}

char searchQuery[256] = "";

bool loadskinhack = false;
bool loadCharacter = false;
bool loadWatch = false;
bool loadDeadbox = false;
bool isLoad = false;

std::string lastKnownName = "";
int emptyNameCount = 0;

std::unordered_map<std::string, int> nameCountMap;
std::unordered_map<std::string, int> nameCountChar;
std::unordered_map<std::string, bool> getplane;
std::unordered_map<std::string, bool> getguns;

uintptr_t location = 0;

uintptr_t getRealOffset(uintptr_t offset)
{
    while (location <= 0)
    {
        location = Tools::GetBaseAddress("libunity.so");
        if (location <= 0)
        {
            usleep(1000);
        }
    }
    return location + offset;
}

std::string GetNameString(uintptr_t off, void *getadd)
{
    auto getC = (String * (*)(void *))(getRealOffset(off));
    if (getadd && Tools::IsPtrValid((void *)getC))
    {
        auto getV = getC(getadd);
        if (getV && Tools::IsPtrValid(getV))
        {
            return getV->CString();
        }
    }
    return "";
}

int (*orig_GetCurrentWeaponKillEffect)(Weapon);
int _GetCurrentWeaponKillEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeKillEffects.find(currentID);
    if (it != activeKillEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentBulletTrackEffect)(Weapon);
int _GetCurrentBulletTrackEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeBulletTrackEffects.find(currentID);
    if (it != activeBulletTrackEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentWeaponFireEffect)(Weapon);
int _GetCurrentWeaponFireEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeWeaponFireEffects.find(currentID);
    if (it != activeWeaponFireEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentWeaponBrocast)(Weapon);
int _GetCurrentWeaponBrocast(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeWeaponBrocast.find(currentID);
    if (it != activeWeaponBrocast.end()) {
        return it->second;
    }
    return 0;
}

// ── InventoryUtil (GameEngine) kill decorations overrides ───────────────
// The in-match kill effect / kill icon consumers are InventoryUtil statics
// (dump 1089418-1089460): GetWeaponKillEffectGroup / GetWeaponKillIcon /
// GetWeaponKillEffectLevel, each with a (SyncWeaponExtraInfo) overload and
// an (out object decorations) overload. WeaponPromotionInfo above only
// feeds the lobby upgrade UI, which is why kill effects never fired in
// match. Killing bullets never send our four WeaponPromotionInfo hooks
// so the maps have to surface through here.

// Resolve a map key from a SyncWeaponExtraInfo object.
// Layout (dump 1430872): VirtualItemID 0x20, ItemID 0x28, BluePrintID 0x30.
static bool FindInMapExtra(const std::unordered_map<int, int> &m, void *extraInfo, int &outVal) {
    if (!extraInfo || !Tools::IsPtrValid(extraInfo)) return false;
    uintptr_t p = (uintptr_t)extraInfo;
    int itemID = (int)*(uint *)(p + 0x28);
    if (itemID != 0) {
        auto it = m.find(itemID);
        if (it != m.end()) { outVal = it->second; return true; }
    }
    int virtID = (int)*(uint *)(p + 0x20);
    if (virtID != 0) {
        auto it = m.find(virtID);
        if (it != m.end()) { outVal = it->second; return true; }
    }
    int bpID = (int)*(uint *)(p + 0x30);
    if (bpID != 0) {
        auto it = m.find(bpID);
        if (it != m.end()) { outVal = it->second; return true; }
    }
    return false;
}

// Resolve a map key from the local pawn's current weapon.
static bool FindInMapLocal(const std::unordered_map<int, int> &m, int &outVal) {
    Pawn *localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return false;
    Weapon *currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return false;
    auto it = m.find(currentWeapon->get_WeaponID());
    if (it != m.end()) { outVal = it->second; return true; }
    return false;
}

uint (*orig_GetWeaponKillEffectGroupExtra)(void *extraInfo) = nullptr;
uint my_GetWeaponKillEffectGroupExtra(void *extraInfo) {
    int val;
    if (FindInMapExtra(activeKillEffects, extraInfo, val) && val > 0)
        return (uint)val;
    return orig_GetWeaponKillEffectGroupExtra ? orig_GetWeaponKillEffectGroupExtra(extraInfo) : 0;
}

uint (*orig_GetWeaponKillEffectGroupDecor)(void *decorations) = nullptr;
uint my_GetWeaponKillEffectGroupDecor(void *decorations) {
    int val;
    if (FindInMapLocal(activeKillEffects, val) && val > 0)
        return (uint)val;
    return orig_GetWeaponKillEffectGroupDecor ? orig_GetWeaponKillEffectGroupDecor(decorations) : 0;
}

uint (*orig_GetWeaponKillIconExtra)(void *extraInfo) = nullptr;
uint my_GetWeaponKillIconExtra(void *extraInfo) {
    int val;
    if (FindInMapExtra(activeKillIcons, extraInfo, val) && val > 0)
        return (uint)val;
    return orig_GetWeaponKillIconExtra ? orig_GetWeaponKillIconExtra(extraInfo) : 0;
}

uint (*orig_GetWeaponKillIconDecor)(void *decorations) = nullptr;
uint my_GetWeaponKillIconDecor(void *decorations) {
    int val;
    if (FindInMapLocal(activeKillIcons, val) && val > 0)
        return (uint)val;
    return orig_GetWeaponKillIconDecor ? orig_GetWeaponKillIconDecor(decorations) : 0;
}

// Mythic-L5 behavior: when the weapon carries an applied mythic skin
// (activeKillIcons), report kill-effect level 5 so the game picks the
// level-5 mythic tier everywhere (kill icons, kill feed, killcam).
uint (*orig_GetWeaponKillEffectLevelExtra)(void *extraInfo) = nullptr;
uint8_t my_GetWeaponKillEffectLevelExtra(void *extraInfo) {
    int val;
    if (FindInMapExtra(activeKillIcons, extraInfo, val) && val > 0)
        return 5;
    if (FindInMapExtra(activeKillEffects, extraInfo, val) && val > 0)
        return 5;
    return orig_GetWeaponKillEffectLevelExtra ? orig_GetWeaponKillEffectLevelExtra(extraInfo) : 0;
}

uint (*orig_GetWeaponKillEffectLevelDecor)(void *decorations) = nullptr;
uint8_t my_GetWeaponKillEffectLevelDecor(void *decorations) {
    int val;
    if (FindInMapLocal(activeKillIcons, val) && val > 0)
        return 5;
    if (FindInMapLocal(activeKillEffects, val) && val > 0)
        return 5;
    return orig_GetWeaponKillEffectLevelDecor ? orig_GetWeaponKillEffectLevelDecor(decorations) : 0;
}

// ── SPKillIcon lookup ─────────────────────────────────────────────────────
// weapon/OBB item ID → SPKillIconItemConfConfig ID (mythic/highest tier wins).
// Built from SPKillIconItemConfConfig instances hooked via my_SPKillIconCtor.
static std::unordered_map<int, int> killIconByWeapon;
static bool loadKillIcons = false;

void LoadKillIcons() {
    if (loadKillIcons) return;

    std::vector<void*> killIcons;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        killIcons = killIconConfigInstance;
    }

    if (killIcons.empty()) return;

    // Absolute config offsets per dump (SPKillIconItemConfConfig, dump 819794):
    // ColorID 0x18, ID 0x2C, ThirdTab 0x3C, WeaponCanEquip 0x70.
    for (void *icon : killIcons) {
        if (!icon || !Tools::IsPtrValid(icon)) continue;
        int id = *(int *)((uintptr_t)icon + 0x2C);
        if (id <= 0) continue;
        auto *equipArray = *(Array<int> **)((uintptr_t)icon + 0x70);
        if (!equipArray || !Tools::IsPtrValid(equipArray)) continue;
        int len = equipArray->getLength();
        for (int i = 0; i < len; i++) {
            int weaponID = equipArray->m_Items[i];
            if (weaponID <= 0) continue;
            auto it = killIconByWeapon.find(weaponID);
            if (it != killIconByWeapon.end()) {
                void *old = nullptr;
                for (void *c : killIcons) {
                    if (*(int *)((uintptr_t)c + 0x2C) == it->second) { old = c; break; }
                }
                if (old) {
                    int rankOld = *(int *)((uintptr_t)old + 0x18) * 100 + *(int *)((uintptr_t)old + 0x3C);
                    int rankNew = *(int *)((uintptr_t)icon + 0x18) * 100 + *(int *)((uintptr_t)icon + 0x3C);
                    if (rankNew > rankOld)
                        it->second = id;
                } else {
                    it->second = id;
                }
            } else {
                killIconByWeapon[weaponID] = id;
            }
        }
    }

    g_killDecorationDiag.killIconConfigs = (int)killIcons.size();
    g_killDecorationDiag.killIconWeapons = (int)killIconByWeapon.size();
    loadKillIcons = true;
}

// ── The InventoryInfo the HUD reads (local player only) ─────────────────────
//
// `public class Weapon : Inventory` (dump 737390, TypeDefIndex 16255), and
// Inventory carries the record the in-match pipeline reads its kill decorations
// from: `protected InventoryInfo m_InventoryInfo; // 0x98` (dump 761114, with
// get_InventoryInfo 0xC362DCC / SetInventoryInfo 0xC362DD4). InventoryInfo
// itself (dump 1088476, TypeDefIndex 23000, GameEngine) is where the kill feed
// and the death effect look for them:
//     CrossHairID 0x30, SkinID 0x34, BluePrintID 0x38, FireEffectGroupID 0x3C,
//     KillEffectGroupID 0x40, KillBoard 0x48, KillIcon 0x4C, MythicArmorID 0x50,
//     MythicSigID 0x54, KillBroadcast 0x58, KillEffectLevel 0x5C
//
// The InventoryUtil / WeaponAttrData getter hooks above only change the local
// weapon's kill icon if the game really calls them for the local player AND
// accepts the id they return (an inlined getter cannot be hooked at all - see
// the install-time flag in the Camo tab). Writing the same values into this
// record covers both cases: it is the very object the HUD already holds, so the
// icon shows whether or not the getter path ran. Only the local player's own
// held weapon is ever touched, and only when the applied skin map has a value
// that differs from what the record already says (no per-frame writes).
inline void KillDecoration_ApplyLocalInventoryInfo()
{
    if (activeKillIcons.empty()) return;   // nothing applied -> nothing to write

    Pawn *localPawn = GamePlay::get_LocalPawn();
    if (!localPawn || !Tools::IsPtrValid((void *)localPawn)) return;

    Weapon *held = localPawn->get_CurrentWeapon();
    if (!held || !Tools::IsPtrValid((void *)held)) return;

    // Inventory.m_InventoryInfo - inherited by Weapon, so this is the live
    // record the game hands to the HUD.
    void *info = *(void **)((uintptr_t)held + 0x98);
    if (!info || !Tools::IsPtrValid(info)) return;

    const int weaponID = held->get_WeaponID();
    if (weaponID <= 0) return;

    auto iconIt = activeKillIcons.find(weaponID);
    const int killIcon = (iconIt != activeKillIcons.end()) ? iconIt->second : 0;
    if (killIcon <= 0) return;

    // Only the kill icon and the level are written. KillEffectGroupID (0x40) is
    // deliberately left alone: kill effects already work through the getter
    // hooks, and this map holds an effect id / asset id depending on which
    // source won at load time - writing that into the live record could replace
    // a working effect with the wrong id, which is a far worse outcome than a
    // missing icon.
    auto *p = (uint8_t *)info;
    if (killIcon > 0 && *(int *)(p + 0x4C) != killIcon) {
        *(int *)(p + 0x4C) = killIcon;
        g_killDecorationDiag.inventoryKillIconWrites++;
    }
    // Level 5 is what the mythic tier keys on (icon, killcam, death effect).
    if (*(uint8_t *)(p + 0x5C) != 5) {
        *(uint8_t *)(p + 0x5C) = 5;
        g_killDecorationDiag.inventoryLevelWrites++;
    }
}

// ── WeaponAttrData / Weapon: the LOCAL player's own weapon ───────────────────
// The four InventoryUtil overloads above resolve decorations for weapon data the
// client only knows through SyncWeaponExtraInfo. The local player's own weapon is
// not one of those: it is described by the client-side WeaponAttrData record
// (dump 49510) — m_BlueprintWeaponId @0x20, m_KillEffectItem @0x68,
// m_KillIconItem @0x78 — and by Weapon (dump 16255), whose
// get_KillEffectGroupIDOverride()@0x5167BDC is the kill effect the in-match
// pipeline actually plays. These three hooks are the local-player path.

uintptr_t WeaponAttrDataKillIconItemIDAddress = 0x8479334;   // WeaponAttrData.get_KillIconItemID
uintptr_t WeaponAttrDataKillEffectItemIDAddress = 0x847924C; // WeaponAttrData.get_KillEffectItemID
uintptr_t WeaponKillEffectGroupOverrideAddress = 0x5167BDC;  // Weapon.get_KillEffectGroupIDOverride

// Look a map up by the WeaponAttrData key (its blueprint weapon id).
static bool FindInMapAttrData(const std::unordered_map<int, int> &m, void *attrData, int &outVal) {
    if (!attrData || !Tools::IsPtrValid(attrData)) return false;
    const int key = (int)*(uint *)((uintptr_t)attrData + 0x20);   // m_BlueprintWeaponId
    g_killDecorationDiag.lastAttrKey = key;
    if (key == 0) return false;
    auto it = m.find(key);
    if (it != m.end() && it->second > 0) { outVal = it->second; return true; }
    return false;
}

uint (*orig_GetKillIconItemID)(void *) = nullptr;
uint my_GetKillIconItemID(void *attrData) {
    g_killDecorationDiag.iconHookCalls++;
    int val;
    if (FindInMapAttrData(activeKillIcons, attrData, val)) {
        g_killDecorationDiag.iconHookHits++;
        return (uint)val;
    }
    return orig_GetKillIconItemID ? orig_GetKillIconItemID(attrData) : 0;
}

uint (*orig_GetKillEffectItemID)(void *) = nullptr;
uint my_GetKillEffectItemID(void *attrData) {
    g_killDecorationDiag.effectItemHookCalls++;
    int val;
    if (FindInMapAttrData(activeKillEffectItems, attrData, val)) {
        g_killDecorationDiag.effectItemHookHits++;
        return (uint)val;
    }
    return orig_GetKillEffectItemID ? orig_GetKillEffectItemID(attrData) : 0;
}

int (*orig_WeaponKillEffectGroupOverride)(void *) = nullptr;
int my_WeaponKillEffectGroupOverride(void *weapon) {
    g_killDecorationDiag.effectGroupHookCalls++;
    if (weapon && Tools::IsPtrValid(weapon)) {
        Weapon *w = (Weapon *)weapon;
        const int weaponID = w->get_WeaponID();
        auto it = activeKillEffects.find(weaponID);
        if (it != activeKillEffects.end() && it->second > 0) {
            g_killDecorationDiag.effectGroupHookHits++;
            return it->second;
        }
    }
    return orig_WeaponKillEffectGroupOverride ? orig_WeaponKillEffectGroupOverride(weapon) : 0;
}

#define ReadInt(base, offset) (*(int *)((uintptr_t)(base) + (offset)))
#define ReadBool(base, offset) (*(bool *)((uintptr_t)(base) + (offset)))
#define ReadFloat(base, offset) (*(float *)((uintptr_t)(base) + (offset)))
#define ReadByte(base, offset) (*(uint8_t *)((uintptr_t)(base) + (offset)))
#define READ_PTR(type, base, offset) (*(type **)((uintptr_t)(base) + (offset)))

// Game quality ladder (dump.cs 683883-683888, WEAPON_QUALITY_COLOR_WHITE /
// GREEN / BULE / PURPLE / GOLD / RED): ColorID 0 = Common (white), 1 = Uncommon
// (green), 2 = Rare (blue), 3 = Epic (purple), 4 = Legendary (gold),
// 5 = Mythic (red). The old map folded 0/1/2 into [C], which made a
// mythic/legendary/epic/rare/uncommon/common search filter impossible.
inline std::string GetRarityPrefix(int colorID) {
    switch (colorID) {
        case 5: return "[M] ";
        case 4: return "[L] ";
        case 3: return "[E] ";
        case 2: return "[R] ";
        case 1: return "[U] ";
        default: return "[C] ";
    }
}

void LoadCharacterSkins() {

    if (loadCharacter)
        return;

    std::vector<void*> charModels;
    std::vector<void*> itemRes;
    std::vector<void*> roleConfs;
    std::vector<void*> roleSkins;
    std::vector<void*> rolePacks;
    std::vector<void*> deadboxSkins;

    {
        std::lock_guard<std::mutex> lock(g_dataMutex);

        charModels   = CharacterModelConfigInstance;
        itemRes      = itemResourceConfigInstance;
        roleConfs    = RoleConfConfigInstance;
        roleSkins    = RoleSkinConfigInstance;
        rolePacks    = RolePackConfConfigInstance;
        deadboxSkins = BRDeadboxSkinConfigInstance;
    }

    // Each skin family is built from its own config tiers and must not wait for
    // all of them. The CHARACTER join only needs CharacterModelConfig x
    // ItemResourceConfig x RoleConfConfig (RolePackConfConfig just refines the
    // pack fields), so one tier the game never constructs in a lobby - a
    // BRDeadboxSkin or a RoleSkin watch config - used to keep this whole
    // function early-returning forever: the characters tab stayed empty while
    // the weapon tab loaded, which is the "character skins not loading" report.
    const bool charTiersReady = !charModels.empty() && !itemRes.empty() && !roleConfs.empty();

    if (!loadWatch && !roleSkins.empty()) {
        watch.clear();
    }

    if (!loadDeadbox && !deadboxSkins.empty() && !roleConfs.empty()) {
        deadboxF.clear();
    }

    if (!loadCharacter && charTiersReady) {
        charData.clear();
        g_targetCharacters.clear();
    }

    std::string lastKnownName = "";
    void* lastKnownLocId = nullptr;

    int lastKnownI = 0;
    int lastKnownJ = 0;
    int lastKnownH = 0;
    int lastKnownEntry = 0;
    int lastKnownGest = 0;
    int lastKnownHand = 0;
    int lastKnownKillS = 0;

    if (!loadWatch && !roleSkins.empty())
    for (auto &a : roleSkins) {

        if (!a || !Tools::IsPtrValid(a))
            continue;

        auto *fx = (RoleSkinFields *)((uintptr_t)a + 0x10);

        if (!Tools::IsPtrValid(fx))
            continue;

        if ((fx->FxAssetID_1P ^ 0) != 0) {

            std::string n = GetNameString(GetNameRoleSkin, (void*)a);

            if (!n.empty()) {

                watch.push_back({
                    GetRarityPrefix(fx->ColorID) + n,
                    fx->FxAssetID_1P
                });
            }
        }
    }

    if (!roleSkins.empty())
        loadWatch = true;      // every RoleSkin config was walked once

    std::string _g = "";

    if (!loadDeadbox && !deadboxSkins.empty() && !roleConfs.empty())
    for (auto &z : deadboxSkins) {

        if (!z || !Tools::IsPtrValid(z))
            continue;

        auto *y = (BRDeadboxSkinFields *)((uintptr_t)z + 0x10);

        if (!Tools::IsPtrValid(y))
            continue;

        bool __b = false;

        for (auto &q : roleConfs) {

            if (!q || !Tools::IsPtrValid(q))
                continue;

            std::string s = GetNameString(GetRoleConfName, q);

            std::string boxClueName = s + " Deadbox";

            if ((y->ID & 0xFFFFFFF) != 0) {

                auto fx = (int (*)(void *))(getRealOffset(GetDeadBoxIDAddress));

                if (!Tools::IsPtrValid((void*)fx))
                    continue;

                int k = fx(q);

                if (y->ID == k && !__b) {

                    auto dump = [&](const std::string &nm) {

                        deadboxF.push_back({
                            GetRarityPrefix(y->ColorID) + nm,
                            {
                                y->ColorID,
                                y->DeadBoxEffectAsset,
                                y->Flag,
                                y->FlagAsset,
                                y->ModelAsset3P,
                                y->ModelAssetUI
                            }
                        });
                    };

                    if (y->ColorID == 5) {

                        _g = boxClueName;

                        dump(boxClueName);

                        __b = true;

                    } else {

                        dump(boxClueName);
                    }
                }

                if (__b)
                    break;

                if (k + 1 == y->ID) {

                    if (y->ColorID == 5 &&
                        y->FlagAsset != 0 &&
                        !_g.empty()) {

                        std::string v = _g + " (Variant)";

                        deadboxF.push_back({
                            GetRarityPrefix(y->ColorID) + v,
                            {
                                y->ColorID,
                                y->DeadBoxEffectAsset,
                                y->Flag,
                                y->FlagAsset,
                                y->ModelAsset3P,
                                y->ModelAssetUI
                            }
                        });
                    }
                }
            }
        }
    }

    if (!deadboxSkins.empty())
        loadDeadbox = true;    // every BRDeadboxSkin config was walked once

    if (!loadCharacter && charTiersReady)
    for (auto X0 : charModels) {

        if (!X0 || !Tools::IsPtrValid(X0))
            continue;

        auto *X1 = (CharacterModelFields *)((uintptr_t)X0 + 0x10);

        if (!Tools::IsPtrValid(X1))
            continue;

        auto A = X1->ItemID;

        auto B = X1->BRBagModel;
        auto C = X1->BRHeadModel;
        auto D = X1->BRLobby;
        auto E = X1->BRModel;
        auto F = X1->BindEffect1P;
        auto G = X1->ChangeClipEffect1P;
        auto H = X1->DefaultModelID;
        auto I = X1->Guarder1P;
        auto J = X1->Guarder3P;
        auto K = X1->GuarderBagModel;
        auto L = X1->GuarderHeadModel;
        auto M = X1->GuarderLobby;
        auto N = X1->Traitor1P;

        for (auto Y0 : itemRes) {

            if (!Y0 || !Tools::IsPtrValid(Y0))
                continue;

            auto *Y1 = (ItemResourceFields *)((uintptr_t)Y0 + 0x10);

            if (!Tools::IsPtrValid(Y1))
                continue;

            if (A != Y1->AvatarModelID)
                continue;

            auto P = Y1->FxAssetID;
            auto Q = Y1->InventoryModelID;
            auto R = Y1->ModelAssetIDRaw;

            auto S = Y1->UIMiniSpriteNameIndex;
            auto T = Y1->UISmallSpriteNameIndex;
            auto U = Y1->UISpriteNameIndex;
            auto V = Y1->UISquareSpriteNameIndex;

            auto W = Y1->ID;

            for (auto Z0 : roleConfs) {

                if (!Z0 || !Tools::IsPtrValid(Z0))
                    continue;

                // RoleConfConfig's first field is at 0x10 (dump 818159). Casting
                // at +0x18 made every read land 8 bytes high, so the ID this
                // join matches on was KeepInCatalog (0x3C) instead of ID (0x34)
                // and NO character ever matched - the empty Character tab.
                auto *Z1 = (RoleConfFields *)((uintptr_t)Z0 + 0x10);

                if (!Tools::IsPtrValid(Z1))
                    continue;

                if (Z1->ID != W)
                    continue;

                auto a = Z1->roleLeftArmID;
                auto b = Z1->roleFinalSuitID;
                auto c = Z1->roleBasicHologramID;
                auto d = Z1->ColorID;
                auto e = Z1->ColorSubID;
                auto f = Z1->ShowRare;
                auto g = Z1->RoleLvGroupID;
                auto h = Z1->RolePackID;
                auto t = Z1->LOCID_Name;

                int i = 0;
                int entry = 0;
                int gest = 0;
                int hand = 0;
                int j = 0;
                int killS = 0;

                for (auto RP : rolePacks) {

                    if (!RP || !Tools::IsPtrValid(RP))
                        continue;

                    // RolePackConfConfig's first field is at 0x10 (dump 818902),
                    // and the struct now follows the live (alphabetical) order.
                    auto *rpF = (RolePackFields *)((uintptr_t)RP + 0x10);

                    if (!Tools::IsPtrValid(rpF))
                        continue;

                    if (rpF->RolePackID != h)
                        continue;

                    if (rpF->LoadingFrame &&
                        rpF->LobbySceneType == 0) {

                        i = rpF->RolePackID;
                        j = rpF->LoadingFrame;
                        entry = rpF->EntryAnimID;
                        gest = rpF->GestureId;
                        hand = rpF->HandEffectUI;
                        killS = rpF->KillStreakSkinID;
                    }
                }

                std::string n0 = GetNameString(GetRoleConfName, Z0);

                if (!n0.empty() && n0.length() > 0) {

                    lastKnownName = n0;
                    lastKnownLocId = t;

                    lastKnownI = i;
                    lastKnownJ = j;
                    lastKnownH = h;

                    lastKnownEntry = entry;
                    lastKnownGest = gest;
                    lastKnownHand = hand;
                    lastKnownKillS = killS;

                } else {

                    if (!lastKnownName.empty() &&
                        lastKnownLocId != nullptr) {

                        n0 = lastKnownName + " A+";

                        t = lastKnownLocId;

                        i = lastKnownI;
                        j = lastKnownJ;
                        h = lastKnownH;

                        entry = lastKnownEntry;
                        gest = lastKnownGest;
                        hand = lastKnownHand;
                        killS = lastKnownKillS;

                    } else {

                        continue;
                    }
                }

                std::string Zz = GetRarityPrefix(d) + n0;

                charData.push_back({

                    Zz,

                    {
                        B,
                        C,
                        D,
                        E,
                        F,
                        G,
                        H,
                        I,
                        J,
                        K,
                        L,
                        M
                    },

                    {
                        P,
                        Q,
                        R
                    },

                    {
                        S,
                        T,
                        U,
                        V
                    },

                    {
                        a,
                        b,
                        c,
                        d,
                        e,
                        f,
                        g,
                        h
                    },

                    {
                        t
                    },

                    {
                        i,
                        entry,
                        gest,
                        hand,
                        j,
                        killS
                    }
                });

                if (!n0.empty()) {

                    g_targetCharacters.push_back({

                        n0,
                        N,
                        X1->Traitor3P,
                        (int)W,
                        (int)Z1->ID,
                        h
                    });
                }
            }

            break;
        }
    }

    // Latch only once the join actually had all three of its inputs. Latching
    // unconditionally marked the character list "loaded" on the very first poll
    // (1.5 s after the config tables start filling), after which the early
    // return at the top of this function kept it empty for the whole session -
    // the second half of "character skins still isn't loading".
    if (charTiersReady)
        loadCharacter = true;   // every CharacterModel config was walked once

    if (!g_targetCharacters.empty()) {

        bool foundCharly = false;

        for (size_t idx = 0; idx < g_targetCharacters.size(); ++idx) {

            if (g_targetCharacters[idx].name == "Charly") {

                g_selectedTargetCharIndex = (int)idx;

                foundCharly = true;

                break;
            }
        }

        if (!foundCharly) {

            for (size_t idx = 0; idx < g_targetCharacters.size(); ++idx) {

                std::string lowName = g_targetCharacters[idx].name;

                std::transform(
                    lowName.begin(),
                    lowName.end(),
                    lowName.begin(),
                    ::tolower
                );

                if (lowName.find("charly") != std::string::npos) {

                    g_selectedTargetCharIndex = (int)idx;

                    break;
                }
            }
        }
    }
}

void LoadWeaponSkins() {
    if (loadskinhack) return;

    std::vector<void*> weaponConfs, itemInvs, weaponAssets, weaponFires, weaponExtras, killEffects, mythicArmors, mythicSights, itemRes;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponConfs = weaponConfInstance;
        itemInvs = itemInventoryInstance;
        weaponAssets = weaponAssetGroupInstance;
        weaponFires = weaponFireEffectInstance;
        weaponExtras = weaponExtraInstance;
        killEffects = killEffectItemInstance;
        mythicArmors = mythicArmorInstance;
        mythicSights = mythicSightInstance;
        itemRes = itemResourceConfigInstance;
    }

    if (weaponConfs.empty() || itemInvs.empty() || weaponAssets.empty() || weaponFires.empty() ||
        weaponExtras.empty() || killEffects.empty() || mythicArmors.empty() || mythicSights.empty() || itemRes.empty()) {
        return;
    }

    std::unordered_map<int, void*> itemInvByItemID;
    std::unordered_map<int, void*> weaponAssetByID;
    std::unordered_map<int, void*> weaponFireByID;
    std::unordered_map<int, void*> weaponExtraByID;
    std::unordered_map<int, int>   mythicArmorBySecondTab;
    std::unordered_map<int, int>   mythicSightByWeapon;

    std::unordered_map<int, void*> itemResByID;

    for (void* inv : itemInvs) {
        if (!inv || !Tools::IsPtrValid(inv)) continue;
        int itemID = *(int*)((uintptr_t)inv + 0x20);
        itemInvByItemID[itemID] = inv;
    }

    for (void* asset : weaponAssets) {
        if (!asset || !Tools::IsPtrValid(asset)) continue;
        int id = *(int*)((uintptr_t)asset + 0x44);
        weaponAssetByID[id] = asset;
    }

    for (void* fire : weaponFires) {
        if (!fire || !Tools::IsPtrValid(fire)) continue;
        int id = *(int*)((uintptr_t)fire + 0x80);
        weaponFireByID[id] = fire;
    }

    for (void* extra : weaponExtras) {
        if (!extra || !Tools::IsPtrValid(extra)) continue;
        weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x10);
        if (!Tools::IsPtrValid(weaponextraFields)) continue;
        weaponExtraByID[weaponextraFields->ID] = extra;
    }

    for (void* armor : mythicArmors) {
        if (!armor || !Tools::IsPtrValid(armor)) continue;
        mythicarmorFields = (MythicArmorFields*)((uintptr_t)armor + 0x14);
        if (!Tools::IsPtrValid(mythicarmorFields)) continue;
        if (mythicarmorFields->ThirdTab == 5) {
            mythicArmorBySecondTab[mythicarmorFields->SecondTab] = mythicarmorFields->AssetID;
        }
    }

    for (void* sight : mythicSights) {
        if (!sight || !Tools::IsPtrValid(sight)) continue;
        mythicsightFields = (MythicSightFields*)((uintptr_t)sight + 0x10);
        if (!Tools::IsPtrValid(mythicsightFields)) continue;
        auto* equipArray = *(Array<int>**)((uintptr_t)sight + 0x38);
        if (equipArray && Tools::IsPtrValid(equipArray) && equipArray->getLength() > 0) {
            int lastIndex = equipArray->getLength() - 1;
            int weaponID = equipArray->m_Items[lastIndex];
            int sightID = *(int*)((uintptr_t)sight + 0x14);
            mythicSightByWeapon[weaponID] = sightID;
        }
    }

    for (void* kill : killEffects) {
        if (!kill || !Tools::IsPtrValid(kill)) continue;
        killeffectFields = (KillEffectItemFields*)((uintptr_t)kill + 0x10);
        if (!Tools::IsPtrValid(killeffectFields)) continue;
        auto* equipArray = *(Array<int>**)((uintptr_t)kill + 0x90);
        if (equipArray && Tools::IsPtrValid(equipArray) && equipArray->getLength() > 0) {
            int lastIndex = equipArray->getLength() - 1;
            int weaponID = equipArray->m_Items[lastIndex];
            // RealAssetIDs is the FIRST field of KillEffectItemConfConfig (0x10,
            // dump 812074); reading it at 0x18 landed on IsAutoDemolition/ColorID
            // and left the whole kill-effect map empty.
            auto* realAssetIDs = *(Array<int>**)((uintptr_t)kill + 0x10);
            if (realAssetIDs && Tools::IsPtrValid(realAssetIDs) && realAssetIDs->getLength() > 0) {
                int lastAssetIndex = realAssetIDs->getLength() - 1;
                int lastAssetID = realAssetIDs->m_Items[lastAssetIndex];
                killEffectByWeapon[weaponID] = lastAssetID;
                // The client-side per-weapon record (WeaponAttrData.m_KillEffectItem)
                // is keyed by the config's item ID (ID @0x2C), not by the asset ID,
                // so remember which item produced this effect asset.
                const int effectItemID = *(int *)((uintptr_t)kill + 0x2C);
                if (effectItemID > 0 && lastAssetID > 0)
                    killEffectItemByAssetId[lastAssetID] = effectItemID;
            }
        }
    }

    for (void* res : itemRes) {
        if (!res || !Tools::IsPtrValid(res)) continue;
        itemFields = (ItemResourceFields*)((uintptr_t)res + 0x10);
        if (!Tools::IsPtrValid(itemFields)) continue;
        itemResByID[itemFields->ID] = res;
    }

    for (void* conf : weaponConfs) {
        if (!conf || !Tools::IsPtrValid(conf)) continue;
        int baseID = *(int*)((uintptr_t)conf + 0x34);
        int confID = *(int*)((uintptr_t)conf + 0x40);

        auto itItem = itemInvByItemID.find(confID);
        if (itItem == itemInvByItemID.end()) continue;
        void* item = itItem->second;
        if (!item || !Tools::IsPtrValid(item)) continue;
        int itemIDbase = *(int*)((uintptr_t)item + 0x20);
        int itemBaseModified;
        int itemBase;
        if (baseID == itemIDbase) {
            itemBase = *(int*)((uintptr_t)item + 0x20);
            itemBaseModified = itemBase + 200;
        }
        if (confID == itemIDbase) {
            uint8_t confColorID = *(uint8_t*)((uintptr_t)conf + 0x22);
            int itemIDskin2 = *(int*)((uintptr_t)item + 0x24);
            int itemIDskin3 = *(int*)((uintptr_t)item + 0x28);

            std::string weaponName = GetNameString(WeaponConfName, conf);
            std::string displayName = GetRarityPrefix(confColorID) + weaponName;

            if (nameCountMap.find(displayName) != nameCountMap.end()) {
                nameCountMap[displayName]++;
                displayName += " +" + std::to_string(nameCountMap[displayName]);
            } else {
                nameCountMap[displayName] = 0;
            }

            if (!displayName.empty()) {
                int fireIds = 0, fireIds2 = 0, assetIds = 0;
                int originalFireID = 0;

                auto itAsset = weaponAssetByID.find(itemIDskin2);
                if (itAsset != weaponAssetByID.end()) {
                    void* asset = itAsset->second;
                    int fireEffectID = *(int*)((uintptr_t)asset + 0x40);
                    assetIds = itemIDskin2;

                    auto itFire = weaponFireByID.find(fireEffectID);
                    if (itFire != weaponFireByID.end()) {
                        void* fireConf = itFire->second;
                        int fireID = *(int*)((uintptr_t)fireConf + 0x80);
                        int assetIdBulletSmoke = *(int*)((uintptr_t)fireConf + 0x1C);

                        if (displayName.find("[M]") != std::string::npos) {
                            if (fireEffectID == fireID) {
                                if (assetIdBulletSmoke != 0) {
                                    fireIds = fireID;
                                    originalFireID = fireID;
                                } else {
                                    int nextFireID = fireID + 1;
                                    bool found = false;
                                    for (int i = 0; i < 10; i++) {
                                        auto itNext = weaponFireByID.find(nextFireID);
                                        if (itNext != weaponFireByID.end()) {
                                            void* nextFireConf = itNext->second;
                                            int nextAssetIdBulletSmoke = *(int*)((uintptr_t)nextFireConf + 0x1C);
                                            if (nextAssetIdBulletSmoke != 0) {
                                                fireIds = nextFireID;
                                                found = true;
                                                break;
                                            }
                                        }
                                        nextFireID++;
                                    }
                                    if (fireIds == 0) fireIds = fireID;
                                    fireIds2 = fireID;
                                }
                            }
                        }
                    }
                }

                int confBaseSkin = 0, confSkinID = 0, confBrocastID = 0, confColor = 0, confBluePrintID = 0;
                if (confID == itemIDbase) {
                    confBaseSkin = ReadInt(conf, 0x34);
                    confColor = ReadByte(conf, 0x22);
                    confSkinID = ReadInt(conf, 0x38);
                    confBrocastID = ReadInt(conf, 0x3C);
                    confBluePrintID = ReadByte(conf, 0x1C);
                }

                int mythicArmor = 0, deadReplay = 0, killEffect = 0, extraOrig = 0;

                auto itExtraBase = weaponExtraByID.find(baseID);
                if (itExtraBase != weaponExtraByID.end()) {
                    void* extra = itExtraBase->second;
                    weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x10);
                    if (Tools::IsPtrValid(weaponextraFields)) {
                        extraOrig = weaponextraFields->ID;
                    }
                }
                auto itExtraConf = weaponExtraByID.find(confID);
                if (itExtraConf != weaponExtraByID.end()) {
                    void* extra = itExtraConf->second;
                    weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x10);
                    if (Tools::IsPtrValid(weaponextraFields)) {
                        deadReplay = weaponextraFields->DefaultDeadReplayEffectId;
                        killEffect = weaponextraFields->DefaultKillEffectId;
                    }
                }

                auto itMythicArmor = mythicArmorBySecondTab.find(itemIDskin3);
                if (itMythicArmor != mythicArmorBySecondTab.end()) {
                    if (displayName.find("[M]") != std::string::npos)
                        mythicArmor = itMythicArmor->second;
                }

                int sightMythic = 0;
                auto itMythicSight = mythicSightByWeapon.find(itemIDskin3);
                if (itMythicSight != mythicSightByWeapon.end()) {
                    if (displayName.find("[M]") != std::string::npos)
                        sightMythic = itMythicSight->second;
                }

                int killEffectFromItem = 0;
                auto itKillEffect = killEffectByWeapon.find(itemIDskin3);
                if (itKillEffect != killEffectByWeapon.end())
                    killEffectFromItem = itKillEffect->second;
                else {
                    itKillEffect = killEffectByWeapon.find(confID);
                    if (itKillEffect != killEffectByWeapon.end())
                        killEffectFromItem = itKillEffect->second;
                    else {
                        itKillEffect = killEffectByWeapon.find(baseID);
                        if (itKillEffect != killEffectByWeapon.end())
                            killEffectFromItem = itKillEffect->second;
                        else {
                            itKillEffect = killEffectByWeapon.find(itemBase);
                            if (itKillEffect != killEffectByWeapon.end())
                                killEffectFromItem = itKillEffect->second;
                        }
                    }
                }
                if (killEffectFromItem != 0)
                    killEffect = killEffectFromItem;

                uint16_t spr1 = 0;
                uint16_t spr2 = 0;
                uint16_t spr3 = 0;
                uint16_t spr4 = 0;

                int xItem1 = 0;
                int xItem2 = 0;
                int xItem3 = 0;

                auto itItemRes = itemResByID.find(confID);
                if (itItemRes != itemResByID.end()) {
                    void* itemResource = itItemRes->second;
                    itemFields = (ItemResourceFields*)((uintptr_t)itemResource + 0x10);
                    if (Tools::IsPtrValid(itemFields)) {
                        xItem1 = itemFields->FxAssetID;
                        xItem2 = itemFields->InventoryModelID;
                        xItem3 = itemFields->ModelAssetIDRaw;
                        spr1 = itemFields->UISmallSpriteNameIndex;
                        spr2 = itemFields->UIMiniSpriteNameIndex;
                        spr3 = itemFields->UISpriteNameIndex;
                        spr4 = itemFields->UISquareSpriteNameIndex;
                    }
                }

                itemData.push_back({displayName,
                    {itemBase, itemIDskin2, itemIDskin3, itemBaseModified},
                    {confBaseSkin, confColor, confID, confBrocastID, confBluePrintID},
                    {extraOrig, mythicArmor, sightMythic, deadReplay, killEffect},
                    {assetIds, fireIds, fireIds2},
                    {xItem1, xItem2, xItem3},
                    {spr1, spr2, spr3, spr4}});
            }
        }
    }

    loadskinhack = true;
}

void LoadPlaneSkins() {
    std::vector<void*> dropPlaneSkins;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        dropPlaneSkins = BRDropPlaneSkinConfigInstance;
    }

    if (dropPlaneSkins.empty()) return;

    for (void* plane : dropPlaneSkins) {
        if (!plane || !Tools::IsPtrValid(plane)) continue;
        dropplaneFields = (BRDropPlaneSkinFields*)((uintptr_t)plane + 0x10);
        if (!Tools::IsPtrValid(dropplaneFields)) continue;
        std::string planeName = GetNameString(GetDropPlaneName, plane);
        if (dropplaneFields->ModelAsset1P != 0 && !getplane[planeName]) {
            getplane[planeName] = true;
            std::string prefix = GetRarityPrefix(dropplaneFields->ColorID);
            dropplane.push_back({prefix + planeName, {dropplaneFields->ColorID, dropplaneFields->ModelAsset1P, dropplaneFields->ModelAsset3P, dropplaneFields->ModelAssetCutScene, dropplaneFields->ModelAssetUI, dropplaneFields->Priority}});
            if (getplane.size() == dropPlaneSkins.size())
                break;
        }
    }
}

// ── VehicleSkis / VehicleSkinHelper forward declarations ─────────────────────

inline bool SkinPtr(void* ptr) {
    return ptr != nullptr && Tools::IsPtrValid(ptr);
}

inline int GetIl2CppArrayLen(void* arr) {
    if (!SkinPtr(arr)) return 0;
    const int len = *(int*)((uintptr_t)arr + 0x18);
    return len >= 0 && len < 512 ? len : 0;
}

inline void* VehicleSkinParams(void* skinConf) {
    if (!SkinPtr(skinConf)) return nullptr;
    return *(void**)((uintptr_t)skinConf + 0xC8);
}

inline void* FindVehicleSkinConfigById(int skinId) {
    if (skinId <= 0) return nullptr;
    auto exact = activeVehicleSkinConfs.find(skinId);
    if (exact != activeVehicleSkinConfs.end() && SkinPtr(exact->second))
        return exact->second;
    for (void* skin : vehicleSkinConfigInstance) {
        if (!SkinPtr(skin)) continue;
        auto* fields = (VehicleSkinConfFields*)((uintptr_t)skin + 0x10);
        if (SkinPtr(fields) && (int)fields->ID == skinId)
            return skin;
    }
    return nullptr;
}

inline int ActiveVehicleSkinIdForType(int vehicleType) {
    auto it = activeVehicleSkins.find(vehicleType);
    return it != activeVehicleSkins.end() ? it->second : 0;
}

inline int ActiveVehicleSkinIdForConf(void* skinConf) {
    if (!SkinPtr(skinConf)) return 0;
    auto* fields = (VehicleSkinConfFields*)((uintptr_t)skinConf + 0x10);
    if (!SkinPtr(fields)) return 0;
    auto byId = activeVehicleSkinsById.find((int)fields->VehicleId);
    if (byId != activeVehicleSkinsById.end()) return byId->second;
    return ActiveVehicleSkinIdForType((int)fields->VehicleId);
}

// Function pointers resolved at runtime
inline void (*VehicleSkinHelper_ChangeVehicleSkinId)(void* vehicleObj, uint vehicleSkinId) = nullptr;
inline void (*VehicleSkinHelper_ChangeVehicleSkinConf)(void* vehicleObj, void* vehConf) = nullptr;
inline void (*VehicleSkis_ApplySkin)(void* instance) = nullptr;

// Hook orig pointers
inline void (*orig_VehicleSkis_SetupSkin)(void* instance, uint skinID, bool findItem) = nullptr;
inline void (*orig_VehicleSkis_ChangeSkinId)(void* instance, uint skinID) = nullptr;
inline void (*orig_VehicleSkis_ChangeSkinConf)(void* instance, void* skinConf) = nullptr;
inline void (*orig_VehicleSkis_BuildVehicleModel)(void* instance) = nullptr;
inline void (*orig_VehicleSkis_LoadModelComplete)(void* instance, int assetID, void* go) = nullptr;

inline int ActiveSkisSkinId() {
    const int selected = ActiveVehicleSkinIdForType(31707110);
    return selected > 0 ? selected : 0;
}

inline void ForceSkisSkinFields(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    auto* fields = SkinPtr(conf) ? (VehicleSkinConfFields*)((uintptr_t)conf + 0x10) : nullptr;
    const int mesh = SkinPtr(fields) ? fields->NewVehicleResId : 0;
    *(uint*)((uintptr_t)instance + 0x88) = (uint)selected;
    if (mesh > 0) {
        *(int*)((uintptr_t)instance + 0x20) = mesh;
        *(int*)((uintptr_t)instance + 0xA8) = mesh;
    }
}

inline void ApplySkisSkinConf(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0 || orig_VehicleSkis_ChangeSkinConf == nullptr) return;
    void* conf = FindVehicleSkinConfigById(selected);
    if (SkinPtr(conf)) orig_VehicleSkis_ChangeSkinConf(instance, conf);
}

inline void ApplySkisSkinToObject(void* targetObj, int selected) {
    if (!SkinPtr(targetObj) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    if (!SkinPtr(conf)) return;
    if (VehicleSkinHelper_ChangeVehicleSkinId != nullptr)
        VehicleSkinHelper_ChangeVehicleSkinId(targetObj, (uint)selected);
    if (VehicleSkinHelper_ChangeVehicleSkinConf != nullptr)
        VehicleSkinHelper_ChangeVehicleSkinConf(targetObj, conf);
}

inline void ApplySkisMaterials(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    void* mats = VehicleSkinParams(conf);
    if (!SkinPtr(mats) || GetIl2CppArrayLen(mats) <= 0 || VehicleSkis_ApplySkin == nullptr) return;
    *(void**)((uintptr_t)instance + 0x90) = mats;
    ForceSkisSkinFields(instance, selected);
    VehicleSkis_ApplySkin(instance);
}

inline void hook_VehicleSkis_SetupSkin(void* instance, uint skinID, bool findItem) {
    uint nextSkin = skinID;
    const int selected = ActiveSkisSkinId();
    if (selected > 0) nextSkin = (uint)selected;
    ForceSkisSkinFields(instance, (int)nextSkin);
    if (orig_VehicleSkis_SetupSkin != nullptr)
        orig_VehicleSkis_SetupSkin(instance, nextSkin, findItem);
    ForceSkisSkinFields(instance, (int)nextSkin);
    ApplySkisSkinConf(instance, (int)nextSkin);
}

inline void hook_VehicleSkis_ChangeSkinId(void* instance, uint skinID) {
    uint nextSkin = skinID;
    const int selected = ActiveSkisSkinId();
    if (selected > 0) nextSkin = (uint)selected;
    ForceSkisSkinFields(instance, (int)nextSkin);
    if (orig_VehicleSkis_ChangeSkinId != nullptr)
        orig_VehicleSkis_ChangeSkinId(instance, nextSkin);
    ForceSkisSkinFields(instance, (int)nextSkin);
    ApplySkisSkinConf(instance, (int)nextSkin);
}

inline void hook_VehicleSkis_ChangeSkinConf(void* instance, void* skinConf) {
    void* nextSkin = skinConf;
    const int selectedId = ActiveSkisSkinId() > 0 ? ActiveSkisSkinId() : ActiveVehicleSkinIdForConf(skinConf);
    if (selectedId > 0) {
        void* selected = FindVehicleSkinConfigById(selectedId);
        if (SkinPtr(selected)) nextSkin = selected;
    }
    ForceSkisSkinFields(instance, selectedId);
    if (orig_VehicleSkis_ChangeSkinConf != nullptr)
        orig_VehicleSkis_ChangeSkinConf(instance, nextSkin);
    ForceSkisSkinFields(instance, selectedId);
}

inline void hook_VehicleSkis_BuildVehicleModel(void* instance) {
    const int selected = ActiveSkisSkinId();
    ForceSkisSkinFields(instance, selected);
    if (orig_VehicleSkis_BuildVehicleModel != nullptr)
        orig_VehicleSkis_BuildVehicleModel(instance);
    ForceSkisSkinFields(instance, selected);
    ApplySkisSkinConf(instance, selected);
}

inline void hook_VehicleSkis_LoadModelComplete(void* instance, int assetID, void* go) {
    const int selected = ActiveSkisSkinId();
    ForceSkisSkinFields(instance, selected);
    if (orig_VehicleSkis_LoadModelComplete != nullptr)
        orig_VehicleSkis_LoadModelComplete(instance, assetID, go);
    ForceSkisSkinFields(instance, selected);
    ApplySkisSkinToObject(go, selected);
    ApplySkisSkinConf(instance, selected);
    ApplySkisMaterials(instance, selected);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoadSnowboardSkins() {
    std::vector<void*> skinConfs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        skinConfs = vehicleSkinConfigInstance;
    }
    if (skinConfs.empty()) return;

    static std::unordered_map<int, bool> loadedIds;

    for (void* skin : skinConfs) {
        if (!skin || !Tools::IsPtrValid(skin)) continue;
        auto* fields = (VehicleSkinConfFields*)((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;
        if (fields->VehicleId != 31707110) continue;
        int skinId = (int)fields->ID;
        if (skinId <= 0 || loadedIds[skinId]) continue;
        loadedIds[skinId] = true;
        std::string prefix = GetRarityPrefix((int)fields->ColorID);
        // Real name when the localisation table is ready (dump 823011); the id
        // form is the fallback so the entry stays clickable and unique.
        std::string skinName = GetNameString(GetVehicleSkinName, skin);
        std::string name = skinName.empty()
            ? prefix + "Snowboard Skin " + std::to_string(skinId)
            : prefix + skinName;
        snowboardData.push_back({name, skinId});
    }
}

// ── Camo catalogue ────────────────────────────────────────────────────────
// Fills camoData from every WeaponCamosConfig row the ctor hook captured, so
// the Camo tab lists the camos this build actually has instead of a
// hand-written set. The table carries one row per (camo, weapon) pair - the
// game's own lookup is GetCamosConfig(camoID, weaponID) - so CamoItemID is the
// key and repeats are folded together by InsertCamo (Skin/Fields.h): the first
// row supplies the entry and a later one upgrades the fields it could not
// answer, which also covers rows whose name table was not ready yet.
void LoadCamos() {
    std::vector<void *> camoConfs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        camoConfs = weaponCamosConfigInstance;
    }
    if (camoConfs.empty()) return;

    for (void *camo : camoConfs) {
        if (!camo || !Tools::IsPtrValid(camo)) continue;
        // WeaponCamosFields starts at CamoType (dump 823828) - same +0x10 cast
        // Skin/Camo.h already reads the table with.
        auto *fields = (WeaponCamosFields *)((uintptr_t)camo + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;
        uint camoItemID = fields->CamoItemID;
        if (camoItemID == 0) continue;

        // get_Name resolves through the localisation table, which is empty until
        // the game loads it, hence the placeholder name. (UnlockedCamo is this
        // same getter - 0xAF7E580, WeaponCamosConfig.get_Name, dump 823888 -
        // which is what Skin/Camo.h reads camo names with.)
        std::string name = GetNameString(UnlockedCamo, camo);
        if (name.empty())
            name = "Camo " + std::to_string(camoItemID);

        InsertCamo((int)camoItemID, name, fields->TextureAssetID, fields->BelongWeaponID);
    }
}

void Skins_Thread()
{
    while (!m_unity)
    {
        sleep(1);
    }

    DobbyHook((void*)getRealOffset(Item2InventoryAddress), (void*)my_Item2InventoryCtor, (void**)&orig_Item2InventoryCtor);
    DobbyHook((void*)getRealOffset(WeaponConfExtraAddress), (void*)my_WeaponConfExtraCtor, (void**)&orig_WeaponConfExtraCtor);
    DobbyHook((void*)getRealOffset(WeaponFireEffectAddress), (void*)my_WeaponFireEffectCtor, (void**)&orig_WeaponFireEffectCtor);
    DobbyHook((void*)getRealOffset(WeaponConfAddress), (void*)my_WeaponConfCtor, (void**)&orig_WeaponConfCtor);
    DobbyHook((void*)getRealOffset(WeaponAssetGroupAddress), (void*)my_WeaponAssetGroupCtor, (void**)&orig_WeaponAssetGroupCtor);
    DobbyHook((void*)getRealOffset(MythicArmorConfigAddress), (void*)my_MythicArmorCtor, (void**)&orig_MythicArmorCtor);
    DobbyHook((void*)getRealOffset(MythicSightConfigAddress), (void*)my_MythicSightCtor, (void**)&orig_MythicSightCtor);
    DobbyHook((void*)getRealOffset(WeaponSkinAddress), (void*)my_WeaponSkinCtor, (void**)&orig_WeaponSkinCtor);
    DobbyHook((void*)getRealOffset(WeaponCamosAddress), (void*)my_WeaponCamosCtor, (void**)&orig_WeaponCamosCtor);
    DobbyHook((void*)getRealOffset(KillEffectItemConfConfigAddress), (void*)my_KillEffectItemCtor, (void**)&orig_KillEffectItemCtor);
    DobbyHook((void*)getRealOffset(SPKillIconItemConfConfigAddress), (void*)my_SPKillIconCtor, (void**)&orig_SPKillIconCtor);
    DobbyHook((void*)getRealOffset(ItemResourceAddress), (void*)my_ItemResourceCtor, (void**)&orig_ItemResourceCtor);
    DobbyHook((void*)getRealOffset(CharacterModelAddress), (void*)my_CharacterModelCtor, (void**)&orig_CharacterModelCtor);
    DobbyHook((void*)getRealOffset(RoleConfAddress), (void*)my_RoleConfCtor, (void**)&orig_RoleConfCtor);
    DobbyHook((void*)getRealOffset(RoleSkinAddress), (void*)my_RoleSkinCtor, (void**)&orig_RoleSkinCtor);
    DobbyHook((void*)getRealOffset(RolePackConfAddress), (void*)my_RolePackConfCtor, (void**)&orig_RolePackConfCtor);
    DobbyHook((void*)getRealOffset(BRDeadboxSkinAddress), (void*)my_BRDeadboxSkinCtor, (void**)&orig_BRDeadboxSkinCtor);
    DobbyHook((void*)getRealOffset(BRDropPlaneSkinAddress), (void*)my_BRDropPlaneSkinCtor, (void**)&orig_BRDropPlaneSkinCtor);
    DobbyHook((void*)getRealOffset(VehicleSkinConfAddress), (void*)my_VehicleSkinConfCtor, (void**)&orig_VehicleSkinConfCtor);
    DobbyHook((void*)getRealOffset(VehicleItemConfAddress), (void*)my_VehicleItemConfCtor, (void**)&orig_VehicleItemConfCtor);

    while (!Tools::GetBaseAddress("libRoosterNN.so"))
    {
        sleep(1);
    }
    
    sleep(5);
    
    Tools::Hook((void*)(m_unity + 0xA61AC70), (void*)_GetCurrentWeaponKillEffect, (void**)&orig_GetCurrentWeaponKillEffect);
    Tools::Hook((void*)(m_unity + 0xA61AAA8), (void*)_GetCurrentWeaponFireEffect, (void**)&orig_GetCurrentWeaponFireEffect);
    Tools::Hook((void*)(m_unity + 0xA61AB8C), (void*)_GetCurrentBulletTrackEffect, (void**)&orig_GetCurrentBulletTrackEffect);
    Tools::Hook((void*)(m_unity + 0xA61ADFC), (void*)_GetCurrentWeaponBrocast, (void**)&orig_GetCurrentWeaponBrocast);

    // InventoryUtil (GameEngine) statics — the in-match kill decorations.
    // dump 1089418-1089460: GetWeaponKillEffectGroup 0xC3661B0/0xC37F1D0,
    // GetWeaponKillIcon 0xC366318/0xC37F220, GetWeaponKillEffectLevel
    // 0xC3664F8/0xC37F25C.
    Tools::Hook((void*)(m_unity + 0xC3661B0), (void*)my_GetWeaponKillEffectGroupExtra, (void**)&orig_GetWeaponKillEffectGroupExtra);
    Tools::Hook((void*)(m_unity + 0xC37F1D0), (void*)my_GetWeaponKillEffectGroupDecor, (void**)&orig_GetWeaponKillEffectGroupDecor);
    Tools::Hook((void*)(m_unity + 0xC366318), (void*)my_GetWeaponKillIconExtra, (void**)&orig_GetWeaponKillIconExtra);
    Tools::Hook((void*)(m_unity + 0xC37F220), (void*)my_GetWeaponKillIconDecor, (void**)&orig_GetWeaponKillIconDecor);
    Tools::Hook((void*)(m_unity + 0xC3664F8), (void*)my_GetWeaponKillEffectLevelExtra, (void**)&orig_GetWeaponKillEffectLevelExtra);
    Tools::Hook((void*)(m_unity + 0xC37F25C), (void*)my_GetWeaponKillEffectLevelDecor, (void**)&orig_GetWeaponKillEffectLevelDecor);

    // Local-player kill decoration hooks (WeaponAttrData / Weapon).
    auto hookUnityRva = [&](uintptr_t rva, void* hook, void** orig) -> bool {
        uintptr_t addr = KittyMemory::getAbsoluteAddress("libunity.so", rva);
        if (addr == 0 || !Tools::IsPtrValid((void*)addr))
            return false;
        Tools::Hook((void*)addr, hook, orig);
        return true;
    };
    g_killIconHookInstalled = hookUnityRva(WeaponAttrDataKillIconItemIDAddress,
                                           (void*)my_GetKillIconItemID,
                                           (void**)&orig_GetKillIconItemID);
    g_killEffectItemHookInstalled = hookUnityRva(WeaponAttrDataKillEffectItemIDAddress,
                                                 (void*)my_GetKillEffectItemID,
                                                 (void**)&orig_GetKillEffectItemID);
    g_killEffectGroupHookInstalled = hookUnityRva(WeaponKillEffectGroupOverrideAddress,
                                                  (void*)my_WeaponKillEffectGroupOverride,
                                                  (void**)&orig_WeaponKillEffectGroupOverride);

    // VehicleSkis hooks
    auto hookVehicleRva = [&](uintptr_t rva, void* hook, void** orig) {
        uintptr_t addr = KittyMemory::getAbsoluteAddress("libunity.so", rva);
        if (addr != 0 && Tools::IsPtrValid((void*)addr))
            Tools::Hook((void*)addr, hook, orig);
    };
    hookVehicleRva(VehicleSkisSetupSkinAddress,       (void*)hook_VehicleSkis_SetupSkin,       (void**)&orig_VehicleSkis_SetupSkin);
    hookVehicleRva(VehicleSkisChangeSkinIdAddress,    (void*)hook_VehicleSkis_ChangeSkinId,    (void**)&orig_VehicleSkis_ChangeSkinId);
    hookVehicleRva(VehicleSkisChangeSkinConfAddress,  (void*)hook_VehicleSkis_ChangeSkinConf,  (void**)&orig_VehicleSkis_ChangeSkinConf);
    hookVehicleRva(VehicleSkisBuildVehicleModelAddress,(void*)hook_VehicleSkis_BuildVehicleModel,(void**)&orig_VehicleSkis_BuildVehicleModel);
    hookVehicleRva(VehicleSkisLoadModelCompleteAddress,(void*)hook_VehicleSkis_LoadModelComplete,(void**)&orig_VehicleSkis_LoadModelComplete);

    uintptr_t vehicleSkinHelperIdAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkinHelperChangeVehicleSkinIdAddress);
    if (vehicleSkinHelperIdAddr != 0 && Tools::IsPtrValid((void*)vehicleSkinHelperIdAddr))
        VehicleSkinHelper_ChangeVehicleSkinId = reinterpret_cast<void (*)(void*, uint)>(vehicleSkinHelperIdAddr);

    uintptr_t vehicleSkinHelperConfAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkinHelperChangeVehicleSkinConfAddress);
    if (vehicleSkinHelperConfAddr != 0 && Tools::IsPtrValid((void*)vehicleSkinHelperConfAddr))
        VehicleSkinHelper_ChangeVehicleSkinConf = reinterpret_cast<void (*)(void*, void*)>(vehicleSkinHelperConfAddr);

    uintptr_t skisApplySkinAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkisApplySkinAddress);
    if (skisApplySkinAddr != 0 && Tools::IsPtrValid((void*)skisApplySkinAddr))
        VehicleSkis_ApplySkin = reinterpret_cast<void (*)(void*)>(skisApplySkinAddr);
    
    while (true)
    {
        LoadCharacterSkins();
        LoadWeaponSkins();
        LoadPlaneSkins();
        LoadSnowboardSkins();
        LoadCamos();
        LoadKillIcons();
        g_killDecorationDiag.killEffectWeapons = (int)killEffectByWeapon.size();
        g_killDecorationDiag.killEffectItemIDs = (int)killEffectItemByAssetId.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
}
