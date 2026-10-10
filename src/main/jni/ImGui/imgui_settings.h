#ifndef IMGUI_SETTINGS_H
#define IMGUI_SETTINGS_H

#include "imgui.h"
#include "../System/Texture/box_shadow.h"

extern float menu[4];
extern ImFont* F50;
extern ImFont* F107;

namespace font
{
    extern ImFont* inter_semibold;
}

namespace c
{
    inline float scale = 1.5f;
    inline float widget_scale = 1.0f;
    // Accent: warm amber
    inline ImVec4 accent = ImColor(210, 134, 10);
    inline ImVec4 separator = ImColor(38, 42, 46);

    namespace bg
    {
        // Deep warm slate
        inline ImVec4 background = ImColor(15, 20, 24);
        inline ImVec2 size = ImVec2(450, 370);
        inline float rounding = 6.f;
    }

    namespace child
    {
        inline ImVec4 background = ImColor(22, 29, 34);
        inline ImVec4 cap = ImColor(30, 38, 44);
        inline float rounding = 6.f;
        inline float padding = 13.f;
        inline float spacing = 13.f;
    }

    namespace page
    {
        inline ImVec4 background_active = ImColor(44, 54, 62);
        inline ImVec4 background = ImColor(28, 36, 42);

        inline ImVec4 text_hov = ImColor(185, 162, 110);
        inline ImVec4 text = ImColor(110, 100, 80);

        inline float rounding = 4.f;
    }

    namespace elements
    {
        inline ImVec4 background_hovered = ImColor(40, 50, 58);
        inline ImVec4 background = ImColor(28, 36, 42);
        inline float rounding = 4.f;
    }

    namespace checkbox
    {
        inline ImVec4 mark = ImColor(15, 20, 24);
        inline ImVec4 background_on = ImColor(210, 134, 10);
        inline ImVec4 background_off = ImColor(44, 54, 62);
        inline ImVec4 circle_inactive = ImColor(90, 100, 110);
        inline float rounding = 4.f;
    }

    namespace text
    {
        inline ImVec4 text_active = ImColor(228, 213, 185);
        inline ImVec4 text_hov = ImColor(185, 162, 110);
        inline ImVec4 text = ImColor(112, 102, 82);
    }

    namespace widget
    {
        inline ImVec2 size = ImVec2(0, 34.f);
        inline ImVec4 background = ImColor(28, 36, 42);
        inline ImVec4 outlinecolor = ImColor(50, 60, 68);
        inline float rounding = 4.f;
        inline float outline = 1.f;
    }

    namespace button
    {
        inline ImVec4 background = ImColor(28, 36, 42);
        inline ImVec4 background_hovered = ImColor(40, 50, 58);
        inline ImVec4 background_active = ImColor(52, 64, 72);
        inline ImVec4 outline = ImColor(60, 72, 82);
        inline float rounding = 4.f;
    }

    namespace scrollbar
    {
        inline float hitbox_area = 24.f;
        inline float hitbox_extra = 24.f;
        inline bool left_side = false;
        inline float gutter_spacing = 4.f;
    }

    inline void ApplyMainWindowStyle(ImGuiStyle& style)
    {
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.ItemSpacing = ImVec2(10.0f * scale, 10.0f * scale);
        style.WindowBorderSize = 0.0f;
        style.ScrollbarSize = 8.0f * scale;
    }

    inline float MainTopAreaHeight()
    {
        return 40.0f * scale;
    }

