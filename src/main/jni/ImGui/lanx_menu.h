#pragma once

// ---------------------------------------------------------------------------
// LANX menu - ported from github.com/eliwoahzja/LANX
// (examples/example_win32_directx11/main.h + main.cpp render loop), wrapped in
// namespace lanx.
//
// Stripped for Android / this project:
//   * DirectX11: textures, D3DX11CreateShaderResourceViewFromMemory,
//     draw_background_blur, the `texture::` namespace and LoadImages()
//   * Win32: GetTickCount (-> LanxNowMs), Discord avatar / WinINet / json.hpp
//   * The notification widget loop (CNotifications - not part of the menu)
// Kept verbatim: the c_tabs sidebar, the c_animated_bg particle background,
// Triangle_background(), the c_welcome_bg text animation, the State struct,
// Style() and the Render() layout (850x596, centered).
// ---------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "lanx_style.h"
#include "lanx_widgets.hpp"
#include "lanx_font.h"

#include <random>
#include <chrono>
#include <cmath>
#include <vector>
#include <string>

namespace lanx
{
    using namespace ImGui;

    // GetTickCount() replacement for the c_welcome_bg animation clock.
    inline unsigned int LanxNowMs()
    {
        static const auto s_start = std::chrono::steady_clock::now();
        return (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - s_start).count();
    }

    // ---- port of LANX main.h: rotation helpers, tabs, particles, welcome ----

    inline int rotation_start_index;

    inline void ImRotateStart()
    {
        rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    }

    inline ImVec2 ImRotationCenter()
    {
        ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

        const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

        return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
    }

    inline void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
    {
        float s = sin(rad), c = cos(rad);
        center = ImRotate(center, s, c) - center;

        auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
    }

    struct s_tab
    {
        const char* header;
        std::vector<const char*> tabs;
    };

    class c_tabs
    {
    private:
        int current_idx;
        int check_id;

        std::vector<s_tab> tab_selection;

