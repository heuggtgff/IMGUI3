#pragma once

// ---------------------------------------------------------------------------
// LANX theme / palette - ported from github.com/eliwoahzja/LANX
// (imgui_settings.h), wrapped in namespace lanx so nothing collides with this
// project's own c:: / font:: / custom:: / main_runtime_theme namespaces.
//
// Note: LANX spells a few members `default`, which is a C++ keyword. In this
// port they are named `def` (text::label::def, text::description::def,
// anim::def).
// ---------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

#include <string>

namespace lanx
{
    // LANX's ImGui fork carries io.AnimSpeed = time elapsed since the last
    // frame; this project's ImGui has no such field, so the frame delta is the
    // equivalent lerp speed everywhere LANX used GetAnimSpeed().
    inline float GetAnimSpeed() { return ImGui::GetIO().DeltaTime; }

    inline ImVec4 rgba(int r, int g, int b, float a)
    {
        return ImVec4(
            (float)r / 255.0f,
            (float)g / 255.0f,
            (float)b / 255.0f,
            a
        );
    }

    namespace font
    {
        // Poppins 20 + merged icomoon icons - the font lanx::Render draws in.
        inline ImFont* menu_default = nullptr;
        inline ImFont* icomoon_logo = nullptr;
        inline ImFont* esp_font = nullptr;
        inline ImFont* regular_m = nullptr;
        inline ImFont* regular_l = nullptr;
        inline ImFont* icomoon_page = nullptr;
        inline ImFont* inter_semibold = nullptr;
        inline ImFont* bold_font = nullptr;
        inline ImFont* s_inter_semibold = nullptr;
        inline ImFont* inter_medium = nullptr;
        inline ImFont* icon_notify = nullptr;
    }

    namespace utils
    {
        inline ImColor GetColorWithAlpha(ImColor color, float alpha)
        {
            return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
        }

        inline ImVec2 center_text(ImVec2 min, ImVec2 max, const char* text)
        {
            return min + (max - min) / 2 - ImGui::CalcTextSize(text) / 2;
        }

        inline ImColor GetDarkColor(const ImColor& color)
        {
            float r = color.Value.x;
            float g = color.Value.y;
            float b = color.Value.z;

            float darkPercentage = 0.4f;
            float darkR = r * darkPercentage;
            float darkG = g * darkPercentage;
            float darkB = b * darkPercentage;

            return ImColor(darkR, darkG, darkB, 255.0f);
        }

        inline ImVec4 ImColorToImVec4(const ImColor& color)
        {
            return ImVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
        }
    }

    inline namespace c
    {
        inline ImColor main_color = rgba(173, 143, 233, 1);
        inline ImColor second_color = rgba(100, 92, 122, 1);

        inline ImColor background_color = rgba(20, 20, 20, 0.5);
        inline ImColor stroke_color(255, 255, 255, 0);
        inline ImColor window_bg_color = rgba(22, 22, 22, 0.71);

        inline ImVec4 separator = ImColor(22, 23, 26);

        inline namespace anim
        {
            inline float speed;
            inline ImColor active(114, 149, 255, 255);
            inline ImColor def(22, 23, 26, 255);
        }

        inline namespace bg
        {
            inline ImVec4 background = rgba(22, 22, 22, 0.71);
            inline ImVec2 size = ImVec2(850, 596);
            inline float rounding = 15.f;
        }

        inline namespace child
        {
            inline ImVec4 background = rgba(60, 60, 60, 0.25);
            inline ImVec4 stroke = ImColor(18, 18, 24, 0);
            inline float rounding = 8.f;
        }

        namespace page
        {
            inline ImVec4 background_active = ImColor(21, 22, 25);
            inline ImVec4 background = ImColor(16, 17, 18);

            inline ImVec4 text_hov = ImColor(150, 162, 205);
            inline ImVec4 text = ImColor(150, 162, 205);

            inline float rounding = 4.f;
        }

        inline namespace elements
        {
            inline ImVec4 background_hovered = ImColor(21, 22, 25);
            inline ImVec4 background = ImColor(16, 17, 18);
            inline float rounding = 2.5f;
        }

        inline namespace checkbox
        {
            inline ImVec4 mark = ImColor(255, 255, 255, 255);
        }

        // NOTE: LANX declares this namespace `inline`, but then the member
        // variable `text` inside it becomes visible as `c::text` too and every
        // `c::text::...` lookup turns ambiguous. Keeping it non-inline and
        // aliasing label/description into `c` gives the same public names LANX
        // code uses (c::text::*, c::label::*, c::description::*) without the
        // collision.
        namespace text
        {
            inline namespace label
            {
                inline ImColor active = ImColor(255, 255, 255, 255);
                inline ImColor hovered = ImColor(205, 205, 205, 255);
                inline ImColor def = ImColor(150, 150, 150, 220);
            }

            inline namespace description
            {
                inline ImColor active = ImColor(200, 200, 200, 102);
                inline ImColor hovered = ImColor(200, 200, 200, 63);
                inline ImColor def = ImColor(200, 200, 200, 40);
            }

            inline ImVec4 text_active = ImColor(255, 255, 255);
            inline ImVec4 text_hov = ImColor(150, 162, 205);
            inline ImVec4 text = ImColor(150, 162, 205);
        }

        namespace label = text::label;
        namespace description = text::description;
    }

    // Page-switch animation state (LANX imgui_settings.h globals).
    inline float page_offset;
    inline bool page_is_changing;
    inline std::string wanted_category;
    inline int wanted_idx;
}