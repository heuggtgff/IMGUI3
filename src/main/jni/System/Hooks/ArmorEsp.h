#pragma once

#include <cstdint>

struct ArmorEspData
{
    int value;
    int maxValue;
};

inline bool GetArmorEspData(Pawn* pawn, ArmorEspData& data)
{
    if (!Tools::IsPtrValid(pawn))
        return false;

    PlayerInfo* info = *(PlayerInfo**)((uintptr_t)pawn + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid(info))
        return false;

    data.value = info->m_VestValue();
    data.maxValue = info->m_VestMaxValue();
    return data.value > 0 && data.maxValue > 0;
}

inline void DrawArmorEspBar(ImDrawList* draw, float x, float y, float h, const ArmorEspData& data)
{
    if (data.maxValue <= 0 || data.value <= 0)
        return;

    const float ratio = ImClamp(data.value / (float)data.maxValue, 0.0f, 1.0f);
    constexpr float barW = 4.0f;
    constexpr ImU32 armorColor = IM_COL32(70, 155, 255, 255);

    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 120));
    draw->AddRect(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
    draw->AddRectFilled(ImVec2(x, y + h * (1.0f - ratio)), ImVec2(x + barW, y + h), armorColor);
}

inline void DrawArmorEsp(ImDrawList* draw, Pawn* pawn, const Rect& rect)
{
    if (!Config.ESPMenu.Armor)
        return;

    ArmorEspData data{};
    if (!GetArmorEspData(pawn, data))
        return;

    const bool hasSideHealth = Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthSide;
    const float x = rect.x - (hasSideHealth ? 14.0f : 8.0f);
    DrawArmorEspBar(draw, x, rect.y, rect.height, data);
}