    inline void UpdateTheme(bool dark_mode, const float* accent_rgba, float dt)
    {
        bg::background = ImLerp(bg::background, dark_mode ? ImColor(15, 15, 15) : ImColor(255, 255, 255), dt * 12.0f);
        separator = ImLerp(separator, dark_mode ? ImColor(22, 23, 26) : ImColor(222, 228, 244), dt * 12.0f);

        const ImVec4 accent_target = dark_mode
            ? (accent_rgba ? ImVec4(accent_rgba[0], accent_rgba[1], accent_rgba[2], 1.0f) : ImColor(118, 187, 117).Value)
            : ImColor(121, 131, 207).Value;
        accent = ImLerp(accent, accent_target, dt * 12.0f);

        elements::background_hovered = ImLerp(elements::background_hovered, dark_mode ? ImColor(31, 33, 38) : ImColor(197, 207, 232), dt * 25.0f);
        elements::background = ImLerp(elements::background, dark_mode ? ImColor(22, 23, 25) : ImColor(222, 228, 244), dt * 25.0f);

        widget::background = ImLerp(widget::background, dark_mode ? ImColor(22, 23, 25) : ImColor(236, 240, 250), dt * 25.0f);
        widget::outlinecolor = ImLerp(widget::outlinecolor, dark_mode ? ImColor(30, 32, 36) : ImColor(194, 204, 228), dt * 25.0f);
        button::background = ImLerp(button::background, dark_mode ? ImColor(22, 23, 25) : ImColor(236, 240, 250), dt * 25.0f);
        button::background_hovered = ImLerp(button::background_hovered, dark_mode ? ImColor(31, 33, 38) : ImColor(213, 222, 242), dt * 25.0f);
        button::background_active = ImLerp(button::background_active, dark_mode ? ImColor(40, 42, 48) : ImColor(196, 206, 232), dt * 25.0f);
        button::outline = ImLerp(button::outline, dark_mode ? ImColor(50, 52, 56) : ImColor(177, 188, 217), dt * 25.0f);

        checkbox::mark = ImLerp(checkbox::mark, dark_mode ? ImColor(0, 0, 0) : ImColor(255, 255, 255), dt * 12.0f);
        checkbox::background_off = ImLerp(checkbox::background_off, dark_mode ? ImColor(40, 42, 48) : ImColor(205, 214, 236), dt * 25.0f);
        checkbox::circle_inactive = ImLerp(checkbox::circle_inactive, dark_mode ? ImColor(80, 84, 96) : ImColor(120, 130, 158), dt * 25.0f);

        child::background = ImLerp(child::background, dark_mode ? ImColor(17, 17, 18) : ImColor(241, 243, 249), dt * 12.0f);
        child::cap = ImLerp(child::cap, dark_mode ? ImColor(20, 21, 23) : ImColor(228, 235, 248), dt * 12.0f);
        child::padding = 13.0f;
        child::spacing = 13.0f;

        page::text_hov = ImLerp(page::text_hov, dark_mode ? ImColor(68, 71, 85) : ImColor(136, 145, 176), dt * 12.0f);
        page::text = ImLerp(page::text, dark_mode ? ImColor(68, 71, 85) : ImColor(136, 145, 176), dt * 12.0f);
        page::background_active = ImLerp(page::background_active, dark_mode ? ImColor(31, 33, 38) : ImColor(196, 205, 228), dt * 25.0f);
        page::background = ImLerp(page::background, dark_mode ? ImColor(22, 23, 25) : ImColor(222, 228, 244), dt * 25.0f);

        text::text_active = ImLerp(text::text_active, dark_mode ? ImColor(255, 255, 255) : ImColor(0, 0, 0), dt * 12.0f);
        text::text_hov = ImLerp(text::text_hov, dark_mode ? ImColor(68, 71, 85) : ImColor(68, 71, 81), dt * 12.0f);
        text::text = ImLerp(text::text, dark_mode ? ImColor(68, 71, 85) : ImColor(68, 71, 81), dt * 12.0f);
    }

