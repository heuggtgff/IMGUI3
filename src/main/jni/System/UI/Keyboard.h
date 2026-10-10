#pragma once

#include "../../ImGui/imgui.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <string>

struct KeyboardAnimationState {
    float hover_alpha = 0.0f;
    float press_alpha = 0.0f;
    float press_time = 0.0f;
};

struct KeyboardState {
    bool is_visible = false;
    float show_time = 0.0f;
    bool allow_clicks = false;
    float open_progress = 0.0f;
    float last_frame_time = 0.0f;
};

static std::map<std::string, KeyboardState> keyboardStates;

namespace keyboard_ui {

inline size_t SafeLen(const char* text, size_t max_len) {
    if (text == nullptr || max_len == 0) {
        return 0;
    }
    size_t len = 0;
    while (len < max_len && text[len] != '\0') {
        ++len;
    }
    return len;
}

inline void AppendText(char* buffer, size_t capacity, const char* text) {
    if (buffer == nullptr || text == nullptr || capacity == 0) {
        return;
    }

    size_t current_len = SafeLen(buffer, capacity);
    if (current_len >= capacity - 1) {
        return;
    }

    size_t add_len = std::strlen(text);
    size_t remaining = (capacity - 1) - current_len;
    size_t copy_len = std::min(add_len, remaining);
    if (copy_len == 0) {
        return;
    }

    std::memcpy(buffer + current_len, text, copy_len);
    buffer[current_len + copy_len] = '\0';
}

inline void Backspace(char* buffer, size_t capacity) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }

    size_t len = SafeLen(buffer, capacity);
    if (len > 0) {
        buffer[len - 1] = '\0';
    }
}

inline float EaseOutSmoothstep(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float ComputeDeltaTime(float current_time, float& last_time) {
    float dt = current_time - last_time;
    if (last_time <= 0.0f || dt <= 0.0f || dt > 0.1f) {
        dt = 1.0f / 60.0f;
    }
    last_time = current_time;
    return dt;
}

inline bool DrawKeyButton(
    ImDrawList* draw,
    const std::string& full_id,
    const char* label,
    const ImVec2& rect_min,
    const ImVec2& rect_max,
    bool allow_clicks,
    float current_time,
    float dt,
    float ui_alpha,
    bool dark_key,
    std::map<std::string, KeyboardAnimationState>& key_animations)
{
    auto& anim = key_animations[full_id];

    const bool hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);
    const bool clicked = hovered && allow_clicks && ImGui::IsMouseClicked(0);

    const float target_hover = hovered ? 1.0f : 0.0f;
    anim.hover_alpha += (target_hover - anim.hover_alpha) * (12.0f * dt);
    anim.hover_alpha = ImClamp(anim.hover_alpha, 0.0f, 1.0f);

    if (clicked) {
        anim.press_time = current_time;
        anim.press_alpha = 1.0f;
    }

    const float since_press = current_time - anim.press_time;
    if (since_press < 0.15f) {
        anim.press_alpha = 1.0f - (since_press / 0.15f);
    } else {
        anim.press_alpha = 0.0f;
    }

    const float effect_alpha = ImMax(anim.hover_alpha * 0.30f, anim.press_alpha * 0.50f);
    const float base_alpha = dark_key ? 0.70f : 0.50f;
    const float fill_alpha = ImClamp(base_alpha + effect_alpha, 0.0f, 1.0f) * ui_alpha;
    const ImU32 fill_col = dark_key
        ? ImColor(60, 60, 60, (int)(255.0f * fill_alpha))
        : ImColor(100, 100, 100, (int)(255.0f * fill_alpha));

    const float rounding = 13.0f;
    draw->AddRectFilled(rect_min, rect_max, fill_col, rounding);

    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const float rect_w = rect_max.x - rect_min.x;
    const float rect_h = rect_max.y - rect_min.y;
    const ImVec2 text_pos(
        rect_min.x + (rect_w - text_size.x) * 0.5f,
        rect_min.y + (rect_h - text_size.y) * 0.5f
    );
    draw->AddText(text_pos, ImColor(255, 255, 255, (int)(255.0f * ui_alpha)), label);

    return clicked;
}

} // namespace keyboard_ui

