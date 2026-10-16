#pragma once
#include "../ImGui/imgui.h"
#include "Keyboard.h"
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cmath>
#include <functional>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cfloat>
#include <cstring>
#include "../ImGui/imgui_settings.h"
#include "../ImGui/custom_widgets.hpp"

#include "../Skin/Fields.h"
#include "../Skin/Thread.h"
#include "../Skin/Camo.h"

extern float menu[4];
extern char searchQuery[256];
extern bool showKeyboard;
extern ImFont* F50;
extern ImFont* JAAT;
namespace font {
    extern ImFont* inter_semibold;
}

const int MIN_SEARCH_LENGTH = 3;
static int skinSubTab = 0;
static float cursorBlinkTimer = 0.0f;
static bool cursorVisible = true;
static std::string activeInputID = "";

std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

struct checkbox_state
{
    ImVec4 background, circle, text, box;
    float slow_circle;
};

enum class SkinTier {
    None,
    Mythic,
    Legendary,
    Epic,
    Common,
    Uncommon,
    Rare
};

// Skin/Thread.h GetRarityPrefix tags: 5=[M], 4=[L], 3=[E], 2=[R], 1=[U], else [C]
// (the game's quality colors: red/gold/purple/blue/green/white).
static SkinTier DetectSkinTier(const std::string& rawName) {
    if (rawName.find("[M]") != std::string::npos || rawName.find("[MYTHIC]") != std::string::npos) return SkinTier::Mythic;
    if (rawName.find("[L]") != std::string::npos || rawName.find("[LEGENDARY]") != std::string::npos) return SkinTier::Legendary;
    if (rawName.find("[E]") != std::string::npos || rawName.find("[EPIC]") != std::string::npos) return SkinTier::Epic;
    if (rawName.find("[R]") != std::string::npos || rawName.find("[RARE]") != std::string::npos) return SkinTier::Rare;
    if (rawName.find("[U]") != std::string::npos || rawName.find("[UNCOMMON]") != std::string::npos) return SkinTier::Uncommon;
    if (rawName.find("[C]") != std::string::npos || rawName.find("[COMMON]") != std::string::npos) return SkinTier::Common;
    return SkinTier::None;
}

// 0 = show everything; otherwise only entries tagged with that quality.
static std::string RarityFilterTag(int filter) {
    switch (filter) {
        case 1: return "[M]";
        case 2: return "[L]";
        case 3: return "[E]";
        case 4: return "[R]";
        case 5: return "[U]";
        case 6: return "[C]";
        default: return "";
    }
}

static bool RarityFilterPass(const std::string& name, int filter) {
    const std::string tag = RarityFilterTag(filter);
    if (tag.empty()) return true;
    if (name.find(tag) != std::string::npos) return true;
    // Tagless entries (a tier whose ColorID was unknown at load time) show up
    // under the Common filter instead of vanishing.
    return filter == 6 && DetectSkinTier(name) == SkinTier::None;
}

static std::string StripTierTag(const std::string& rawName) {
    std::string out = rawName;
    const char* tags[] = {"[MYTHIC]", "[LEGENDARY]", "[EPIC]", "[COMMON]", "[RARE]", "[UNCOMMON]", "[M]", "[L]", "[E]", "[R]", "[U]", "[C]"};
    for (const char* tag : tags) {
        const std::string t(tag);
        size_t pos = out.find(t);
        if (pos != std::string::npos) {
            out.erase(pos, t.length());
            while (!out.empty() && out.front() == ' ')
                out.erase(out.begin());
            while (!out.empty() && out.back() == ' ')
                out.pop_back();
            break;
        }
    }
    return out;
}

std::string GetBaseWeaponName(const std::string& fullName) {
    size_t dashPos = fullName.find(" - ");
    if (dashPos == std::string::npos) return fullName;
    std::string base = fullName.substr(0, dashPos);
    size_t tagEnd = base.find("] ");
    if (tagEnd != std::string::npos && tagEnd + 2 < base.length())
        base = base.substr(tagEnd + 2);
    return base;
}