    inline void ApplyTheme()
    {
        // Warm amber accent
        accent = ImColor(210, 134, 10);
        separator = ImColor(50, 60, 68);

        bg::background = ImColor(15, 20, 24, 230);
        child::background = ImColor(22, 29, 34, 220);
        child::cap = ImColor(30, 38, 44, 200);
        child::padding = 13.0f;
        child::spacing = 13.0f;

        page::background_active = ImColor(52, 64, 72, 255);
        page::background = ImColor(28, 36, 42, 200);
        page::text_hov = ImColor(228, 213, 185);
        page::text = ImColor(140, 120, 80);

        elements::background_hovered = ImColor(44, 54, 62, 210);
        elements::background = ImColor(28, 36, 42, 180);

        checkbox::mark = ImColor(15, 20, 24);
        checkbox::background_on = ImColor(210, 134, 10);
        checkbox::background_off = ImColor(44, 54, 62);
        checkbox::circle_inactive = ImColor(90, 100, 110);

        text::text_active = ImColor(228, 213, 185);
        text::text_hov = ImColor(185, 162, 110);
        text::text = ImColor(112, 102, 82);

        widget::background = ImColor(22, 29, 34, 200);
        widget::outlinecolor = ImColor(58, 70, 80, 180);

        button::background = ImColor(22, 29, 34, 200);
        button::background_hovered = ImColor(38, 48, 56, 220);
        button::background_active = ImColor(52, 64, 72, 240);
        button::outline = ImColor(70, 84, 94, 180);

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScrollbarSize = 5.0f;
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.10f, 0.12f, 0.72f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.40f, 0.10f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.70f, 0.52f, 0.14f, 0.90f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.82f, 0.60f, 0.18f, 1.0f);
    }

    inline void DrawWindowShadow(const ImVec2& menuSize)
    {
        RectangleShadowSettings shadowSettings;
        shadowSettings.rectPos = ImVec2(0.0f, 0.0f);
        shadowSettings.rectSize = menuSize;
        shadowSettings.sigma = 14.0f;
        shadowSettings.padding = ImVec2(0.0f, 0.0f);
        shadowSettings.rings = 5;
        shadowSettings.spacingBetweenRings = 2;
        shadowSettings.samplesPerCornerSide = 2;
        // Warm dark shadow
        shadowSettings.shadowColor = ImGui::ColorConvertU32ToFloat4(IM_COL32(8, 12, 16, 200));
        shadowSettings.shadowSize = ImVec2(0.0f, 0.0f);
        drawRectangleShadowVerticesAdaptive(shadowSettings);
    }

    inline void DrawMenuBackdrop(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding, ImTextureID backgroundTexture = nullptr)
    {
        if (drawList == nullptr) {
            return;
        }

        if (backgroundTexture != nullptr) {
            drawList->AddImageRounded(
                backgroundTexture,
                min,
                max,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f),
                IM_COL32(255, 255, 255, 255),
                rounding
            );
        } else {
            // Warm slate background
            drawList->AddRectFilled(min, max, IM_COL32(15, 20, 24, 255), rounding);
        }

        // Subtle warm tint overlay
        drawList->AddRectFilled(min, max, IM_COL32(20, 14, 8, 40), rounding);
        // Warm amber border
        drawList->AddRect(min, max, IM_COL32(210, 134, 10, 90), rounding, 0, 1.0f);
    }
}

namespace main_runtime_theme
{
    inline float g_menuHue = 0.76f;  // violet default for the runtime UI

    // ============================================================
    // THEME SYSTEM
    // ============================================================
    enum ThemeID {
        THEME_DARK_SLATE  = 0,  // default — deep slate + amber
        THEME_MIDNIGHT    = 1,  // near-black + cool blue
        THEME_FOREST      = 2,  // dark green tones
        THEME_BLOOD       = 3,  // crimson dark
        THEME_LIGHT       = 4,  // light warm grey
        THEME_COUNT       = 5
    };
    inline int g_activeTheme = THEME_DARK_SLATE;

    struct ThemePalette {
        const char* name;
        // Backgrounds
        ImU32 bgWindow;       // main menu bg
        ImU32 bgChild;        // child panel
        ImU32 bgCap;          // header/cap
        ImU32 bgContent;      // content host
        ImU32 bgTabBar;       // tab bar
        ImU32 bgStatusBar;    // bottom bar
        // Text
        ImU32 textActive;
        ImU32 textMuted;
        // Borders
        ImU32 border;
        // Accent hue (HSV hue 0-1)
        float accentHue;
        float accentSat;
        float accentVal;
    };