inline void RenderVirtualKeyboard(const char* id, char* searchQuery, size_t querySize, bool* showKeyboard) {
    if (id == nullptr || searchQuery == nullptr || querySize == 0 || showKeyboard == nullptr) {
        return;
    }

    static std::map<std::string, KeyboardAnimationState> key_animations;

    const std::string keyboard_id(id);
    KeyboardState& kb_state = keyboardStates[keyboard_id];
    const float now = (float)ImGui::GetTime();

    const bool stale_hidden = (now - kb_state.last_frame_time) > 0.25f;
    if (*showKeyboard && (!kb_state.is_visible || stale_hidden)) {
        kb_state.is_visible = true;
        kb_state.show_time = now;
        kb_state.allow_clicks = false;
        kb_state.open_progress = 0.0f;
        kb_state.last_frame_time = now;
    } else if (!*showKeyboard) {
        kb_state.is_visible = false;
        kb_state.open_progress = 0.0f;
        kb_state.allow_clicks = false;
        return;
    }

    if (!kb_state.is_visible) {
        return;
    }

    const float dt = keyboard_ui::ComputeDeltaTime(now, kb_state.last_frame_time);

    kb_state.open_progress += (1.0f - kb_state.open_progress) * (8.0f * dt);
    if (kb_state.open_progress > 0.995f) {
        kb_state.open_progress = 1.0f;
    }

    if ((now - kb_state.show_time) > 0.10f) {
        kb_state.allow_clicks = true;
    }

    const float ease = keyboard_ui::EaseOutSmoothstep(kb_state.open_progress);
    const float slide_offset_y = (1.0f - ease) * 34.0f;
    const float ui_alpha = ease;

    ImGuiIO& io = ImGui::GetIO();
    const float screen_w = io.DisplaySize.x;
    const float screen_h = io.DisplaySize.y;
    const float keyboard_h = screen_h * 0.56f;

    ImGui::SetNextWindowPos(ImVec2(0.0f, screen_h - keyboard_h), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(screen_w, keyboard_h), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 13.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 18.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(18.0f / 255.0f, 18.0f / 255.0f, 20.0f / 255.0f, (230.0f / 255.0f) * ui_alpha));

    ImGui::Begin(
        id,
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float content_w = ImGui::GetContentRegionAvail().x;
    const float content_h = ImGui::GetContentRegionAvail().y;

    const float key_spacing_x = 8.0f;
    const float key_spacing_y = 10.0f;
    const int row_count = 5;
    const float key_w = (content_w - key_spacing_x * 9.0f) / 10.0f;
    const float max_key_h = 60.0f;
    const float min_key_h = 34.0f;
    const float auto_key_h = (content_h - key_spacing_y * (row_count - 1)) / row_count;
    const float key_h = ImClamp(auto_key_h, min_key_h, max_key_h);
    const float bottom_key_h = key_h;

    auto row_start_x = [&](int key_count, float key_width) -> float {
        const float total_w = key_count * key_width + (key_count - 1) * key_spacing_x;
        return origin.x + (content_w - total_w) * 0.5f;
    };

    auto draw_char_row = [&](const char* row_chars, float y) -> float {
        const int count = (int)std::strlen(row_chars);
        const float start_x = row_start_x(count, key_w);

        for (int i = 0; i < count; ++i) {
            const char ch[2] = { row_chars[i], '\0' };
            const float x = start_x + i * (key_w + key_spacing_x);
            const ImVec2 rect_min(x, y + slide_offset_y);
            const ImVec2 rect_max(x + key_w, y + key_h + slide_offset_y);

            const std::string full_id = keyboard_id + "_" + ch;
            if (keyboard_ui::DrawKeyButton(draw, full_id, ch, rect_min, rect_max, kb_state.allow_clicks, now, dt, ui_alpha, false, key_animations)) {
                keyboard_ui::AppendText(searchQuery, querySize, ch);
            }
        }

        return y + key_h + key_spacing_y;
    };

    float y = origin.y;

    y = draw_char_row("1234567890", y);
    y = draw_char_row("QWERTYUIOP", y);
    y = draw_char_row("ASDFGHJKL", y);

    {
        const float start_x = row_start_x(9, key_w);
        const char* special_left = "[M]";
        const char* letters = "ZXCVBNM";
        const char* special_right = "del";

        const ImVec2 m_rect_min(start_x, y + slide_offset_y);
        const ImVec2 m_rect_max(start_x + key_w, y + key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_[M]", special_left, m_rect_min, m_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, true, key_animations)) {
            keyboard_ui::AppendText(searchQuery, querySize, special_left);
        }

        for (int i = 0; i < 7; ++i) {
            char ch[2] = { letters[i], '\0' };
            const float x = start_x + (i + 1) * (key_w + key_spacing_x);
            const ImVec2 rect_min(x, y + slide_offset_y);
            const ImVec2 rect_max(x + key_w, y + key_h + slide_offset_y);

            if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_" + ch, ch, rect_min, rect_max, kb_state.allow_clicks, now, dt, ui_alpha, false, key_animations)) {
                keyboard_ui::AppendText(searchQuery, querySize, ch);
            }
        }

        const float del_x = start_x + 8 * (key_w + key_spacing_x);
        const ImVec2 del_rect_min(del_x, y + slide_offset_y);
        const ImVec2 del_rect_max(del_x + key_w, y + key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_del", special_right, del_rect_min, del_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, true, key_animations)) {
            keyboard_ui::Backspace(searchQuery, querySize);
        }

        y += key_h + key_spacing_y;
    }

    {
        const float side_w = key_w;
        const float space_w = key_w * 5.5f + key_spacing_x * 4.5f;
        const float total_w = side_w + key_spacing_x + side_w + key_spacing_x + space_w + key_spacing_x + side_w;
        const float start_x = origin.x + (content_w - total_w) * 0.5f;

        const ImVec2 l_rect_min(start_x, y + slide_offset_y);
        const ImVec2 l_rect_max(start_x + side_w, y + bottom_key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_[L]", "[L]", l_rect_min, l_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, true, key_animations)) {
            keyboard_ui::AppendText(searchQuery, querySize, "[L]");
        }

        const float e_x = start_x + side_w + key_spacing_x;
        const ImVec2 e_rect_min(e_x, y + slide_offset_y);
        const ImVec2 e_rect_max(e_x + side_w, y + bottom_key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_[E]", "[E]", e_rect_min, e_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, true, key_animations)) {
            keyboard_ui::AppendText(searchQuery, querySize, "[E]");
        }

        const float space_x = e_x + side_w + key_spacing_x;
        const ImVec2 space_rect_min(space_x, y + slide_offset_y);
        const ImVec2 space_rect_max(space_x + space_w, y + bottom_key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_space", "space", space_rect_min, space_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, false, key_animations)) {
            keyboard_ui::AppendText(searchQuery, querySize, " ");
        }

        const float clear_x = space_x + space_w + key_spacing_x;
        const ImVec2 clear_rect_min(clear_x, y + slide_offset_y);
        const ImVec2 clear_rect_max(clear_x + side_w, y + bottom_key_h + slide_offset_y);
        if (keyboard_ui::DrawKeyButton(draw, keyboard_id + "_clear", "Clear", clear_rect_min, clear_rect_max, kb_state.allow_clicks, now, dt, ui_alpha, true, key_animations)) {
            searchQuery[0] = '\0';
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}