        void Header(const char* name)
        {
            ImGui::PushFont(font::regular_m);
            ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), name);
            ImGui::PopFont();
        }

    public:
        c_tabs(std::vector<s_tab> tab_info)
        {
            this->current_idx = 0;
            this->check_id = 0;
            this->tab_selection = tab_info;
        }

        int GetCurrentTab()
        {
            return current_idx;
        }

        bool IsTabActive(int id)
        {
            return bool(this->c_tabs::current_idx == id);
        }

        void DrawTabs()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 14));
            ImGui::SetCursorPos(ImVec2(20.f, 85));
            ImGui::BeginChild("Tabs", ImVec2(160, ImGui::GetContentRegionAvail().y));
            {
                this->check_id = 0;
                for (int n = 0; n < tab_selection.size(); n++)
                {
                    this->Header(tab_selection[n].header);

                    for (int i = 0; i < tab_selection[n].tabs.size(); i++)
                    {
                        Tab(tab_selection[n].tabs[i], &this->c_tabs::current_idx, int(this->check_id));
                        this->check_id++;
                    }
                }

                if (page_is_changing)
                {
                    if (page_offset > 890.f)
                    {
                        page_offset = -900.f;
                        page_is_changing = false;
                        this->c_tabs::current_idx = wanted_idx;
                    }
                }

                page_offset = ImLerp(page_offset, page_is_changing ? 900.f : 0.f, ImGui::GetIO().DeltaTime * 12.f);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
    };

    struct c_animated_bg
    {
    private:
        const int p_cout = 60;

        ImVec2 GenerateRandomPositionInRect(const ImRect& rect)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> x_dist(rect.Min.x, rect.Max.x);
            std::uniform_real_distribution<float> y_dist(rect.Min.y, rect.Max.y);

            float random_x = x_dist(gen);
            float random_y = y_dist(gen);

            return ImVec2(random_x, random_y);
        }

        float GenerateRandomFloat(float min, float max)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(min, max);

            return dist(gen);
        }

        struct p_info
        {
            ImVec2 current_pos;
            ImVec2 target_pos;
            float radius, current_angle, target_angle, speed, alpha, anim_time, timer;
        };

        std::vector<p_info> particle;

        float Distance(const ImVec2& point1, const ImVec2& point2)
        {
            float deltaX = point2.x - point1.x;
            float deltaY = point2.y - point1.y;

            return std::sqrt(deltaX * deltaX + deltaY * deltaY);
        }

        inline ImVec2 vector_angles(const ImVec2& start, const ImVec2& end)
        {
            const ImVec2 delta = end - start;

            return ImVec2(std::atan2(delta.y, delta.x),
                std::atan2(std::sqrt(delta.x * delta.x + delta.y * delta.y),
                    0.0f));
        }

        void rotate_to(int idx, ImVec2 vec)
        {
            const auto local_pos = this->particle[idx].current_pos;

            auto delta_angles = vector_angles(ImGui::GetMousePos(), local_pos);
            delta_angles.x += 0.1f;

            this->particle[idx].current_angle = delta_angles.x;
        }

    public:
        c_animated_bg()
        {
            particle.resize(p_cout);
        }

        void render()
        {
            for (int i = 0; i < p_cout; i++)
            {
                ImGuiWindow* window = ImGui::GetCurrentWindow();

                if (!window->Rect().Contains(particle[i].target_pos))
                {
                    particle[i].target_pos = GenerateRandomPositionInRect(window->Rect());
                    particle[i].speed = 0.6f + GenerateRandomFloat(0.5f, 1.0f);
                    particle[i].current_angle = GenerateRandomFloat(0.5f, 360.f);
                    particle[i].target_angle = GenerateRandomFloat(0.5f, 360.f);
                    particle[i].radius = GenerateRandomFloat(1.5f, 3.5f);
                    particle[i].anim_time = GenerateRandomFloat(1500, 4000);
                    particle[i].timer = 0.f;
                    particle[i].alpha = -3.f;
                }

                particle[i].alpha = ImLerp(particle[i].alpha, 1.f, c::anim::speed / 10);

                particle[i].timer += c::anim::speed;

                if (particle[i].timer < particle[i].anim_time / 2)
                    particle[i].target_angle = 180;

                if (particle[i].timer > particle[i].anim_time / 2)
                    particle[i].target_angle = 0;

                if (particle[i].timer > particle[i].anim_time)
                {
                    particle[i].target_angle = 360;
                    particle[i].timer = 0;
                }

                particle[i].target_pos.x += cosf(particle[i].current_angle) * particle[i].speed;
                particle[i].target_pos.y += sinf(particle[i].current_angle) * particle[i].speed;

                particle[i].current_pos = ImLerp(particle[i].current_pos, particle[i].target_pos, c::anim::speed);

                // LANX used AddShadowCircle for the glow; the rim-draw below
                // gives the same halo with two stacked filled circles.
                window->DrawList->AddCircleFilled(particle[i].current_pos, particle[i].radius + 2.5f, utils::GetColorWithAlpha(c::anim::active, particle[i].alpha / 2), 36);
                window->DrawList->AddCircleFilled(particle[i].current_pos, particle[i].radius, utils::GetColorWithAlpha(c::anim::active, particle[i].alpha), 36);
            }
        }
    };

    inline void Triangle_background(ImVec2 p)
    {
        ImVec2 screen_size = c::bg::size;

        static ImVec2 particle_pos[100];
        static ImVec2 particle_speed[100];
        static float particle_size[100];
        static float particle_transparency[100];

        for (int i = 0; i < 100; ++i)
        {
            if (particle_pos[i].x == 0 && particle_pos[i].y == 0)
            {
                particle_pos[i].x = rand() % (int)screen_size.x;
                particle_pos[i].y = rand() % 20;
                particle_speed[i] = ImVec2(rand() % 205, rand() % 205);
                particle_size[i] = rand() % 3 + 3;
                particle_transparency[i] = static_cast<float>(rand()) / RAND_MAX;
            }

            particle_pos[i] += particle_speed[i] * ImVec2(ImGui::GetIO().DeltaTime, ImGui::GetIO().DeltaTime);

            if (particle_pos[i].x < p.x || particle_pos[i].x > p.x + screen_size.x)
                particle_speed[i].x = -particle_speed[i].x;
            if (particle_pos[i].y < p.y || particle_pos[i].y > p.y + screen_size.y + 50)
                particle_speed[i].y = -particle_speed[i].y;

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCircleFilled(particle_pos[i], particle_size[i], ImColor(1.f, 1.f, 1.f, particle_transparency[i]), 16);
            draw_list->AddShadowCircle(particle_pos[i], particle_size[i], ImColor(1.f, 1.f, 1.f, particle_transparency[i]), 30.f, ImVec2(0, 0));
        }
    }

    // Welcome/text animation - LANX c_welcome_bg, verbatim except the clock
    // (GetTickCount -> LanxNowMs) and the logo sprite (AddImageRounded on a
    // DX11 texture -> an animated filled circle).
    class c_welcome_bg
    {
    private:
        float welcome_progress;
        float cheatname_progress;
        float logo_progress;
        float background_alpha;

        bool is_finish = false;

        const char* top_text;
        const char* bottom_text;

    public:
        c_welcome_bg(const char* welcome_text, const char* cheat_name)
        {
            top_text = welcome_text;
            bottom_text = cheat_name;
            cheatname_progress = 0.f;
            logo_progress = 0.f;
            welcome_progress = 0.f;
            background_alpha = 0.f;
        }

        void update()
        {
            static unsigned int dwTickStart = LanxNowMs();
            if (LanxNowMs() - dwTickStart > 150)
            {
                if (!this->is_finish)
                {
                    this->c_welcome_bg::welcome_progress = ImLerp(this->c_welcome_bg::welcome_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    if (LanxNowMs() - dwTickStart > 1000)
                        this->c_welcome_bg::cheatname_progress = ImLerp(this->c_welcome_bg::cheatname_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    if (LanxNowMs() - dwTickStart > 1200)
                        this->c_welcome_bg::logo_progress = ImLerp(this->c_welcome_bg::logo_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    this->c_welcome_bg::background_alpha = ImLerp(this->c_welcome_bg::background_alpha, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);
                }
                else
                {
                    if (LanxNowMs() - dwTickStart > 3000)
                        this->c_welcome_bg::welcome_progress = ImLerp(this->c_welcome_bg::welcome_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    if (LanxNowMs() - dwTickStart > 3200)
                        this->c_welcome_bg::logo_progress = ImLerp(this->c_welcome_bg::logo_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    if (LanxNowMs() - dwTickStart > 2600)
                        this->c_welcome_bg::cheatname_progress = ImLerp(this->c_welcome_bg::cheatname_progress, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);

                    if (LanxNowMs() - dwTickStart > 3400)
                        this->c_welcome_bg::background_alpha = ImLerp(this->c_welcome_bg::background_alpha, this->is_finish ? 0.f : 1.f, c::anim::speed / 3);
                }
                if (LanxNowMs() - dwTickStart > 2000)
                    this->is_finish = true;
            }

            ImRect window_bb = ImGui::GetCurrentWindow()->Rect();

            ImGui::GetForegroundDrawList()->AddRectFilled(window_bb.Min, window_bb.Max, utils::GetColorWithAlpha(c::window_bg_color, this->c_welcome_bg::background_alpha), ImGui::GetStyle().FrameRounding);

            ImGui::PushFont(font::regular_m);
            ImGui::GetForegroundDrawList()->AddText(ImVec2(utils::center_text(window_bb.Min, window_bb.Max, bottom_text).x, window_bb.GetCenter().y -
                (-20 * this->c_welcome_bg::cheatname_progress)), utils::GetColorWithAlpha(c::anim::active,
                    this->c_welcome_bg::cheatname_progress), bottom_text);
            ImGui::PopFont();

            ImGui::PushFont(font::regular_l);
            ImGui::GetForegroundDrawList()->AddText(ImVec2(utils::center_text(window_bb.Min, window_bb.Max, top_text).x, window_bb.GetCenter().y - (50 * this->c_welcome_bg::welcome_progress)), utils::GetColorWithAlpha(c::label::active, this->c_welcome_bg::welcome_progress), top_text);
            ImGui::PopFont();

            static int current_frame = 0;
            static float frame_offset = 0.f;
            static float static_frame_offset = 0.01111111111f;

            frame_offset = current_frame * static_frame_offset;

            static unsigned int dwTickStart2 = LanxNowMs();
            if (LanxNowMs() - dwTickStart2 > 15)
            {
                if (current_frame + 1 <= 90)
                    current_frame++;
                else
                    current_frame = 0;

                dwTickStart2 = LanxNowMs();
            }

            ImRect image_bb(window_bb.GetCenter() - ImVec2(30, 30), window_bb.GetCenter() + ImVec2(30, 30));

            // LANX drew a DX11 sprite-sheet logo here (AddImageRounded on
            // texture::anim_logo offset by logo_progress); same motion, drawn
            // as an accent circle instead of an image.
            const float logo_y = 105.f * this->c_welcome_bg::logo_progress;
            ImGui::GetForegroundDrawList()->AddCircleFilled(
                ImVec2(image_bb.GetCenter().x, image_bb.GetCenter().y - logo_y),
                30.f,
                utils::GetColorWithAlpha(c::anim::active, this->c_welcome_bg::logo_progress),
                36);
        }

        bool is_finished()
        {
            return is_finish;
        }
    };

    // ---------------------------------------------------------------------------
    // Menu state - LANX main.cpp kept its values in file-scope arrays; this
    // port groups them in State so Main.cpp can own one static instance.
    // ---------------------------------------------------------------------------
    struct State
    {
        bool            checkboxes[100];
        int             slider_int[64];
        int             combo[64];
        int             keybind[100];
        int             keybind_mode[100];
        float           color_edit[10][4];
        float           col[4];
        ImGuiColorEditFlags picker_flags =
            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;
    };

    // ---------------------------------------------------------------------------
    // Style() - LANX main.cpp style block, verbatim.
    // ---------------------------------------------------------------------------
    inline void Style()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.FramePadding = ImVec2(18, 10);
        s.ItemSpacing = ImVec2(4, 10);
        s.FrameRounding = 2.f;
        s.WindowRounding = 20.f;
        s.WindowBorderSize = 0.f;
        s.PopupBorderSize = 0.f;
        s.WindowPadding = ImVec2(20, 20);
        s.ChildBorderSize = 1.f;
        s.Colors[ImGuiCol_Border] = ImVec4(0.f, 0.f, 0.f, 0.f);
        s.Colors[ImGuiCol_Separator] = ImVec4(1.f, 1.f, 1.f, 0.2f);
        s.Colors[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
        s.WindowShadowSize = 0;
        s.PopupRounding = 5.f;
        s.ScrollbarSize = 1;
        s.SeparatorTextPadding = ImVec2(10, 10);
    }

    // ---------------------------------------------------------------------------
    // Render() - the LANX window: 850x596 centered, header block, sidebar tab
    // switcher, nine tab pages (Combat / Visuals / Miscellaneous groups) and
    // the animated particle background.
    // ---------------------------------------------------------------------------
    inline void Render(State& s, bool* open)
    {
        if (open && !*open)
            return;

        const ImGuiStyle& style = ImGui::GetStyle();

        ImGui::SetNextWindowPos(ImVec2(
            (ImGui::GetIO().DisplaySize.x - c::bg::size.x) * 0.5f,
            (ImGui::GetIO().DisplaySize.y - c::bg::size.y) * 0.5f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(c::bg::size, ImGuiCond_Always);

        ImGui::Begin("##LanxMenu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        {
            // Main.cpp wraps this window in a 0,0 WindowPadding push - the LANX
            // layout is written against its native 20,20 padding.
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            const bool pushed_menu_font = (font::menu_default != nullptr);
            if (pushed_menu_font)
                ImGui::PushFont(font::menu_default);

            c::anim::speed = ImGui::GetIO().DeltaTime * 14.f;
            c::second_color = utils::GetDarkColor(c::main_color);

            const ImVec2& pos = ImGui::GetWindowPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Panel background (LANX drew this over a DirectX blur - dropped).
            draw_list->AddRectFilled(pos, pos + c::bg::size,
                utils::GetColorWithAlpha(c::window_bg_color, c::window_bg_color.Value.w * style.Alpha), c::bg::rounding);

            static c_animated_bg p_animated_bg;
            p_animated_bg.render();

            draw_list->AddText(pos + ImVec2(15, c::bg::size.y - 35), c::label::def, "dev build 24.03");

            // top block
            draw_list->AddRectFilled(pos, pos + ImVec2(c::bg::size.x, 70), GetColorU32(c::child::background), c::bg::rounding, ImDrawFlags_RoundCornersTopRight);

            PushFont(font::bold_font);
            draw_list->AddText(utils::center_text(pos, pos + ImVec2(70, 70), ICON_FIRE_FILL) + ImVec2(0, 4.5f), c::main_color, ICON_FIRE_FILL);
            draw_list->AddText(ImVec2(pos.x + 60, utils::center_text(pos, pos + ImVec2(70, 70), "LineFlow").y), c::label::active, "LineFlow");
            PopFont();

            draw_list->AddText(pos + ImVec2(c::bg::size.x - 70 - CalcTextSize("Lyapos").x, 18), c::label::active, "Lyapos");

            PushFont(font::esp_font);
            draw_list->AddText(pos + ImVec2(c::bg::size.x - 70 - CalcTextSize("Administrator").x, 37), c::label::def, "Administrator");
            PopFont();
            // top block

            static c_tabs p_tabs = [] {
                std::vector<s_tab> tabs_info;
                tabs_info.push_back({ "Combat", {"Aim Assistance", "Close Aim", "Weapon Config"} });
                tabs_info.push_back({ "Visuals", {"Players", "Radar", "World"} });
                tabs_info.push_back({ "Miscellaneous ", {"Misc", "Exploits", "Configuration"} });
                return c_tabs(tabs_info);
            }();
            p_tabs.DrawTabs();

            static const ImGuiColorEditFlags picker_flags = s.picker_flags;

            const float half_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            const float full_h = 450;

            if (p_tabs.IsTabActive(0))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Main Aim Config##L", ImVec2(half_w, full_h), true);
                SliderInt("Distance", &s.slider_int[2], 0, 100);
                SliderInt("FOV Size", &s.slider_int[1], 0, 100);
                SliderInt("Smoothing", &s.slider_int[0], 0, 100);
                Checkbox("Prediction", &s.checkboxes[4]);
                Checkbox("Ignore Knocked", &s.checkboxes[3]);
                Checkbox("Visible Check", &s.checkboxes[2]);
                Checkbox("Auto Aim", &s.checkboxes[1]);
                Checkbox("Enable Aimbot", &s.checkboxes[0]);
                ColorEdit4("FOV Color", &s.col[0], picker_flags);
                static const char* aim_combo_list[] = { "Automatic Rifle", "Thompson", "Revolver", "Bow", "Knife" };
                Combo("Aim Key", &s.combo[1], aim_combo_list, IM_ARRAYSIZE(aim_combo_list));
                Combo("Aim Bone", &s.combo[0], aim_combo_list, IM_ARRAYSIZE(aim_combo_list));
                BindBox("Draw FOV", &s.checkboxes[5], &s.keybind[2], &s.keybind_mode[2]);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Backup Aim Config##R", ImVec2(half_w, full_h), true);
                SliderInt("Distance", &s.slider_int[5], 0, 100);
                SliderInt("FOV Size", &s.slider_int[4], 0, 100);
                ColorEdit4("Main color", (float*)&c::main_color, picker_flags);
                SliderInt("Smoothing", &s.slider_int[3], 0, 100);
                Combo("Aim Key", &s.combo[3], aim_combo_list, IM_ARRAYSIZE(aim_combo_list));
                Combo("Aim Bone", &s.combo[2], aim_combo_list, IM_ARRAYSIZE(aim_combo_list));
                Checkbox("Prediction", &s.checkboxes[10]);
                Checkbox("Ignore Knocked", &s.checkboxes[9]);
                Checkbox("Visible Check", &s.checkboxes[8]);
                Checkbox("Auto Aim", &s.checkboxes[7]);
                Checkbox("Enable Aimbot", &s.checkboxes[6]);
                BindBox("Draw FOV", &s.checkboxes[11], &s.keybind[3], &s.keybind_mode[3]);
                EndChild();
            }

            if (p_tabs.IsTabActive(1))
            {
                static const char* esp_combo_list[] = { "Head", "Neck", "Chest", "Pelvis", "Legs" };
                static const char* color_mode_list[] = { "Static", "Gradient", "Rainbow", "Pulse" };
                static const char* team_filter_list[] = { "All", "Enemies", "Teammates", "Neutral" };
                static const char* glow_mode_list[] = { "Basic", "Rainbow", "Pulse", "Blink" };
                static const char* material_list[] = { "Flat", "Wireframe", "Shaded", "Metallic" };
                (void)esp_combo_list;

                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Visuals (General)##L2", ImVec2(half_w, full_h), true);
                Checkbox("Show Player Box", &s.checkboxes[12]);
                Checkbox("Show Health Bar", &s.checkboxes[13]);
                Checkbox("Show Skeleton", &s.checkboxes[14]);
                Checkbox("Show Names", &s.checkboxes[15]);
                Checkbox("Show Distance", &s.checkboxes[16]);
                SliderInt("Box Thickness", &s.slider_int[6], 1, 10);
                SliderInt("Health Bar Width", &s.slider_int[7], 1, 20);
                SliderInt("Name Font Size", &s.slider_int[8], 10, 30);
                Combo("ESP Color Mode", &s.combo[4], color_mode_list, IM_ARRAYSIZE(color_mode_list));
                Combo("Team Filter", &s.combo[5], team_filter_list, IM_ARRAYSIZE(team_filter_list));
                BindBox("Toggle Visuals", &s.checkboxes[17], &s.keybind[4], &s.keybind_mode[4]);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Visuals (Advanced)##R2", ImVec2(half_w, full_h), true);
                Checkbox("Glow Effect", &s.checkboxes[18]);
                Checkbox("Chams", &s.checkboxes[19]);
                Checkbox("Offscreen Arrows", &s.checkboxes[20]);
                Checkbox("Distance Circles", &s.checkboxes[21]);
                Checkbox("Ammo Count", &s.checkboxes[22]);
                SliderInt("Glow Intensity", &s.slider_int[9], 0, 100);
                SliderInt("Chams Opacity", &s.slider_int[10], 0, 100);
                SliderInt("Arrow Size", &s.slider_int[11], 5, 50);
                Combo("Glow Color Mode", &s.combo[6], glow_mode_list, IM_ARRAYSIZE(glow_mode_list));
                Combo("Chams Material", &s.combo[7], material_list, IM_ARRAYSIZE(material_list));
                BindBox("Toggle Advanced ESP", &s.checkboxes[23], &s.keybind[5], &s.keybind_mode[5]);
                EndChild();
            }

            if (p_tabs.IsTabActive(2))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                Child("Weapon Config tab", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    static const char* opt_combo_list[] = { "Option 1", "Option 2", "Option 3" };

                    Checkbox("Enable", &s.checkboxes[20 + 0]);
                    Combo("Aim bone for each weapon category", &s.combo[20], opt_combo_list, IM_ARRAYSIZE(opt_combo_list));
                    Combo("Smoothing each weapon category", &s.combo[21], opt_combo_list, IM_ARRAYSIZE(opt_combo_list));
                    Combo("Fov each weapon category", &s.combo[22], opt_combo_list, IM_ARRAYSIZE(opt_combo_list));
                    Checkbox("Triggerbot", &s.checkboxes[21]);
                    SliderInt("Triggerbot Distance ", &s.slider_int[20], 0, 100);
                    Keybind("Trigger Key", &s.keybind[20], &s.keybind_mode[20]);
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(3))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                Child("ESP Player", ImVec2(ImGui::GetContentRegionAvail().x / 2, 460), true);
                {
                    Checkbox("Enable", &s.checkboxes[30]);
                    Checkbox("2D Box", &s.checkboxes[31]);
                    Checkbox("Cornered Box", &s.checkboxes[32]);
                    Checkbox("Skeleton", &s.checkboxes[33]);
                    Checkbox("Head Circle", &s.checkboxes[34]);
                    Checkbox("Name", &s.checkboxes[35]);
                    Checkbox("Distance", &s.checkboxes[36]);
                    Checkbox("Snaplines", &s.checkboxes[37]);
                    Checkbox("Show Bots", &s.checkboxes[38]);
                    Checkbox("Platform", &s.checkboxes[39]);
                    Checkbox("Rank", &s.checkboxes[40]);
                    Checkbox("Weapon", &s.checkboxes[41]);
                    Checkbox("Team Check", &s.checkboxes[42]);
                    SliderInt("Skeleton Thickness", &s.slider_int[30], 0, 192);
                    ColorEdit4("Visible", s.col, picker_flags);

                    Checkbox("Invisible", &s.checkboxes[43]);
                    Checkbox("Visible Skeleton", &s.checkboxes[44]);
                    Checkbox("Visible Text", &s.checkboxes[46]);
                    Checkbox("Invisible Text", &s.checkboxes[47]);
                }
                EndChild();

                ImGui::SetCursorPos(ImVec2(540, 85));

                Child("ESP Preview", ImVec2(ImGui::GetContentRegionAvail().x - 20, 460), true);
                {
                    // LANX previewed its live ESP through esp.h/m_esp_draw -
                    // that renderer belongs to this app, not the ported menu.
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(4))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85));
                Child("Radar", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    Checkbox("Enable", &s.checkboxes[50]);
                    SliderInt("Size", &s.slider_int[50], 0, 100);
                    SliderInt("Position X", &s.slider_int[51], 0, 100);
                    SliderInt("Position Y", &s.slider_int[52], 0, 100);
                    SliderInt("Reneder Distance", &s.slider_int[53], 0, 100);
                    SliderInt("Enemy Distance", &s.slider_int[54], 0, 100);
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(5))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85));
                Child("World esp", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    Checkbox("Enable", &s.checkboxes[60]);
                    Checkbox("Vehicles", &s.checkboxes[61]);
                    Checkbox("Chests", &s.checkboxes[62]);
                    Checkbox("Ammo Boxes", &s.checkboxes[63]);
                    Checkbox("Floor Loot", &s.checkboxes[64]);
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(6))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85));
                Child("Misc", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    Checkbox("Crosshair", &s.checkboxes[70]);
                    Checkbox("Fps Counter", &s.checkboxes[71]);
                    Checkbox("Stream Proof", &s.checkboxes[72]);
                    Checkbox("Vehicles", &s.checkboxes[73]);
                    Checkbox("Chests", &s.checkboxes[74]);
                    Checkbox("Ammo Boxes", &s.checkboxes[75]);
                    Checkbox("Floor Loot", &s.checkboxes[76]);
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(7))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85));
                Child("Exploits", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    Checkbox("No Recoil", &s.checkboxes[80]);
                    Checkbox("No Bloom", &s.checkboxes[81]);
                    Checkbox("Wireframe", &s.checkboxes[82]);
                    Checkbox("First Person", &s.checkboxes[83]);
                    Keybind("Vehicle Speed Uncap (Hold key)", &s.keybind[84], &s.keybind_mode[84]);
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(8))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85));
                Child("Config", ImVec2(ImGui::GetContentRegionAvail().x - 21, 460), true);
                {
                    Button("Save", ImVec2(ImGui::GetContentRegionAvail().x, 50));
                    Button("Load", ImVec2(ImGui::GetContentRegionAvail().x, 50));
                }
                EndChild();
            }

            if (pushed_menu_font)
                ImGui::PopFont();
            ImGui::PopStyleVar();
        }
        ImGui::End();
    }
}