    inline ThemePalette g_themes[THEME_COUNT] = {
        // DARK SLATE (default)
        {
            "SLATE",
            IM_COL32(15, 20, 24, 230),   // bgWindow
            IM_COL32(22, 29, 34, 220),   // bgChild
            IM_COL32(22, 29, 34, 230),   // bgCap
            IM_COL32(18, 24, 29, 190),   // bgContent
            IM_COL32(18, 24, 29, 215),   // bgTabBar
            IM_COL32(18, 24, 29, 210),   // bgStatusBar
            IM_COL32(228, 213, 185, 255),// textActive
            IM_COL32(112, 102, 82, 190), // textMuted
            IM_COL32(50, 62, 72, 180),   // border
            0.10f, 0.90f, 0.92f,         // amber accent
        },
        // MIDNIGHT
        {
            "NIGHT",
            IM_COL32(6, 8, 18, 235),
            IM_COL32(10, 14, 28, 220),
            IM_COL32(12, 16, 32, 230),
            IM_COL32(8, 12, 22, 200),
            IM_COL32(8, 12, 24, 215),
            IM_COL32(8, 12, 22, 210),
            IM_COL32(200, 215, 245, 255),
            IM_COL32(80, 95, 130, 190),
            IM_COL32(30, 42, 80, 180),
            0.60f, 0.80f, 1.0f,          // electric blue accent
        },
        // FOREST
        {
            "FOREST",
            IM_COL32(8, 18, 12, 235),
            IM_COL32(10, 24, 16, 220),
            IM_COL32(12, 26, 18, 230),
            IM_COL32(8, 20, 13, 200),
            IM_COL32(8, 20, 14, 215),
            IM_COL32(8, 20, 13, 210),
            IM_COL32(190, 235, 200, 255),
            IM_COL32(70, 110, 80, 190),
            IM_COL32(25, 60, 35, 180),
            0.38f, 0.85f, 0.82f,         // green accent
        },
        // BLOOD
        {
            "BLOOD",
            IM_COL32(18, 6, 6, 235),
            IM_COL32(26, 10, 10, 220),
            IM_COL32(28, 12, 12, 230),
            IM_COL32(20, 8, 8, 200),
            IM_COL32(20, 8, 8, 215),
            IM_COL32(20, 8, 8, 210),
            IM_COL32(245, 210, 205, 255),
            IM_COL32(130, 70, 70, 190),
            IM_COL32(80, 22, 22, 180),
            0.01f, 0.90f, 0.90f,         // crimson accent
        },
        // LIGHT
        {
            "LIGHT",
            IM_COL32(225, 220, 212, 230),
            IM_COL32(210, 205, 196, 220),
            IM_COL32(215, 210, 200, 230),
            IM_COL32(220, 214, 206, 200),
            IM_COL32(218, 212, 204, 215),
            IM_COL32(218, 212, 204, 210),
            IM_COL32(40, 32, 22, 255),
            IM_COL32(120, 108, 88, 190),
            IM_COL32(160, 148, 128, 180),
            0.10f, 0.85f, 0.75f,         // warm amber (darker for light bg)
        },
    };

    inline void ApplyThemePreset(int themeId)
    {
        if (themeId < 0 || themeId >= THEME_COUNT) return;
        g_activeTheme = themeId;
        const ThemePalette& t = g_themes[themeId];

        // Apply accent hue
        g_menuHue = t.accentHue;
        ImGui::ColorConvertHSVtoRGB(t.accentHue, t.accentSat, t.accentVal,
            menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
        c::accent = ImGui::ColorConvertU32ToFloat4(
            IM_COL32((int)(menu[0]*255),(int)(menu[1]*255),(int)(menu[2]*255),255));

        // Apply backgrounds
        c::bg::background   = ImGui::ColorConvertU32ToFloat4(t.bgWindow);
        c::child::background= ImGui::ColorConvertU32ToFloat4(t.bgChild);
        c::child::cap       = ImGui::ColorConvertU32ToFloat4(t.bgCap);

        // Apply text
        c::text::text_active= ImGui::ColorConvertU32ToFloat4(t.textActive);
        c::text::text       = ImGui::ColorConvertU32ToFloat4(t.textMuted);

        // Separator
        c::separator        = ImGui::ColorConvertU32ToFloat4(t.border);
    }
    // ============================================================

    inline ImVec4 GetAccentVec4(float alpha = 1.0f)
    {
        return ImVec4(menu[0], menu[1], menu[2], alpha);
    }

    inline ImU32 GetAccentU32(float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentVec4(alpha));
    }

    inline ImVec4 GetAccentTint(float strength, float alpha = 1.0f)
    {
        return ImVec4(menu[0] * strength, menu[1] * strength, menu[2] * strength, alpha);
    }

