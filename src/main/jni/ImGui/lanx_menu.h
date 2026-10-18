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
#include <cstdio>
#include <cstring>

// Feature state owned by Main.cpp (definitions near the top of that file).
extern bool  HIDEESP;
extern bool  UnlimitedFly;
extern float SnowBsize;
extern float SlideRange;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;

namespace lanx
{
    using namespace ImGui;

    // Page-switch travel (pixels) for the tab animation - see DrawTabs().
    inline constexpr float kPageSlide = 160.f;

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
                    if (page_offset > kPageSlide - 10.f)
                    {
                        page_offset = -kPageSlide;
                        page_is_changing = false;
                        this->c_tabs::current_idx = wanted_idx;
                    }
                }

                // Short, fast slide instead of LANX's 900px / 12Hz sweep: the
                // page is readable almost immediately after a tab tap instead
                // of sliding in from off-panel for a third of a second.
                page_offset = ImLerp(page_offset, page_is_changing ? kPageSlide : 0.f,
                    ImMin(1.f, ImGui::GetIO().DeltaTime * 26.f));
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
    };

    struct c_animated_bg
    {
    private:
        const int p_cout = 60;

        // ONE engine for the whole particle field. LANX's original built a
        // fresh std::random_device (on Android that is an /dev/urandom open)
        // plus a new mt19937 for every single call - 60 particles x several
        // calls per frame - which is what made opening the menu and switching
        // pages crawl. The engine is seeded once.
        static std::mt19937& RandomEngine()
        {
            static std::mt19937 engine = [] {
                std::random_device seed;
                return std::mt19937(seed());
            }();
            return engine;
        }

        ImVec2 GenerateRandomPositionInRect(const ImRect& rect)
        {
            std::uniform_real_distribution<float> x_dist(rect.Min.x, rect.Max.x);
            std::uniform_real_distribution<float> y_dist(rect.Min.y, rect.Max.y);

            float random_x = x_dist(RandomEngine());
            float random_y = y_dist(RandomEngine());

            return ImVec2(random_x, random_y);
        }

        float GenerateRandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);

            return dist(RandomEngine());
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
        // Live ESP preview renderer, supplied by Main.cpp (LANX previewed its
        // own esp.h draw - this project passes in DrawEspPreview).
        void          (*draw_esp_preview)(ImDrawList*, ImVec2, float, float) = nullptr;
        // Set by the header's minimize control; Main.cpp turns it into the
        // existing thin restore bar (`windowCollapsed`) and clears the flag.
        bool            minimize_requested = false;
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
    // ESP colour helpers for the Players page.
    //
    // runtime_preview_menu::DrawRuntimeEspColorRow is the app's own colour row
    // (a custom::ColorEdit4 over a float[4]); it writes back 0..255 while the
    // ESP itself stores 0..1 (CREATE_COLOR divides by 255), so the value has
    // to be normalised after an edit. The line colour is then fanned out to
    // the team's box / name / health / distance colours, exactly like the
    // app's own ESP settings helper does.
    // ---------------------------------------------------------------------------
    inline void NormalizeEspColor(float* col)
    {
        if (col == nullptr)
            return;
        for (int i = 0; i < 4; ++i)
            if (col[i] > 1.0f)
                col[i] /= 255.0f;
    }

    inline void ApplyEspColorGroup(float* line, float* box, float* name, float* health, float* distance)
    {
        if (line == nullptr)
            return;
        NormalizeEspColor(line);
        if (box)      std::memcpy(box, line, sizeof(float) * 4);
        if (name)     std::memcpy(name, line, sizeof(float) * 4);
        if (health)   std::memcpy(health, line, sizeof(float) * 4);
        if (distance) std::memcpy(distance, line, sizeof(float) * 4);
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

            draw_list->AddText(pos + ImVec2(c::bg::size.x - 106 - CalcTextSize("Lyapos").x, 18), c::label::active, "Lyapos");

            PushFont(font::esp_font);
            draw_list->AddText(pos + ImVec2(c::bg::size.x - 106 - CalcTextSize("Administrator").x, 37), c::label::def, "Administrator");
            PopFont();

            // Minimize: collapses the panel to the thin restore bar Main.cpp
            // already draws (tap the bar to bring the menu back).
            {
                const ImVec2 mn_pos = pos + ImVec2(c::bg::size.x - 46.f, 17.f);
                ImGui::SetCursorScreenPos(mn_pos);
                ImGui::InvisibleButton("##lanx_minimize", ImVec2(24.f, 24.f));
                const bool mn_hot = ImGui::IsItemHovered();
                draw_list->AddRectFilled(mn_pos, mn_pos + ImVec2(24.f, 24.f),
                    mn_hot ? utils::GetColorWithAlpha(c::main_color, 0.95f)
                           : utils::GetColorWithAlpha(c::main_color, 0.14f), 6.f);
                const ImU32 dash_col = mn_hot
                    ? (ImU32)IM_COL32(18, 18, 22, 255)
                    : (ImU32)utils::GetColorWithAlpha(c::label::def, 1.0f);
                draw_list->AddRectFilled(mn_pos + ImVec2(7.f, 11.f), mn_pos + ImVec2(17.f, 13.f),
                    dash_col, 1.f);
                if (ImGui::IsItemClicked())
                    s.minimize_requested = true;
            }
            // top block

            static c_tabs p_tabs = [] {
                std::vector<s_tab> tabs_info;
                // The labels name what the page actually holds - the pages are
                // Aim, Triggerbot, Weapon mods / player ESP, awareness ESP,
                // world + camera, performance, memory patches, skins, config.
                tabs_info.push_back({ "Combat", {"Aim Assist", "Triggerbot", "Weapons"} });
                tabs_info.push_back({ "Visuals", {"Players", "Awareness", "World"} });
                tabs_info.push_back({ "Miscellaneous ", {"Performance", "Patches", "Skins", "Configuration"} });
                return c_tabs(tabs_info);
            }();
            p_tabs.DrawTabs();

            static const ImGuiColorEditFlags picker_flags = s.picker_flags;

            // Page geometry. The sidebar occupies the first 180px of the 850px
            // panel, so the columns start at a 200px cursor inset and have to
            // share what is left - sizing them from the whole content width
            // pushed the right-hand column under the panel edge and clipped
            // it. Height keeps the 40px card cap inside the 596px panel.
            const float col_gap = ImGui::GetStyle().ItemSpacing.x * 3.0f;
            const float col_w = (c::bg::size.x - 40.0f - 200.0f - col_gap) * 0.5f;
            const float full_h = 430;

            if (p_tabs.IsTabActive(0))
            {
                static const char* targets[]   = { "Head", "Chest", "Body" };
                static const char* triggers[]  = { "None", "Shooting", "Scoping" };
                static const char* target_by[] = { "Distance", "FOV" };

                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Aim Assist##L0", ImVec2(col_w, full_h), true);
                Checkbox("Aim Assist 360", &Config.Aim.Aimbot360);
                Checkbox("Silent Aim (Bullet Track)", &Config.Aim.AimSilent);
                SliderFloat("Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
                SliderFloat("FOV Radius", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");
                Checkbox("Show FOV Circle", &Config.ESPMenu.ShowFov);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Targeting##R0", ImVec2(col_w, full_h), true);
                Combo("Aim Trigger", (int*)&Config.Aim.Trigger, triggers, IM_ARRAYSIZE(triggers));
                Combo("Hit Part", (int*)&Config.Aim.Target, targets, IM_ARRAYSIZE(targets));
                Combo("Target By", (int*)&Config.Aim.By, target_by, IM_ARRAYSIZE(target_by));
                Separator_line();
                Checkbox("Hide Stream (screenshot-safe)", &HIDEESP);
                Separator_line();
                ColorEdit4("Menu Color", (float*)&c::main_color, picker_flags);
                EndChild();
            }

            if (p_tabs.IsTabActive(1))
            {
                static const char* trigger_modes[] = { "Always", "ADS Only", "Firing Only" };

                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Triggerbot##L1", ImVec2(col_w, full_h), true);
                Checkbox("Triggerbot", &Config.NewFeaturesMenu.Triggerbot);
                if (Config.NewFeaturesMenu.Triggerbot)
                    Combo("Mode", &Config.NewFeaturesMenu.TriggerbotMode, trigger_modes, IM_ARRAYSIZE(trigger_modes));
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "Zone = the dot crosshair");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "re-press rate = weapon fire interval");
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Dot Crosshair##R1", ImVec2(col_w, full_h), true);
                Checkbox("Dot Crosshair", &Config.Crosshair.Enabled);
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "The triggerbot aims through this dot");
                SliderFloat("Opacity", &Config.Crosshair.Color[3], 0.0f, 1.0f, "%.2f");
                SliderFloat("Size (trigger zone)", &Config.Crosshair.Size, 1.0f, 40.0f, "%.0f px");
                SliderFloat("Offset X", &Config.Crosshair.OffsetX, -400.0f, 400.0f, "%.0f px");
                SliderFloat("Offset Y", &Config.Crosshair.OffsetY, -400.0f, 400.0f, "%.0f px");
                ImGui::TextUnformatted("Crosshair Color");
                ColorEdit4("##crosshair_color", Config.Crosshair.Color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
                if (Button("Reset To Centre", ImVec2(ImGui::GetContentRegionAvail().x, 44)))
                {
                    Config.Crosshair.OffsetX = 0.0f;
                    Config.Crosshair.OffsetY = 0.0f;
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(2))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Weapon Mods##L2", ImVec2(col_w, full_h), true);
                Checkbox("Hitbox", &Config.ExtraMenu.Hit);
                Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
                Checkbox("No Spread", &Config.ExtraMenu.Spread);
                Checkbox("No Shake", &Config.ExtraMenu.Shake);
                Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
                Checkbox("Firerate", &Config.ExtraMenu.Fire);
                Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
                Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
                Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
                Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
                Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
                Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Items & Movement##R2", ImVec2(col_w, full_h), true);
                Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
                Checkbox("Mythic Attachments", &Config.ExtraMenu.MythicAttachments);
                Checkbox("Unlock Blueprints", &Config.ExtraMenu.Blueprints);
                Checkbox("Unlimited Fly", &UnlimitedFly);
                Separator_line();
                SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
                SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
                SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
                SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
                EndChild();
            }

            if (p_tabs.IsTabActive(3))
            {
                static const char* box_types[]        = { "Fill", "Outline", "Corner", "3D" };
                static const char* line_positions[]   = { "Top", "Mid", "Bottom" };
                static const char* health_positions[] = { "Top", "Side" };
                static const char* esp_styles[]       = { "None", "3D Sphere", "Player Signal" };

                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("ESP Player##L3", ImVec2(col_w, full_h), true);
                Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
                Checkbox("ESP Box", &Config.ESPMenu.Box);
                Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
                Checkbox("ESP Health", &Config.ESPMenu.Health);
                Checkbox("ESP Name", &Config.ESPMenu.Name);
                Checkbox("ESP Distance", &Config.ESPMenu.Distance);
                Checkbox("ESP Count", &Config.ESPMenu.Count);
                Checkbox("360 Alert", &Config.ESPMenu.Alert);
                Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
                Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
                Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
                Combo("Box Type", (int*)&Config.ESPMenu.BoxType, box_types, IM_ARRAYSIZE(box_types));
                Combo("Line Position", (int*)&Config.ESPMenu.Target, line_positions, IM_ARRAYSIZE(line_positions));
                Combo("Health Position", (int*)&Config.ESPMenu.HealthPosition, health_positions, IM_ARRAYSIZE(health_positions));
                Combo("ESP Style", (int*)&Config.ESPMenu.EspStyle, esp_styles, IM_ARRAYSIZE(esp_styles));
                Separator_line();
                // One row per team: the colour is the ESP's line colour and the
                // helper fans it out to box / name / health / distance too.
                if (runtime_preview_menu::DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER))
                    ApplyEspColorGroup(Config.sColorsESPPLAYER.LinePLAYER, Config.sColorsESPPLAYER.BoxPLAYER,
                        Config.sColorsESPPLAYER.NamePLAYER, Config.sColorsESPPLAYER.HealthPLAYER,
                        Config.sColorsESPPLAYER.DistancePLAYER);
                if (runtime_preview_menu::DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT))
                    ApplyEspColorGroup(Config.sColorsESPBOT.LineBOT, Config.sColorsESPBOT.BoxBOT,
                        Config.sColorsESPBOT.NameBOT, Config.sColorsESPBOT.HealthBOT,
                        Config.sColorsESPBOT.DistanceBOT);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("ESP Preview##R3", ImVec2(col_w, full_h), true);
                {
                    // The app's own DrawEspPreview - it reads the Config.ESPMenu
                    // flags above, so this panel is the live ESP, not a mock-up.
                    const ImVec2 preview_pos = ImGui::GetCursorScreenPos();
                    const ImVec2 preview_avail = ImGui::GetContentRegionAvail();
                    if (s.draw_esp_preview != nullptr && preview_avail.x > 40.0f && preview_avail.y > 40.0f)
                        s.draw_esp_preview(ImGui::GetWindowDrawList(), preview_pos, preview_avail.x, preview_avail.y);
                    else
                        ImGui::TextUnformatted("ESP preview unavailable");
                }
                EndChild();
            }

            if (p_tabs.IsTabActive(4))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Awareness##L4", ImVec2(col_w, full_h), true);
                Checkbox("Armor ESP", &Config.ESPMenu.Armor);
                Checkbox("Grenade Warning", &Config.ESPMenu.GrenadeWarn);
                Checkbox("BR Class", &Config.ESPMenu.BRClass);
                Checkbox("Vehicle ESP", &Config.ESPMenu.Vehicle);
                Checkbox("Vehicle Health", &Config.ESPMenu.VehicleHealth);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Snapshots##R4", ImVec2(col_w, full_h), true);
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "ESP extras draw through the");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "player ESP pass - toggles apply");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "on the next frame.");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "Hide Stream keeps them out of");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "captured screenshots.");
                Separator_line();
                Checkbox("Hide Stream (screenshot-safe)", &HIDEESP);
                EndChild();
            }

            if (p_tabs.IsTabActive(5))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("World##L5", ImVec2(col_w, full_h), true);
                Checkbox("No Gravity", &Config.ExtraMenu.NoGravity);
                SliderFloat("Gravity Scale", &Config.ExtraMenu.NoGravityScale, 0.1f, 3.0f, "%.2fx");
                Checkbox("Clear Terrain", &Config.ExtraMenu.ClearTerrain);
                Checkbox("No Smoke", &Config.ExtraMenu.NoSmoke);
                Checkbox("Walk Underwater", &Config.ExtraMenu.WalkUnderWater);
                Checkbox("Spectate X", &Config.ExtraMenu.Spectatex);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Camera & Interface##R5", ImVec2(col_w, full_h), true);
                Checkbox("Camera POV", &Config.ExtraMenu.CameraPov);
                SliderFloat("POV Size", &Config.ExtraMenu.CameraPovSize, 0.0f, 100.0f, "%.0f");
                Separator_line();

                // Stretch Screen: writes UnityEngine.Camera.aspect (dump .57),
                // which widens the horizontal FOV while the vertical one stays
                // put. Restores the camera's own aspect when switched off.
                Checkbox("Stretch Screen", &Config.ExtraMenu.StretchScreen);
                if (Config.ExtraMenu.StretchScreen)
                {
                    SliderFloat("Aspect", &Config.ExtraMenu.StretchAspect, 1.0f, 3.0f, "%.2f");
                    const ImVec2 preset = ImVec2((ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f, 34.0f);
                    if (Button("16:9", preset))
                        Config.ExtraMenu.StretchAspect = 1.7778f;
                    ImGui::SameLine(0.0f, 4.0f);
                    if (Button("21:9", preset))
                        Config.ExtraMenu.StretchAspect = 2.3333f;
                    ImGui::SameLine(0.0f, 4.0f);
                    if (Button("4:3", preset))
                        Config.ExtraMenu.StretchAspect = 1.3333f;
                }
                Separator_line();
                Checkbox("Clear Display", &Config.ExtraMenu.ClearDisplay);
                Checkbox("VSync Mode", &Config.ExtraMenu.Grap);
                Separator_line();
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "Camera / interface changes are");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "applied by the world runtime and");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "restore when switched back off.");
                EndChild();
            }

            if (p_tabs.IsTabActive(6))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Performance##L6", ImVec2(col_w, full_h), true);
                if (Checkbox("Frame Smoothness", &Config.Aim.FpsLevel))
                    Config.Aim.showFPSLevelSlider = Config.Aim.FpsLevel;
                if (Config.Aim.showFPSLevelSlider)
                    SliderFloat("Frame Level", &Config.Aim.FpsLevel1, 30.0f, 120.0f, "%.0f");
                if (Checkbox("Ultra Frame Boost", &Config.Aim.FpsLevelUltra))
                    Config.Aim.showFPSLevelUltraSlider = Config.Aim.FpsLevelUltra;
                if (Config.Aim.showFPSLevelUltraSlider)
                    SliderFloat("Ultra Frame Level", &Config.Aim.FpsLevelUltra1, 60.0f, 144.0f, "%.0f");
                Separator_line();
                Checkbox("Smart Reload", &Config.ExtraMenu.SmartReload);
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Account & Data##R6", ImVec2(col_w, full_h), true);
                Checkbox("Attachment Unlock", &Config.ExtraMenu.Attachment);
                Checkbox("Unlock Blueprints", &Config.ExtraMenu.Blueprints);
                Checkbox("Reset Guest Account", &Config.ExtraMenu.ResetGuest);
                Separator_line();
                Checkbox("Camo Diagnostics (test)", &Config.ExtraMenu.CamoTest);
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "Diagnostics only reports what the");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "camo hooks resolve - it changes");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "nothing in a match.");
                EndChild();
            }

            if (p_tabs.IsTabActive(7))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Movement Patches##L7", ImVec2(col_w, full_h), true);
                Checkbox("No Crouch (patched)", &Config.NewFeaturesMenu.NoCrouch);
                Checkbox("No Hit Flinch (patched)", &Config.NewFeaturesMenu.NoHitFlinch);
                Checkbox("No Drift / Hardscope (patched)", &Config.NewFeaturesMenu.NoDrift);
                Checkbox("No Slide Fatigue (patched)", &Config.NewFeaturesMenu.NoSlideFatigue);
                Checkbox("No Hipfire Delay (patched)", &Config.NewFeaturesMenu.NoHipfireDelay);
                Checkbox("No Sprint Fire Delay (patched)", &Config.NewFeaturesMenu.NoSprintToFireDelay);
                if (Config.NewFeaturesMenu.NoSprintToFireDelay)
                {
                    ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "  %s",
                        NewFeatures::g_noSprintFireDelayWeapon.load() != nullptr
                            ? "armed: local weapon cached"
                            : "waiting for the local pawn tick");
                }
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("Patch Status##R7", ImVec2(col_w, full_h), true);
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "These toggles write game code at");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "runtime. Each one is written once");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "per state change and restored the");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "moment you switch it back off, so");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "an idle menu writes nothing.");
                Separator_line();
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "No Drift also needs the patched");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "weapon, so apply it while holding");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "the weapon you want to keep steady.");
                EndChild();
            }

            if (p_tabs.IsTabActive(8))
            {
                // The repo's own skins browser (System/UI/SkinTab.h): character /
                // watch / deadbox / plane / weapon / camo / snowboard rows, the
                // search field with the virtual keyboard and the apply checkboxes
                // that write the weapon's config rows. It used to be the whole
                // menu; it is a page again.
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                Child("Skins##L8", ImVec2(col_w * 2.0f + col_gap, full_h), true);
                if (Config.ExtraMenu.Blueprints)
                    ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.55f, 1.0f),
                        "Blueprints unlocked - the game's own loadout can equip these skins");
                else
                    ImGui::TextColored(ImVec4(0.90f, 0.70f, 0.35f, 1.0f),
                        "Enable Unlock Blueprints (Weapons page) so the loadout accepts the equip");
                RenderTab4Content();
                EndChild();
            }

            if (p_tabs.IsTabActive(9))
            {
                static std::string config_status = "no config written yet";

                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));

                Child("Configuration##L8", ImVec2(col_w, full_h), true);
                if (Button("SAVE CONFIG", ImVec2(ImGui::GetContentRegionAvail().x, 47)))
                {
                    SaveConfiguration("lineflow");
                    config_status = "saved -> configs/lineflow.json";
                }
                if (Button("LOAD CONFIG", ImVec2(ImGui::GetContentRegionAvail().x, 47)))
                    config_status = LoadConfiguration("lineflow")
                        ? "loaded configs/lineflow.json"
                        : "configs/lineflow.json not found";
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "%s", config_status.c_str());
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "Stored in the game's own files dir,");
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "so it survives app updates.");
                Separator_line();
                if (Button("RESET TARGETING", ImVec2(ImGui::GetContentRegionAvail().x, 47)))
                {
                    Config.Crosshair.OffsetX = 0.0f;
                    Config.Crosshair.OffsetY = 0.0f;
                    Config.Crosshair.Size = 6.0f;
                    Config.Crosshair.Color[3] = 0.95f;
                    config_status = "crosshair reset to defaults";
                }
                EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

                Child("License##R8", ImVec2(col_w, full_h), true);
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "LICENSE   : %s", usedKey.c_str());
                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "EXPIRY    : %s", settings_tab::BuildRuntimeExpiryLabel().c_str());
                ImGui::TextColored(userType == "PREMIUM PAID"
                        ? ImVec4(0.75f, 0.60f, 0.00f, 0.86f)
                        : ImVec4(0.00f, 0.80f, 0.00f, 0.86f),
                    "PLAN      : %s", userType.c_str());
                Separator_line();

                // Logo controls (same values the on-screen logo reads).
                static float logo_opacity = 1.0f;
                static float logo_size = 1.0f;
                static bool  logo_loaded = false;
                if (!logo_loaded)
                {
                    logo_opacity = GetLogoOpacity();
                    logo_size = GetLogoSizeMultiplier();
                    logo_loaded = true;
                }
                if (SliderFloat("Logo Opacity", &logo_opacity, 0.0f, 1.0f, "%.2f"))
                    SetLogoOpacity(logo_opacity);
                if (SliderFloat("Logo Size", &logo_size, 0.1f, 2.0f, "%.2f"))
                    SetLogoSizeMultiplier(logo_size);
                if (Button("RESET LOGO", ImVec2(ImGui::GetContentRegionAvail().x, 44)))
                {
                    logo_opacity = 1.0f;
                    logo_size = 1.0f;
                    SetLogoOpacity(1.0f);
                    SetLogoSizeMultiplier(1.0f);
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