namespace ImGui {
bool AstralInput(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0)) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 inputSize = size;
    if (inputSize.x <= 0.0f)
        inputSize.x = GetContentRegionAvail().x;
    if (inputSize.y <= 0.0f)
        inputSize.y = 50.0f;

    ImRect bb(pos, ImVec2(pos.x + inputSize.x, pos.y + inputSize.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    ButtonBehavior(bb, id, &hovered, &held);
    bool clicked = hovered && IsMouseClicked(0);
    bool isActive = (g.ActiveId == id);

    if (clicked) {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        activeInputID = label;
    }

    if (isActive) {
        cursorBlinkTimer += io.DeltaTime;
        if (cursorBlinkTimer >= 1.0f) {
            cursorBlinkTimer = 0.0f;
            cursorVisible = !cursorVisible;
        }
    } else {
        cursorBlinkTimer = 0.0f;
        cursorVisible = true;
    }

    ImU32 bgColor = GetColorU32(isActive ? c::elements::background_hovered : hovered ? c::button::background_hovered : c::widget::background);
    ImU32 borderColor = GetColorU32(isActive ? c::accent : c::button::outline);

    ImDrawList* draw_list = window->DrawList;

    draw_list->AddRectFilled(bb.Min, bb.Max, bgColor, 4.0f);
    draw_list->AddRect(bb.Min, bb.Max, borderColor, 4.0f, 0, 1.0f);

    std::string text = buf;
    ImVec2 textPos = ImVec2(bb.Min.x + 15.0f, bb.Min.y + (inputSize.y - GetFontSize()) * 0.5f);

    PushClipRect(ImVec2(bb.Min.x + 10.0f, bb.Min.y), ImVec2(bb.Max.x - 10.0f, bb.Max.y), true);

    if (text.empty() && !isActive) {
        ImVec4 hintColor = c::text::text_hov;
        hintColor.w *= 0.70f;
        draw_list->AddText(textPos, GetColorU32(hintColor), "Type here...");
    } else {
        draw_list->AddText(textPos, GetColorU32(c::text::text_active), text.c_str());

        if (isActive && cursorVisible) {
            ImVec2 textSize = CalcTextSize(text.c_str());
            float cursorX = textPos.x + textSize.x + 2.0f;
            float cursorY1 = textPos.y;
            float cursorY2 = textPos.y + GetFontSize();
            draw_list->AddLine(ImVec2(cursorX, cursorY1), ImVec2(cursorX, cursorY2),
                               GetColorU32(c::text::text_active), 2.0f);
        }
    }

    PopClipRect();

    if (isActive) {
        float animProgress = fmod(cursorBlinkTimer, 1.0f);
        float lineWidth = bb.GetWidth() * 0.3f;
        float lineStart = bb.Min.x + (bb.GetWidth() - lineWidth) * 0.5f;
        float lineEnd = lineStart + lineWidth;

        float alpha = 1.0f - animProgress;
        if (animProgress > 0.5f) {
            alpha = (1.0f - animProgress) * 2.0f;
        } else {
            alpha = animProgress * 2.0f;
        }

        ImVec4 lineColorF = c::accent;
        lineColorF.w = alpha;
        ImU32 lineColor = GetColorU32(lineColorF);
        draw_list->AddLine(ImVec2(lineStart, bb.Max.y - 2.0f),
                           ImVec2(lineEnd, bb.Max.y - 2.0f),
                           lineColor, 3.0f);
    }

    return false;
}

    bool CheckboxFullWidth(const char* label, bool* v)
    {
        return custom::Checkbox(label, v);
    }
}

static bool DrawTieredSkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    if (!value) return false;

    std::string rawLabel = label;
    const SkinTier tier = DetectSkinTier(rawLabel);
    const std::string cleanLabel = StripTierTag(rawLabel);
    
    ImGui::PushID(label);
    
    const bool pressed = custom::Checkbox("##skin_checkbox", value);
    if (pressed && applySkin) {
        applySkin(); 
    }
    
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 rowMin = ImGui::GetItemRectMin();
    const ImVec2 rowMax = ImGui::GetItemRectMax();
    const float su = c::scale * c::widget_scale;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* labelFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float labelSize = labelFont->FontSize * 0.84f;

    float currentX = rowMin.x + 35.0f * su; 

    if (tier != SkinTier::None) {
        const char* badgeText = "";
        ImU32 badgeBg = IM_COL32(34, 46, 41, 170);
        ImU32 badgeFg = IM_COL32(210, 226, 218, 245);

        if (tier == SkinTier::Mythic) {
            badgeText = "MYTHIC";
            badgeBg = IM_COL32(145, 30, 34, 190);
            badgeFg = IM_COL32(255, 184, 184, 250);
        } else if (tier == SkinTier::Legendary) {
            badgeText = "LEGENDARY";
            badgeBg = IM_COL32(137, 79, 16, 190);
            badgeFg = IM_COL32(255, 217, 151, 250);
        } else if (tier == SkinTier::Epic) {
            badgeText = "EPIC";
            badgeBg = IM_COL32(90, 52, 150, 190);
            badgeFg = IM_COL32(227, 196, 255, 250);
        } else if (tier == SkinTier::Common) {
            badgeText = "COMMON";
            badgeBg = IM_COL32(23, 74, 40, 200);     
            badgeFg = IM_COL32(74, 222, 128, 255);    
        } else if (tier == SkinTier::Rare) {
            badgeText = "RARE";
            badgeBg = IM_COL32(16, 62, 96, 190);
            badgeFg = IM_COL32(147, 197, 253, 250);
        } else if (tier == SkinTier::Uncommon) {
            badgeText = "UNCOMMON";
            badgeBg = IM_COL32(38, 68, 22, 190);
            badgeFg = IM_COL32(163, 230, 53, 250);
        }
        
        const ImVec2 badgeTextSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, badgeText);
        const float badgePadX = 6.0f * su;
        const float badgePadY = 2.0f * su;
        const float badgeH = badgeTextSize.y + (badgePadY * 2.0f);
        const float badgeW = badgeTextSize.x + (badgePadX * 2.0f);

        const ImVec2 badgeMin(currentX, rowMin.y + (rowMax.y - rowMin.y - badgeH) * 0.5f);
        const ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + badgeH);

        dl->AddRectFilled(badgeMin, badgeMax, badgeBg, 4.0f * su);
        dl->AddText(labelFont, labelSize, ImVec2(badgeMin.x + badgePadX, badgeMin.y + badgePadY), badgeFg, badgeText);

        currentX += badgeW + (6.0f * su); 
    }
    
    const ImVec2 nameSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, cleanLabel.c_str());
    const float nameY = rowMin.y + (rowMax.y - rowMin.y - nameSize.y) * 0.5f;
    
    dl->AddText(
        labelFont,
        labelSize,
        ImVec2(currentX, nameY),
        hovered ? IM_COL32(240, 232, 248, 255) : IM_COL32(236, 240, 246, 255),
        cleanLabel.c_str()
    );
    
    ImGui::PopID();
    return pressed;
}

