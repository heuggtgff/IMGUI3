#pragma once

#include <string>
#include <unordered_map>

struct CamoConfigSnapshot {
    uint8_t CamoType;
    uint8_t ColorID;
    int TextureAssetID;
    int TextureSpecialAssetID;
};

struct CamoSkinSnapshot {
    int CamoID;
    int ColorSubID;
};

static std::unordered_map<uint, CamoConfigSnapshot> g_CamoOriginals;
static std::unordered_map<uint, CamoSkinSnapshot> g_CamoSkinOriginals;
static bool g_CamoApplied = false;
static int g_CamoAppliedMode = 0;

static int GetCamoTextureAssetID(int mode)
{
    switch (mode) {
        case 1: return 600026482;
        case 2: return 600026184;
        case 3: return 600025935;
        case 4: return 600025647;
        case 5: return 600024796;
        case 6: return 600023492;
        case 7: return 600021143;
        case 8: return 600023027;
        case 9: return 600021144;
        case 10: return 600021142;
        default: return 0;
    }
}

static const char *GetCamoName(int mode)
{
    switch (mode) {
        case 1: return "golden emerald";
        case 2: return "blistering magma";
        case 3: return "polychromatic";
        case 4: return "glacial ripple";
        case 5: return "red sprite";
        case 6: return "aether crystal";
        case 7: return "platinum";
        case 8: return "diamond";
        case 9: return "damascus";
        case 10: return "gold";
        default: return "";
    }
}

static void RestoreCamo()
{
    if (!g_CamoApplied) {
        return;
    }

    for (void *camo : weaponCamosConfigInstance) {
        if (!camo || !Tools::IsPtrValid(camo)) continue;
        auto *fields = reinterpret_cast<WeaponCamosFields *>((uintptr_t)camo + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;

        auto original = g_CamoOriginals.find(fields->CamoItemID);
        if (original == g_CamoOriginals.end()) continue;

        fields->CamoType = original->second.CamoType;
        fields->ColorID = original->second.ColorID;
        fields->TextureAssetID = original->second.TextureAssetID;
        fields->TextureSpecialAssetID = original->second.TextureSpecialAssetID;
    }

    for (void *skin : weaponSkinConfigInstance) {
        if (!skin || !Tools::IsPtrValid(skin)) continue;
        auto *fields = reinterpret_cast<WeaponSkinFields *>((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;

        auto original = g_CamoSkinOriginals.find(fields->ID);
        if (original == g_CamoSkinOriginals.end()) continue;

        fields->CamoID = original->second.CamoID;
        fields->ColorSubID = original->second.ColorSubID;
    }

    g_CamoOriginals.clear();
    g_CamoSkinOriginals.clear();
    g_CamoApplied = false;
    g_CamoAppliedMode = 0;
}

static void ApplyCamo()
{
    if (!Config.ExtraMenu.CamoTest || Config.ExtraMenu.CamoTestMode <= 0) {
        RestoreCamo();
        return;
    }

    if (g_CamoApplied && g_CamoAppliedMode != Config.ExtraMenu.CamoTestMode) {
        RestoreCamo();
    }

    if (g_CamoApplied || weaponCamosConfigInstance.empty()) {
        return;
    }

    const int targetAssetID = GetCamoTextureAssetID(Config.ExtraMenu.CamoTestMode);
    const std::string targetName = GetCamoName(Config.ExtraMenu.CamoTestMode);
    if (targetAssetID == 0 || targetName.empty()) {
        return;
    }

    WeaponCamosFields source{};
    bool foundSource = false;

    for (void *camo : weaponCamosConfigInstance) {
        if (!camo || !Tools::IsPtrValid(camo)) continue;
        auto *fields = reinterpret_cast<WeaponCamosFields *>((uintptr_t)camo + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;

        const std::string nameLower = ToLower(GetNameString(UnlockedCamo, camo));
        if (nameLower.find(targetName) != std::string::npos || fields->TextureAssetID == targetAssetID) {
            source = *fields;
            foundSource = true;
            break;
        }
    }

    if (!foundSource) {
        return;
    }

    for (void *camo : weaponCamosConfigInstance) {
        if (!camo || !Tools::IsPtrValid(camo)) continue;
        auto *fields = reinterpret_cast<WeaponCamosFields *>((uintptr_t)camo + 0x10);
        if (!Tools::IsPtrValid(fields) || fields->CamoItemID == source.CamoItemID) continue;

        const std::string nameLower = ToLower(GetNameString(UnlockedCamo, camo));
        const bool completionist =
            nameLower.find("diamond") != std::string::npos ||
            nameLower.find("gold") != std::string::npos ||
            nameLower.find("platinum") != std::string::npos ||
            nameLower.find("damascus") != std::string::npos;
        if (completionist) continue;

        g_CamoOriginals.emplace(fields->CamoItemID, CamoConfigSnapshot{
            fields->CamoType,
            fields->ColorID,
            fields->TextureAssetID,
            fields->TextureSpecialAssetID
        });

        fields->CamoType = source.CamoType;
        fields->ColorID = source.ColorID;
        fields->TextureAssetID = source.TextureAssetID;
        fields->TextureSpecialAssetID = source.TextureSpecialAssetID;
    }

    for (void *skin : weaponSkinConfigInstance) {
        if (!skin || !Tools::IsPtrValid(skin)) continue;
        auto *fields = reinterpret_cast<WeaponSkinFields *>((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(fields) || fields->ID == source.CamoItemID) continue;

        g_CamoSkinOriginals.emplace(fields->ID, CamoSkinSnapshot{
            fields->CamoID,
            fields->ColorSubID
        });

        fields->CamoID = static_cast<int>(source.CamoItemID);
        fields->ColorSubID = 0;
    }

    g_CamoApplied = !g_CamoOriginals.empty() || !g_CamoSkinOriginals.empty();
    if (g_CamoApplied) {
        g_CamoAppliedMode = Config.ExtraMenu.CamoTestMode;
    }
}

static void UpdateCamoOverride()
{
    ApplyCamo();
}