    inline ImU32 GetAccentTintU32(float strength, float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentTint(strength, alpha));
    }

    inline void ApplyAccentFromHue()
    {
        // Default to warm amber hue (~0.10 in HSV)
        ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.90f, 0.92f, menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
    }

    inline float GetContentPadding()
    {
        return 10.0f;
    }

    inline float GetColumnGap()
    {
        return 10.0f;
    }

    inline float GetChildPadding()
    {
        return 10.0f;
    }

    inline ImVec4 GetSidebarShellBackgroundColor()
    {
        return ImColor(0, 0, 0, 130);
    }

    inline ImVec4 GetActiveTabBackgroundColor()
    {
        return ImColor(0, 0, 0, 150);
    }

    inline void ApplyThemeState()
    {
        c::scale = 1.15f;
        c::widget_scale = 1.45f;
        const float childPadding = GetChildPadding();
        c::accent = ImColor(GetAccentVec4());
        c::separator = ImColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Read from active theme palette for base background colors
        const ThemePalette& tp = g_themes[g_activeTheme];
        const ImVec4 bgW  = ImGui::ColorConvertU32ToFloat4(tp.bgWindow);
        const ImVec4 bgCh = ImGui::ColorConvertU32ToFloat4(tp.bgChild);
        const ImVec4 bgCo = ImGui::ColorConvertU32ToFloat4(tp.bgContent);
        const ImVec4 tAct = ImGui::ColorConvertU32ToFloat4(tp.textActive);
        const ImVec4 tMut = ImGui::ColorConvertU32ToFloat4(tp.textMuted);

        c::bg::background = ImColor(bgW.x, bgW.y, bgW.z, 0.50f);
        c::child::background = GetActiveTabBackgroundColor();
        c::child::cap = ImColor(bgCh.x, bgCh.y, bgCh.z, 0.88f);
        c::child::padding = childPadding / c::scale;
        c::child::spacing = childPadding / c::scale;

        c::page::background_active = ImColor(GetAccentTint(0.28f, 0.55f));
        c::page::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.60f);
        c::page::text_hov = ImColor(tAct.x, tAct.y, tAct.z, 1.0f);
        c::page::text = ImColor(tMut.x, tMut.y, tMut.z, 0.96f);

        // Elements tinted from theme bg
        c::elements::background_hovered = ImColor(
            ImClamp(bgCh.x + 0.04f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.04f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.04f, 0.f, 1.f), 0.86f);
        c::elements::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.70f);

        c::checkbox::mark = ImColor(bgW.x, bgW.y, bgW.z, 1.0f);
        c::checkbox::background_on = ImColor(GetAccentTint(0.92f, 0.96f));
        c::checkbox::background_off = ImColor(
            ImClamp(bgCh.x + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.06f, 0.f, 1.f), 0.90f);
        c::checkbox::circle_inactive = ImColor(
            ImClamp(tMut.x * 0.75f, 0.f, 1.f),
            ImClamp(tMut.y * 0.75f, 0.f, 1.f),
            ImClamp(tMut.z * 0.75f, 0.f, 1.f), 0.90f);

        c::text::text_active = ImColor(tAct.x, tAct.y, tAct.z, 1.0f);
        c::text::text_hov = ImColor(GetAccentTint(0.90f, 0.92f));
        c::text::text = ImColor(tMut.x, tMut.y, tMut.z, 0.95f);

        c::widget::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.76f);
        c::widget::outlinecolor = ImColor(
            ImClamp(bgCh.x + 0.08f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.08f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.08f, 0.f, 1.f), 0.68f);

        c::button::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.80f);
        c::button::background_hovered = ImColor(
            ImClamp(bgCh.x + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.06f, 0.f, 1.f), 0.88f);
        c::button::background_active = ImColor(
            ImClamp(bgCh.x + 0.10f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.10f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.10f, 0.f, 1.f), 0.92f);
        c::button::outline = ImColor(GetAccentTint(0.55f, 0.48f));

        // Scrollbar tinted from accent
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScrollbarSize = 5.0f;
        const ImVec4& ac = c::accent;
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(bgW.x, bgW.y, bgW.z, 0.72f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(ac.x*0.60f, ac.y*0.60f, ac.z*0.60f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(ac.x*0.80f, ac.y*0.80f, ac.z*0.80f, 0.90f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(ac.x,       ac.y,       ac.z,       1.0f);
    }
}

#endif // IMGUI_SETTINGS_H