static bool SkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    return DrawTieredSkinCheckbox(label, value, applySkin);
}

static bool WeaponSkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    return DrawTieredSkinCheckbox(label, value, applySkin);
}

inline void SelectSnowboardSkin(int skinId)
{
    void* selectedSkin = FindVehicleSkinConfigById(skinId);
    if (!SkinPtr(selectedSkin))
        return;

    auto* fields = (VehicleSkinConfFields*)((uintptr_t)selectedSkin + 0x10);
    if (!SkinPtr(fields))
        return;

    activeVehicleSkins[31707110] = skinId;
    activeVehicleSkinsById[(int)fields->VehicleId] = skinId;
    activeVehicleSkinConfs[skinId] = selectedSkin;
}

inline void RenderSkinCategoryContent(int categoryIndex, bool drawSubTabs = false) {

    ImGui::BeginChild("SkinContent", ImVec2(0, 0));
    ImGui::Dummy(ImVec2(0, 8 * c::scale));
    
    if (drawSubTabs) {
        const float tabSpacing = 8.0f * c::scale;
        const float tabHeight = 34.0f * c::scale;
        // Pinalitan ang / 5.0f ng / 6.0f para magkasya ang 6 na sub-tabs sa isang linya
        const float tabWidth = ImMax(32.0f, (ImGui::GetContentRegionAvail().x - tabSpacing * 6.0f) / 7.0f);
        ImFont* subTabFont = font::inter_semibold ? font::inter_semibold : F50;
        
        if (subTabFont) ImGui::PushFont(subTabFont);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(tabSpacing, tabSpacing));
        
        if (custom::Page(skinSubTab == 0, "Character", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 0;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 1, "Watch", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 1;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 2, "Deadbox", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 2;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 3, "Plane", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 3;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 4, "Weapon", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 4;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 5, "Camo", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 5;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 6, "Snowboard", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 6;
        if (subTabFont) ImGui::PopFont();
    }

    static char charSearchQuery[256] = "";
    static bool skinSearchWasActive = false;
    static bool charSearchWasActive = false;
    bool charActive = false;
    bool charHovered = false;
    
    ImGui::Indent(10.0f);
    float totalAvail = ImGui::GetContentRegionAvail().x - 20.0f;
    float halfWidth = (totalAvail - 10.0f) * 0.5f;
    float searchWidth = (skinSubTab == 0) ? halfWidth : totalAvail;
    
    // One search field for the tab; on the Camo sub-tab it is the camo search.
    ImGui::TextColored(c::text::text_active, skinSubTab == 5 ? "Search Camo:" : "Search Skin:");
    if (skinSubTab == 0) {
        ImGui::SameLine(halfWidth + 20.0f);
        ImGui::TextColored(c::text::text_active, "Search Custom Char:");
    }
    
    ImGui::AstralInput("##SearchSkin", searchQuery, IM_ARRAYSIZE(searchQuery), ImVec2(searchWidth, 50));
    bool skinClicked = ImGui::IsItemClicked();
    bool skinActive  = ImGui::IsItemActive();
    bool skinHovered = ImGui::IsItemHovered();
    if (skinClicked || (!skinSearchWasActive && skinActive)) {
        activeInputID = "##SearchSkin";
        showKeyboard = true;
    }
    skinSearchWasActive = skinActive;
    
    if (skinSubTab == 0) {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::AstralInput("##SearchCustomChar", charSearchQuery, IM_ARRAYSIZE(charSearchQuery), ImVec2(halfWidth, 50));
        bool charClicked = ImGui::IsItemClicked();
        charActive = ImGui::IsItemActive();
        charHovered = ImGui::IsItemHovered();
        if (charClicked || (!charSearchWasActive && charActive)) {
            activeInputID = "##SearchCustomChar";
            showKeyboard = true;
        }
        charSearchWasActive = charActive;
    }
        
    // (The "tap outside the keyboard closes it" test used to live here. It ran
    // off the search box's own IsItemActive(), so while the box was active - i.e.
    // always, while typing - it never fired, and its 0.60f panel height did not
    // even match the keyboard's 0.56f. RenderVirtualKeyboard owns it now, on the
    // panel's real rect.)

    // ---- Rarity filter (applies to every skin sub-tab; tag-based) -----------
    // Drawn with the menu's own combo widget. The stock ImGui::BeginCombo drew
    // its frame in the default (dark, transparent-over-black) colours this fork
    // themes away, so on device the row read as a bare "Filter:" label with an
    // invisible control next to it. custom::Combo is the same control every other
    // dropdown in this menu uses (Mode / Hitbox / Box Type) and paints its own
    // label, field, arrow and popup.
    // The Camo sub-tab deliberately has no filter dropdown - its list is the
    // whole camo table, narrowed only by the search field.
    static int rarityFilter = 0;               // 0 = All
    if (skinSubTab != 5) {
        static const char* rarityNames[] = {"All", "Mythic", "Legendary", "Epic",
                                            "Rare", "Uncommon", "Common"};
        custom::Combo("Filter", &rarityFilter, rarityNames, IM_ARRAYSIZE(rarityNames),
                      IM_ARRAYSIZE(rarityNames));
    }

    custom::Separator_line();
    ImGui::Unindent(30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::Indent(20.0f);
    std::string searchLower = ToLower(searchQuery);
    int itemCount = 0;
    const char *itemType = "";
    switch (skinSubTab)
    {
    case 0:
        itemCount = charData.size();
        itemType = "Characters";
        break;
    case 1:
        itemCount = watch.size();
        itemType = "Watches";
        break;
    case 2:
        itemCount = deadboxF.size();
        itemType = "Deadboxes";
        break;
    case 3:
        itemCount = dropplane.size();
        itemType = "Planes";
        break;
    case 4:
        itemCount = itemData.size();
        itemType = "Weapons";
        break;
    case 5:
        itemCount = camoData.size();
        itemType = "Camo";
        break;
    case 6:
        itemCount = snowboardData.size();
        itemType = "Snowboards";
        break;
    }
    if (skinSubTab == 0 && !g_targetCharacters.empty()) {
        std::string charSearchLower = ToLower(charSearchQuery);
        std::vector<int> filteredIndices;
        std::vector<const char*> filteredNames;

        for (size_t i = 0; i < g_targetCharacters.size(); i++) {
            if (strlen(charSearchQuery) >= MIN_SEARCH_LENGTH) {
                if (ToLower(g_targetCharacters[i].name).find(charSearchLower) == std::string::npos) continue;
            }
            filteredIndices.push_back(i);
            filteredNames.push_back(g_targetCharacters[i].name.c_str());
        }

        if (!filteredNames.empty()) {
            int currentInFiltered = 0;
            for (size_t i = 0; i < filteredIndices.size(); i++) {
                if (filteredIndices[i] == g_selectedTargetCharIndex) { currentInFiltered = i; break; }
            }
            custom::BeginGroup();
            if (custom::Combo("Target Character:", &currentInFiltered, filteredNames.data(), filteredNames.size())) {
                g_selectedTargetCharIndex = filteredIndices[currentInFiltered];
            }
            custom::EndGroup();
        }
    }

    ImGui::BeginChild("##SkinList", ImVec2(0, ImGui::GetContentRegionAvail().y - 10), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    switch (skinSubTab) {
case 0:
    if (!charData.empty()) {

        for (const auto& getchar : charData) {

            if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {

                if (ToLower(getchar.charName).find(searchLower) == std::string::npos)
                    continue;
            }
            if (!RarityFilterPass(getchar.charName, rarityFilter))
                continue;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

            SkinCheckbox(getchar.charName.c_str(), &sBool[getchar.charName], [&]() {

                for (auto& pair : sBool) {

                    if (pair.first != getchar.charName)
                        pair.second = false;
                }

                // Bounds-checked: g_targetCharacters can be rebuilt between two
                // frames, and an out-of-range index would read past the vector
                // instead of just doing nothing.
                if (g_selectedTargetCharIndex < 0 ||
                    g_selectedTargetCharIndex >= (int)g_targetCharacters.size())
                    return;

                // Only the tiers the apply really reads, and no RolePackConf:
                // the game builds those pack configs lazily, and demanding them
                // here made the click do nothing in a lobby (see
                // CharacterApplyReady in Skin/Fields.h).
                if (!CharacterApplyReady(CharacterModelConfigInstance,
                                         itemResourceConfigInstance,
                                         RoleConfConfigInstance))
                    return;

                int selTraitor1P = 0, selTraitor3P = 0, selItemID = 0,
                    selRoleID = 0, selPackID = 0;
                ResolveTargetIds(g_targetCharacters[g_selectedTargetCharIndex],
                                 selTraitor1P, selTraitor3P,
                                 selItemID, selRoleID, selPackID);

                // The whole apply lives in Skin/Fields.h so the host test
                // (Skin/tests/character_apply_check.cpp) drives the same code
                // this button does. Only the SELECTED target's rows are written,
                // and only the non-zero values of this character.
                ApplyCharacterSkin(getchar,
                                   selTraitor1P, selTraitor3P,
                                   selItemID, selRoleID, selPackID,
                                   CharacterModelConfigInstance,
                                   itemResourceConfigInstance,
                                   RoleConfConfigInstance,
                                   RolePackConfConfigInstance);
            });

            ImGui::PopStyleVar();
        }
    }
    break;

case 1:
    for (const auto& w : watch) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(w.watchname).find(searchLower) == std::string::npos)
            continue;
        if (!RarityFilterPass(w.watchname, rarityFilter))
            continue;

        if (w.watchname.find("Watch") == std::string::npos)
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(w.watchname.c_str(), &sBool[w.watchname], [&]() {

            // Same target resolution the character tab uses, and the same
            // "selected target only" rule: the old `|| cf->Traitor1P == 710001101`
            // wrote Charly's model config whatever the dropdown showed.
            int sT1P = 0, sT3P = 0, sItem = 0, sRole = 0, sPack = 0;
            if (g_selectedTargetCharIndex >= 0 &&
                g_selectedTargetCharIndex < (int)g_targetCharacters.size())
                ResolveTargetIds(g_targetCharacters[g_selectedTargetCharIndex],
                                 sT1P, sT3P, sItem, sRole, sPack);

            for (auto m : CharacterModelConfigInstance) {

                if (!m)
                    continue;

                auto* cf = (CharacterModelFields*)((uintptr_t)m + 0x10);

                if (cf->Traitor1P == sT1P || cf->Traitor3P == sT3P) {

                    cf->BindEffect1P = w.watchvalue;
                }
            }
        });

        ImGui::PopStyleVar();
    }
    break;

case 2:
    for (const auto& d : deadboxF) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(d.deadname).find(searchLower) == std::string::npos)
            continue;
        if (!RarityFilterPass(d.deadname, rarityFilter))
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(d.deadname.c_str(), &sBool[d.deadname], [&]() {

            for (auto db : BRDeadboxSkinConfigInstance) {

                auto* df = (BRDeadboxSkinFields*)((uintptr_t)db + 0x10);

                if (df->ID == 180300004) {

                    df->ColorID            = d.dead[0];
                    df->DeadBoxEffectAsset = d.dead[1];
                    df->Flag               = d.dead[2];
                    df->FlagAsset          = d.dead[3];
                    df->ModelAsset3P       = d.dead[4];
                    df->ModelAssetUI       = d.dead[5];
                }
            }
        });

        ImGui::PopStyleVar();
    }
    break;

case 3:
    for (const auto& p : dropplane) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(p.planename).find(searchLower) == std::string::npos)
            continue;
        if (!RarityFilterPass(p.planename, rarityFilter))
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(p.planename.c_str(), &sBool[p.planename], [&]() {

            for (auto pl : BRDropPlaneSkinConfigInstance) {

                auto* dpf = (BRDropPlaneSkinFields*)((uintptr_t)pl + 0x10);

                if (dpf->ID == 0) {

                    dpf->ColorID          = p.plane[0];
                    dpf->ModelAsset1P     = p.plane[1];
                    dpf->ModelAsset3P     = p.plane[2];
                    dpf->ModelAssetCutScene = p.plane[3];
                    dpf->ModelAssetUI     = p.plane[4];
                    dpf->Priority         = p.plane[5];
                }
            }
        });

        ImGui::PopStyleVar();
    }
    break;

    case 4:

        if (!itemData.empty())
        {
            auto applyWeaponSkin = [&](const auto& getitem) {
                int baseID = getitem.WeaponConf[0];
                int extraID = getitem.WeaponExtra[0];
                int skinID = getitem.WeaponConf[2];
                int itemID = getitem.Item2Inventory[0];
                int lootID = getitem.Item2Inventory[3];
                for (auto item : itemInventoryInstance)
                {
                    if (!item)
                        continue;
                    item2Fields = (Item2InventoryFields *)((uintptr_t)item + 0x20);
                    if (item2Fields->ItemID == getitem.Item2Inventory[0] || item2Fields->ItemID == getitem.Item2Inventory[3])
                    {
                        if (getitem.Item2Inventory[1] != 0)
                            item2Fields->WeaponAssetGroupID = getitem.Item2Inventory[1];
                        if (getitem.Item2Inventory[2] != 0)
                            item2Fields->WeaponIconID = getitem.Item2Inventory[2];
                    }
                }
                for (auto conf : weaponConfInstance)
                {
                    if (!conf)
                        continue;
                    weaponconfFields = (WeaponConfFields *)((uintptr_t)conf + 0x20);
                    if (baseID == weaponconfFields->ID ||
                            extraID == weaponconfFields->ID ||
                            skinID == weaponconfFields->ID ||
                            itemID == weaponconfFields->ID ||
                            lootID == weaponconfFields->ID)
                    {
                        weaponconfFields->DefWeaponSkinID = getitem.WeaponConf[2];
                        weaponconfFields->DefaultKillBrocast = getitem.WeaponConf[3];
                        weaponconfFields->ExternalUnVisible = true;

                        for (auto skinConf : weaponConfInstance)
                        {
                            if (!skinConf)
                                continue;
                            WeaponConfFields *skinFields = (WeaponConfFields *)((uintptr_t)skinConf + 0x20);
                            if (skinFields->ID == getitem.WeaponConf[2])
                            {
                                // Only copy a name that really exists — a null
                                // string would blank the weapon's name/desc.
                                if (Tools::IsPtrValid(skinFields->LOCID_Name))
                                    weaponconfFields->LOCID_Name = skinFields->LOCID_Name;
                                break;
                            }
                        }
                        if (baseID == weaponconfFields->ID)
                        {
                            weaponconfFields->ColorID = getitem.WeaponConf[1];
                        }
                    }
                }
                if (getitem.WeaponExtra[4] > 0)
                {
                    const int effectAssetID = getitem.WeaponExtra[4];
                    auto itEffectItem = killEffectItemByAssetId.find(effectAssetID);
                    const int effectItemID = itEffectItem != killEffectItemByAssetId.end()
                                             ? itEffectItem->second : 0;
                    const int effectKeys[5] = {baseID, extraID, skinID, itemID, lootID};
                    for (int key : effectKeys)
                    {
                        activeKillEffects[key] = effectAssetID;
                        // The client-side per-weapon record (WeaponAttrData)
                        // resolves the effect from the ITEM id, so hand it that
                        // too when LoadWeaponSkins found one.
                        if (effectItemID > 0)
                            activeKillEffectItems[key] = effectItemID;
                    }
                }
                if (DetectSkinTier(getitem.itemName) == SkinTier::Mythic)
                {
                    // Mythic skin applied → behave like mythic level 5 and
                    // surface the mythic kill icon through InventoryUtil.
                    // The InventoryUtil hooks gate on val > 0, so weapons
                    // without a matching SPKillIcon entry stay untouched.
                    const int mythicKeys[5] = {baseID, extraID, skinID, itemID, lootID};
                    for (int key : mythicKeys)
                    {
                        auto itIcon = killIconByWeapon.find(key);
                        if (itIcon != killIconByWeapon.end() && itIcon->second > 0)
                            activeKillIcons[key] = itIcon->second;
                    }
                }
                for (auto extra : weaponExtraInstance)
                {
                    if (!extra)
                        continue;
                    weaponextraFields = (WeaponConfExtraFields *)((uintptr_t)extra + 0x10);
                    if (weaponextraFields->ID == baseID ||
                            weaponextraFields->ID == extraID ||
                            weaponextraFields->ID == skinID ||
                            weaponextraFields->ID == itemID ||
                            weaponextraFields->ID == lootID)
                    {
                        if (getitem.itemName.find("[MYTHIC]") != std::string::npos)
                        {
                            if (getitem.WeaponExtra[1] != 0)
                                weaponextraFields->DefaultMythicArmor = getitem.WeaponExtra[1];
                            if (getitem.itemName.find("[MYTHIC] AK117 - Memento Mori") != std::string::npos &&
                                    getitem.WeaponExtra[2] != 0)
                            {
                                weaponextraFields->DefaultMythicSig = getitem.WeaponExtra[2];
                            }
                        }
                        // Guarded: writing a 0 here is what silently deleted the
                        // weapon's own kill effect / death replay effect.
                        if (getitem.WeaponExtra[3] != 0)
                            weaponextraFields->DefaultDeadReplayEffectId = getitem.WeaponExtra[3];
                        if (getitem.WeaponExtra[4] != 0)
                            weaponextraFields->DefaultKillEffectId = getitem.WeaponExtra[4];
                    }
                }
                for (auto asset : weaponAssetGroupInstance)
                {
                    if (!asset)
                        continue;
                    weaponAssetFields = (WeaponAssetGroupFields *)((uintptr_t)asset + 0x40);
                    if (weaponAssetFields->Id == getitem.WeaponAsset[0])
                    {
                        if (getitem.WeaponAsset[1] / 10 == getitem.WeaponAsset[2] / 10 && getitem.WeaponAsset[1] > 0)
                        {
                            weaponAssetFields->FireEffectGroupID = getitem.WeaponAsset[1];
                        }
                    }
                }
                for (auto itemResource : itemResourceConfigInstance)
                {
                    if (!itemResource)
                        continue;
                    itemFields = (ItemResourceFields *)((uintptr_t)itemResource + 0x10);
                    if (!Tools::IsPtrValid(itemFields))
                        continue;
                    const int resourceID = itemFields->ID;
                    if (resourceID != skinID && resourceID != itemID && resourceID != lootID &&
                            resourceID != extraID && resourceID != baseID)
                        continue;
                    // One guarded block per id (the six copies above repeated the
                    // same writes and one of them missed UISpriteNameIndex). Every
                    // write is skipped when the value is 0: ItemRes/ItemResInt are
                    // only non-zero when LoadWeaponSkins matched the item's own
                    // ItemResourceConfig, and writing the zero of an unmatched
                    // resource is what blanked the weapon card in the loadout.
                    if (getitem.ItemResInt[0] != 0)
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                    if (getitem.ItemResInt[1] != 0)
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                    if (getitem.ItemResInt[2] != 0)
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                    if (getitem.ItemRes[0] != 0)
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                    if (getitem.ItemRes[1] != 0)
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                    if (getitem.ItemRes[2] != 0)
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                    if (getitem.ItemRes[3] != 0)
                        itemFields->UISquareSpriteNameIndex = getitem.ItemRes[3];
                }

                int fireEffectID = 0;
                for (auto asset : weaponAssetGroupInstance)
                {
                    if (!asset)
                        continue;
                    weaponAssetFields = (WeaponAssetGroupFields *)((uintptr_t)asset + 0x40);
                    if (weaponAssetFields && weaponAssetFields->Id == getitem.WeaponAsset[0])
                    {
                        fireEffectID = weaponAssetFields->FireEffectGroupID;
                        break;
                    }
                }

                if (fireEffectID > 0)
                {
                    for (auto fireConf : weaponFireEffectInstance)
                    {
                        if (!fireConf)
                            continue;
                        WeaponFireEffectFields *wfFields = (WeaponFireEffectFields *)((uintptr_t)fireConf + 0x10);
                        if (!Tools::IsPtrValid(wfFields))
                            continue;

                        // WeaponFireEffectConfigConfig (dump 824278): Id @0x80,
                        // LevelEffectDelayTimeUI @0x84, ..._King @0x88 — the
                        // struct's offset comments were 4 bytes out and wrote
                        // 0x88/0x8C, i.e. 4 bytes past the end of the pair.
                        const int fireConfID = *(int *)((uintptr_t)fireConf + 0x80);
                        if (fireConfID == fireEffectID)
                        {
                            *(float *)((uintptr_t)fireConf + 0x84) = 0.1f;
                            *(float *)((uintptr_t)fireConf + 0x88) = 0.1f;
                            auto *killCountArray = *(Array<int> **)((uintptr_t)fireConf + 0x118);
                            if (killCountArray && Tools::IsPtrValid(killCountArray))
                            {
                                for (int i = 0; i < killCountArray->getLength(); ++i)
                                {
                                    killCountArray->m_Items[i] = 0;
                                }
                            }
                            auto *killCountArrayKing = *(Array<int> **)((uintptr_t)fireConf + 0x120);
                            if (killCountArrayKing && Tools::IsPtrValid(killCountArrayKing))
                            {
                                for (int i = 0; i < killCountArrayKing->getLength(); ++i)
                                {
                                    killCountArrayKing->m_Items[i] = 0;
                                }
                            }
                            break;
                        }
                    }
                }
            };

            for (const auto& getitem : itemData)
            {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {
                    std::string nameLower = ToLower(getitem.itemName);
                    if (nameLower.find(searchLower) == std::string::npos)
                        continue;
                }
                if (!RarityFilterPass(getitem.itemName, rarityFilter))
                    continue;
                if (sBool[getitem.itemName])
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    WeaponSkinCheckbox(getitem.itemName.c_str(), &sBool[getitem.itemName],
                                       [&, getitem]()
                    {
                        std::string currentBase = GetBaseWeaponName(getitem.itemName);
                        for (const auto& other : itemData)
                        {
                            if (other.itemName != getitem.itemName &&
                                    GetBaseWeaponName(other.itemName) == currentBase)
                            {
                                sBool[other.itemName] = false;
                            }
                        }
                        applyWeaponSkin(getitem);
                    });
                    ImGui::PopStyleVar();
                }
            }

            for (const auto& getitem : itemData)
            {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {
                    std::string nameLower = ToLower(getitem.itemName);
                    if (nameLower.find(searchLower) == std::string::npos)
                        continue;
                }
                if (!RarityFilterPass(getitem.itemName, rarityFilter))
                    continue;
                if (!sBool[getitem.itemName])
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    WeaponSkinCheckbox(getitem.itemName.c_str(), &sBool[getitem.itemName],
                                       [&, getitem]()
                    {
                        std::string currentBase = GetBaseWeaponName(getitem.itemName);
                        for (const auto& other : itemData)
                        {
                            if (other.itemName != getitem.itemName &&
                                    GetBaseWeaponName(other.itemName) == currentBase)
                            {
                                sBool[other.itemName] = false;
                            }
                        }
                        applyWeaponSkin(getitem);
                    });
                    ImGui::PopStyleVar();
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No weapon skins loaded");
        }
        break;

    case 5:
    {
        // --- Camo: the whole camo table, applied to every gun ---
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Camo - replaces the equipped camo on every gun");
        ImGui::Dummy(ImVec2(0, 10));

        // Skin / kill-decoration diagnostics: each count is a stage of the
        // pipeline, so a zero names the stage that failed without another build.
        // KillIcon cfg/map = SPKillIcon configs captured / weapons mapped;
        // KillFx = weapons mapped to an effect asset / effect items identified;
        // Active = what the menu has switched on; Hooks = live getter traffic
        // (AttrData = WeaponAttrData, Group = Weapon.get_KillEffectGroupIDOverride).
        {
            const ImVec4 dim(0.65f, 0.65f, 0.72f, 1);
            ImGui::TextColored(dim, "KillIcon cfg=%d map=%d active=%d",
                g_killDecorationDiag.killIconConfigs,
                g_killDecorationDiag.killIconWeapons,
                (int)activeKillIcons.size());
            ImGui::TextColored(dim, "KillFx map=%d items=%d active=%d/%d",
                g_killDecorationDiag.killEffectWeapons,
                g_killDecorationDiag.killEffectItemIDs,
                (int)activeKillEffects.size(), (int)activeKillEffectItems.size());
            ImGui::TextColored(dim, "Hooks attr=%d/%d item=%d/%d grp=%d/%d",
                g_killDecorationDiag.iconHookHits, g_killDecorationDiag.iconHookCalls,
                g_killDecorationDiag.effectItemHookHits, g_killDecorationDiag.effectItemHookCalls,
                g_killDecorationDiag.effectGroupHookHits, g_killDecorationDiag.effectGroupHookCalls);
            ImGui::TextColored(dim, "HookInstalled %s%s%s key=%d res=%d",
                g_killIconHookInstalled ? "icon " : "-icon ",
                g_killEffectItemHookInstalled ? "item " : "-item ",
                g_killEffectGroupHookInstalled ? "grp" : "-grp",
                g_killDecorationDiag.lastAttrKey, (int)itemData.size());
            // InventoryInfo writes: the record the kill feed reads, filled from
            // the local player's held weapon (Weapon : Inventory, info @0x98).
            // Non-zero here means the mythic kill icon reached the HUD even if
            // the getter hooks were never called at all (attr=0/0).
            ImGui::TextColored(dim, "InventoryInfo icon=%d lvl=%d",
                g_killDecorationDiag.inventoryKillIconWrites,
                g_killDecorationDiag.inventoryLevelWrites);
        }
        ImGui::Dummy(ImVec2(0, 6));

        // --- Every camo in the build, one search field, no filter dropdown ---
        // The list is camoData, filled by LoadCamos() (Skin/Thread.h) from the
        // game's own WeaponCamosConfig table - one row per camo the build has,
        // de-duplicated there (Skin/Fields.h InsertCamo): several table rows for
        // one camo (one per weapon) are one entry here, not the duplicate list
        // rows this used to show.
        static std::unordered_map<uint, int> camoOriginals;
        static std::unordered_map<uint, WeaponSkinCamoOriginal> camoSkinOriginals;
        static int selectedCamoItemID = -1;   // -1 = no override, the tab's OFF

        // The ctor hooks push confs from another thread, so take a copy: the
        // apply below then walks a list that cannot move under it.
        std::vector<void *> camoConfs;
        std::vector<void *> camoSkinConfs;
        {
            std::lock_guard<std::mutex> lock(g_dataMutex);
            camoConfs = weaponConfInstance;
            camoSkinConfs = weaponSkinConfigInstance;
        }

        // One pick has to be written in TWO places to reach every gun:
        //   * WeaponConfConfig.DefWeaponSkinID - the default skin of a gun that has
        //     no camo chosen (ApplyCamoToEveryGun);
        //   * WeaponSkinConfig.CamoID - the weapon-skin record the equipped camo of
        //     a gun is read from (ApplyCamoToEveryWeaponSkin). Writing only the
        //     first lever left every gun that already had a camo on its own camo -
        //     the "it only changes the guns without camo" report.
        auto applyCamoEverywhere = [&](int camoItemID) {
            ApplyCamoToEveryGun(camoItemID, camoConfs, camoOriginals);
            ApplyCamoToEveryWeaponSkin(camoItemID, camoSkinConfs, camoSkinOriginals);
        };

        bool camoDefault = selectedCamoItemID < 0;
        if (custom::Checkbox("Default / OFF", &camoDefault)) {
            if (camoDefault) {
                selectedCamoItemID = -1;
                applyCamoEverywhere(0);
            }
        }

        // Diagnostic for the picked camo. `id` is the game's own CamoItemID and
        // `apply` the value actually written: the proven constant for the seven
        // camos this build already equips correctly, otherwise the row's own id.
        // Seeing both is what makes a wrong id obvious on device.
        for (const auto &camo : camoData) {
            if (camo.camoItemID != selectedCamoItemID) continue;
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
                "Selected: %s  id=%08X apply=%08X%s  confs=%d skins=%d",
                camo.camoName.c_str(), (unsigned)camo.camoItemID, (unsigned)camo.applyID,
                camo.applyID == camo.camoItemID ? "" : " (proven)",
                (int)camoConfs.size(), (int)camoSkinConfs.size());
            break;
        }

        ImGui::Separator();

        if (camoData.empty()) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1),
                "No camos loaded yet - open the game's camo menu once");
        } else {
            int shown = 0;
            for (const auto &camo : camoData) {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
                    CamoNameLower(camo.camoName).find(searchLower) == std::string::npos)
                    continue;
                shown++;
                // Names can repeat across rows, so the camo id keys the widget.
                ImGui::PushID((int)camo.camoItemID);
                bool checked = (selectedCamoItemID == camo.camoItemID);
                if (DrawTieredSkinCheckbox(camo.camoName.c_str(), &checked)) {
                    if (checked) {
                        selectedCamoItemID = camo.camoItemID;
                        applyCamoEverywhere(camo.applyID);
                    } else {
                        selectedCamoItemID = -1;
                        applyCamoEverywhere(0);
                    }
                }
                ImGui::PopID();
            }
            if (shown == 0)
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No camo matches the search");
        }
    break;
    }

    case 6:
        if (!snowboardData.empty()) {
            // Selected (ON) first
            for (const auto& sb : snowboardData) {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
                    ToLower(sb.skinName).find(searchLower) == std::string::npos)
                    continue;
                if (!RarityFilterPass(sb.skinName, rarityFilter))
                    continue;
                if (sBool[sb.skinName]) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    bool wasOn = sBool[sb.skinName];
                    DrawTieredSkinCheckbox(sb.skinName.c_str(), &sBool[sb.skinName], [&, sb]() {
                        SelectSnowboardSkin(sb.skinId);
                        for (const auto& other : snowboardData) {
                            if (other.skinName != sb.skinName)
                                sBool[other.skinName] = false;
                        }
                    });
                    if (wasOn && !sBool[sb.skinName]) {
                        activeVehicleSkins.erase(31707110);
                        activeVehicleSkinsById.clear();
                        activeVehicleSkinConfs.erase(sb.skinId);
                    }
                    ImGui::PopStyleVar();
                }
            }
            // Unselected (OFF) below
            for (const auto& sb : snowboardData) {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
                    ToLower(sb.skinName).find(searchLower) == std::string::npos)
                    continue;
                if (!RarityFilterPass(sb.skinName, rarityFilter))
                    continue;
                if (!sBool[sb.skinName]) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    DrawTieredSkinCheckbox(sb.skinName.c_str(), &sBool[sb.skinName], [&, sb]() {
                        SelectSnowboardSkin(sb.skinId);
                        for (const auto& other : snowboardData) {
                            if (other.skinName != sb.skinName)
                                sBool[other.skinName] = false;
                        }
                    });
                    ImGui::PopStyleVar();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No snowboard skins loaded");
        }
        break;
    }
    if (showKeyboard) {
        if (activeInputID == "##SearchCustomChar") {
            RenderVirtualKeyboard("##VirtualKeyboard", charSearchQuery, IM_ARRAYSIZE(charSearchQuery), &showKeyboard);
        } else {
            RenderVirtualKeyboard("##VirtualKeyboard", searchQuery, IM_ARRAYSIZE(searchQuery), &showKeyboard);
        }
    }
}

inline void RenderTab4Content() {
    RenderSkinCategoryContent(skinSubTab, true);
}
