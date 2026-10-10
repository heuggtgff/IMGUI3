inline void DrawTopContainer(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &size, const char *label, ImFont *font, float fontSize) {
    if (drawList == nullptr || label == nullptr || font == nullptr) {
        return;
    }

    const ImVec2 max = min + size;
   
    drawList->AddRectFilled(min, max, IM_COL32(22, 29, 34, 220), 7.0f);
    
    drawList->AddLine(ImVec2(min.x + 12.0f, max.y - 1.0f), ImVec2(max.x - 12.0f, max.y - 1.0f), IM_COL32(210, 134, 10, 120), 1.0f);

    const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
    const ImVec2 textPos(min.x + (size.x - textSize.x) * 0.5f, min.y + (size.y - textSize.y) * 0.5f);
    drawList->AddText(font, fontSize, textPos, IM_COL32(228, 213, 185, 255), label);
}


inline bool DrawSidebarButton(const char *id, const char *label, const Texture &icon, bool active, const ImVec2 &size) {
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    
    if (active) {
        drawList->AddRectFilled(min, min + size, IM_COL32(38, 48, 56, 200), 6.0f);
        
        drawList->AddRectFilled(
            ImVec2(min.x + 4.0f, min.y + size.y - 3.0f),
            ImVec2(min.x + size.x - 4.0f, min.y + size.y - 1.0f),
            IM_COL32(210, 134, 10, 220),
            2.0f
        );
    } else if (hovered) {
        drawList->AddRectFilled(min, min + size, IM_COL32(30, 38, 44, 140), 6.0f);
    }

    const ImU32 textColor = active
        ? IM_COL32(228, 213, 185, 255)
        : hovered ? IM_COL32(185, 162, 110, 220)
        : IM_COL32(112, 100, 78, 190);

    // Center icon
    ImFont *labelFont = F50 ? F50 : (font::inter_semibold ? font::inter_semibold : ImGui::GetFont());
    const float labelSize = (labelFont == F50) ? 13.5f : labelFont->FontSize * 0.95f;
    const ImVec2 textSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label);

    float contentX = min.x + (size.x - textSize.x) * 0.5f;
    float contentY = min.y + (size.y - textSize.y) * 0.5f - 1.0f;

    if (icon.id != 0) {
        const float iconH = size.y * 0.42f;
        const float iconW = iconH;
        const ImVec2 iconMin(min.x + (size.x - iconW) * 0.5f, min.y + (size.y - iconH) * 0.5f - 2.0f);
        drawList->AddImage((ImTextureID)(intptr_t)icon.id, iconMin, iconMin + ImVec2(iconW, iconH),
            ImVec2(0, 0), ImVec2(1, 1), active ? IM_COL32(228, 213, 185, 255) : IM_COL32(140, 124, 96, 200));
    } else {
        drawList->AddText(labelFont, labelSize, ImVec2(contentX, contentY), textColor, label);
    }
    return clicked;
}

inline bool DrawHeaderImageButton(const char *id, const Texture &icon, const ImVec2 &size, const char *fallbackIcon, const char *fallbackAscii) {
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const float pressOffset = held ? 1.0f : 0.0f;
    const ImVec2 iconMin(min.x, min.y + pressOffset);
    const ImVec2 iconMax(iconMin.x + size.x, iconMin.y + size.y);
    const ImU32 tint = held ? IM_COL32(210, 190, 160, 255) : hovered ? IM_COL32(235, 215, 180, 255) : IM_COL32(220, 200, 165, 245);

    if (icon.id != 0) {
        drawList->AddImage((ImTextureID)(intptr_t)icon.id, iconMin, iconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
    } else if (fallbackIcon != nullptr || fallbackAscii != nullptr) {
        ImFont *iconFont = F107 ? F107 : (font::inter_semibold ? font::inter_semibold : ImGui::GetFont());
        const char *glyph = (F107 != nullptr && fallbackIcon != nullptr) ? fallbackIcon : fallbackAscii;
        if (glyph != nullptr) {
            const float glyphSize = (F107 != nullptr) ? (size.y * 0.58f) : (ImGui::GetFontSize() * 0.95f);
            const ImVec2 glyphSizeVec = iconFont->CalcTextSizeA(glyphSize, FLT_MAX, 0.0f, glyph);
            const ImVec2 glyphPos(
                min.x + (size.x - glyphSizeVec.x) * 0.5f,
                min.y + pressOffset + (size.y - glyphSizeVec.y) * 0.5f
            );
            drawList->AddText(iconFont, glyphSize, glyphPos, tint, glyph);
        }
    }
    return clicked;
}

inline void HandleHeaderDragZone(const char *id, const ImVec2 &min, const ImVec2 &size) {
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }

    ImGui::SetCursorScreenPos(min);
    ImGui::InvisibleButton(id, size);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 0.0f)) {
        ImGui::SetWindowPos(ImGui::GetWindowPos() + ImGui::GetIO().MouseDelta);
    }
}

struct ChildFrame {
    ImVec2 min;
    ImVec2 size;
    float rounding;
};

inline ChildFrame BeginContentChild(const char *id, const ImVec2 &size, ImGuiWindowFlags flags = 0) {
    ChildFrame frame{};
    frame.min = ImGui::GetCursorScreenPos();
    frame.size = size;
    frame.rounding = c::child::rounding * c::scale;
    custom::Child(id, size, true, flags);
    return frame;
}

inline void EndContentChild(const ChildFrame &frame) {
    custom::EndChild();
}

inline void ContentGap(float height = 10.0f) {
    ImGui::Dummy(ImVec2(0.0f, height));
}

inline bool DrawActionButton(const char *id, const char *label, float h = 50.0f) {
    const float sidePad = 10.0f;
    const float rowAvailW = ImGui::GetContentRegionAvail().x;
    const ImVec2 p(ImGui::GetCursorScreenPos().x + sidePad, ImGui::GetCursorScreenPos().y);
    const ImVec2 size(ImMax(1.0f, rowAvailW - sidePad * 2.0f), h);

    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton(id, size);
    const bool clicked = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImU32 bg = IM_COL32(22, 29, 34, 228);
    if (hovered) bg = IM_COL32(38, 48, 56, 238);
    if (held) bg = IM_COL32(52, 64, 72, 246);
    const ImU32 border = IM_COL32(210, 134, 10, hovered ? 180 : 120);
    dl->AddRectFilled(p, p + size, bg, 10.0f);
    dl->AddRect(p, p + size, border, 10.0f, 0, 1.0f);

    ImFont *labelFont = F50 ? F50 : (font::inter_semibold ? font::inter_semibold : ImGui::GetFont());
    const float labelSize = (labelFont == F50) ? 13.0f : labelFont->FontSize * 0.92f;
    const ImVec2 ts = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label);
    dl->AddText(labelFont, labelSize, ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), IM_COL32(228, 213, 185, 255), label);
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    return clicked;
}

inline void DrawToggleRow(const char *label, bool *value) {
    if (label == nullptr || value == nullptr) {
        return;
    }

    const float rowH = 34.0f;
    const float rowGap = 8.0f;
    const float rowW = ImMax(1.0f, ImGui::GetContentRegionAvail().x);
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax(rowMin.x + rowW, rowMin.y + rowH);

    ImGui::PushID(label);
    const ImGuiID animId = ImGui::GetID("##toggle_anim");
    ImGui::InvisibleButton("##toggle_row", ImVec2(rowW, rowH));
    const bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        *value = !(*value);
    }
    ImGui::PopID();

    ImGuiStorage *storage = ImGui::GetStateStorage();
    const float targetT = *value ? 1.0f : 0.0f;
    float animT = storage->GetFloat(animId, targetT);
    float follow = ImGui::GetIO().DeltaTime * 14.0f;
    if (follow > 1.0f) follow = 1.0f;
    animT += (targetT - animT) * follow;
    storage->SetFloat(animId, animT);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    if (hovered) {
        drawList->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 8), 8.0f);
    }

    ImFont *labelFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float labelSize = labelFont->FontSize * 0.84f;
    const ImVec2 textSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label);
    drawList->AddText(labelFont, labelSize, ImVec2(rowMin.x + 4.0f, rowMin.y + (rowH - textSize.y) * 0.5f), hovered ? IM_COL32(228, 213, 185, 255) : IM_COL32(185, 168, 135, 235), label);

    ImFont *statusFont = F50 ? F50 : labelFont;
    const float statusSize = (statusFont == F50) ? 13.5f : (labelFont->FontSize * 0.84f);
    const char *statusText = *value ? "ON" : "OFF";
    const ImVec2 statusTextSize = statusFont->CalcTextSizeA(statusSize, FLT_MAX, 0.0f, statusText);
    // OFF: muted slate, ON: warm amber
    const int offR = 90, offG = 100, offB = 108;
    const int onR = 210, onG = 134, onB = 10;
    const int statusR = (int)(offR + (onR - offR) * animT);
    const int statusG = (int)(offG + (onG - offG) * animT);
    const int statusB = (int)(offB + (onB - offB) * animT);
    drawList->AddText(statusFont, statusSize, ImVec2(rowMax.x - 10.0f - statusTextSize.x, rowMin.y + (rowH - statusTextSize.y) * 0.5f), IM_COL32(statusR, statusG, statusB, hovered ? 255 : 236), statusText);

    ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y + rowGap));
}

inline bool DrawSliderBar(const char *id, const char *label, float *value, float minValue, float maxValue, const ImVec2 &requestedSize, const char *format = nullptr) {
    if (value == nullptr) {
        return false;
    }

    const float width = requestedSize.x > 0.0f ? requestedSize.x : ImGui::GetContentRegionAvail().x;
    const float height = requestedSize.y > 0.0f ? requestedSize.y : 44.0f;
    const ImVec2 rowCursor = ImGui::GetCursorPos();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + width, min.y + height);
    ImGui::Dummy(ImVec2(width, height));

    ImGuiStorage *storage = ImGui::GetStateStorage();
    const ImGuiID animId = ImGui::GetID(id);
    const float sliderMinX = min.x + 10.0f;
    const float sliderMaxX = max.x - 10.0f;
    const float sliderW = sliderMaxX - sliderMinX;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImFont *sliderFont = ImGui::GetFont();
    const float sliderLabelSize = sliderFont->FontSize * 0.84f;
    const float rowY = min.y + 1.0f;
    const float trackH = 6.0f;
    const float trackY = rowY + sliderLabelSize + 10.0f + trackH * 0.5f;
    const float knobR = 8.0f;
    const ImVec2 trackMin(sliderMinX, trackY - trackH * 0.5f);
    const ImVec2 trackMax(sliderMaxX, trackY + trackH * 0.5f);
    const ImVec2 hitMin(trackMin.x, ImMax(min.y, trackMin.y - 10.0f));
    const ImVec2 hitMax(trackMax.x, ImMin(max.y, trackMax.y + 10.0f));

    ImGui::SetCursorScreenPos(hitMin);
    ImGui::InvisibleButton(id, ImVec2(hitMax.x - hitMin.x, hitMax.y - hitMin.y));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::SetCursorPos(ImVec2(rowCursor.x, rowCursor.y + height + 10.0f));

    bool valueChanged = false;
    if (active) {
        float tDrag = (ImGui::GetIO().MousePos.x - sliderMinX) / (sliderW > 0.0f ? sliderW : 1.0f);
        tDrag = ImClamp(tDrag, 0.0f, 1.0f);
        const float nextValue = minValue + (maxValue - minValue) * tDrag;
        valueChanged = std::fabs(nextValue - *value) > 0.0001f;
        *value = nextValue;
    }

    float t = (*value - minValue) / ((maxValue - minValue) > 0.0f ? (maxValue - minValue) : 1.0f);
    t = ImClamp(t, 0.0f, 1.0f);
    float animT = storage->GetFloat(animId, t);
    float follow = ImGui::GetIO().DeltaTime * (active ? 35.0f : 12.0f);
    if (follow > 1.0f) follow = 1.0f;
    animT += (t - animT) * follow;
    storage->SetFloat(animId, animT);

    const float shownValue = minValue + (maxValue - minValue) * animT;
    char valueBuf[32] = {};
    if (format != nullptr) {
        std::snprintf(valueBuf, sizeof(valueBuf), format, shownValue);
    } else {
        std::snprintf(valueBuf, sizeof(valueBuf), "%.2f", shownValue);
    }

    const ImVec2 valueSize = sliderFont->CalcTextSizeA(sliderLabelSize, FLT_MAX, 0.0f, valueBuf);
    dl->AddText(sliderFont, sliderLabelSize, ImVec2(sliderMinX, rowY), IM_COL32(185, 168, 135, 240), label);
    dl->AddText(sliderFont, sliderLabelSize, ImVec2(sliderMaxX - valueSize.x, rowY), IM_COL32(210, 155, 50, 255), valueBuf);
    dl->AddRectFilled(trackMin, trackMax, IM_COL32(30, 38, 44, 235), trackH * 0.5f);
    dl->AddRect(trackMin, trackMax, IM_COL32(62, 75, 85, 160), trackH * 0.5f, 0, 1.0f);

    const float fillX = trackMin.x + (trackMax.x - trackMin.x) * animT;
    if (fillX > trackMin.x + 1.0f) {
        dl->AddRectFilled(trackMin, ImVec2(fillX, trackMax.y), IM_COL32(180, 108, 8, 230), trackH * 0.5f, ImDrawFlags_RoundCornersLeft);
    }

    const ImVec2 knob(fillX, trackY);
    dl->AddCircleFilled(knob, knobR + 4.0f, IM_COL32(200, 130, 10, active ? 76 : hovered ? 58 : 38), 22);
    dl->AddCircleFilled(knob, knobR + 2.0f, IM_COL32(200, 130, 10, active ? 130 : 100), 22);
    dl->AddCircleFilled(knob, knobR, IM_COL32(210, 140, 18, 255), 24);
    dl->AddCircle(knob, knobR + 0.3f, IM_COL32(240, 220, 180, 220), 24, 1.6f);

    return valueChanged;
}

inline void DrawComboRow(const char *label, const char *id, int *index, const char *const *items, int count) {
    if (index == nullptr || items == nullptr || count <= 0) {
        return;
    }

    *index = ImClamp(*index, 0, count - 1);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(185, 168, 135, 212));
    ImGui::SetWindowFontScale(0.86f);
    ImGui::TextUnformatted(label);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    const ImVec4 comboTextColor = ImVec4(0.90f, 0.84f, 0.72f, 0.97f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.12f, 0.16f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.16f, 0.21f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.16f, 0.20f, 0.26f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.77f, 0.56f, 1.00f, 0.86f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.08f, 0.11f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Text, comboTextColor);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.48f, 0.36f, 0.10f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.66f, 0.50f, 0.14f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.66f, 0.50f, 0.14f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 10.0f));

    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::SetNextItemWidth(ImMax(120.0f, ImGui::GetContentRegionAvail().x - 20.0f));
    if (ImGui::BeginCombo(id, items[*index])) {
        const float popupItemHeight = ImGui::GetTextLineHeightWithSpacing() + 8.0f;
        for (int i = 0; i < count; ++i) {
            const bool selected = (*index == i);
            if (ImGui::Selectable(items[i], selected, 0, ImVec2(0.0f, popupItemHeight))) {
                *index = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(205, 132, 255, 220), 4.0f, 0, 1.0f);

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(9);
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
}

inline bool DrawEspColorRow(const char *label, float *color) {
    if (color == nullptr) return false;

    auto normalize = [](float v) {
        return (v <= 1.0f) ? ImClamp(v, 0.0f, 1.0f) : ImClamp(v / 255.0f, 0.0f, 1.0f);
    };

    float col[4] = {
        normalize(color[0]),
        normalize(color[1]),
        normalize(color[2]),
        normalize(color[3])
    };

    const float rowX = ImGui::GetCursorPosX();
    const float rowW = ImGui::GetContentRegionAvail().x;
    const float previewW = ImGui::GetFrameHeight() * 1.15f;

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.90f, 0.84f, 0.72f, 0.97f), "%s", label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(rowX + ImMax(0.0f, rowW - previewW));
    ImGui::SetNextItemWidth(previewW);
    bool changed = ImGui::ColorEdit4(
        (std::string("##") + label).c_str(),
        col,
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_PickerHueWheel
    );

    if (changed) {
        color[0] = col[0] * 255.0f;
        color[1] = col[1] * 255.0f;
        color[2] = col[2] * 255.0f;
        color[3] = 255.0f;
    }

    return changed;
}

inline void PushNativeComboStyle() {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.12f, 0.16f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.16f, 0.21f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.16f, 0.20f, 0.26f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.77f, 0.56f, 1.00f, 0.86f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.08f, 0.11f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.84f, 0.72f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.48f, 0.36f, 0.10f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.66f, 0.50f, 0.14f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.66f, 0.50f, 0.14f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 10.0f));
}

inline void PopNativeComboStyle() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(9);
}

inline void PushNativeSliderStyle() {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.08f, 0.18f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.10f, 0.25f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.19f, 0.12f, 0.29f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.73f, 0.45f, 0.98f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.83f, 0.60f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.64f, 0.43f, 0.90f, 0.78f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.95f, 1.0f, 0.98f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
}

inline void PopNativeSliderStyle() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);
}

inline void PushNativeColorStyle() {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.12f, 0.16f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.16f, 0.21f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.16f, 0.20f, 0.26f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.77f, 0.56f, 1.00f, 0.86f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.08f, 0.11f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.84f, 0.72f, 0.97f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
}

inline void PopNativeColorStyle() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);
}

inline void PushNativeButtonStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.06f, 0.15f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.10f, 0.25f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.13f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.77f, 0.56f, 1.00f, 0.86f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.96f, 0.95f, 1.00f, 0.98f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
}

inline void PopNativeButtonStyle() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
}

inline void PushNativeCheckboxStyle() {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.10f, 0.18f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.12f, 0.24f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.14f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.75f, 0.56f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.63f, 0.44f, 0.92f, 0.78f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.94f, 0.99f, 0.97f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
}

inline void PopNativeCheckboxStyle() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);
}

inline void RenderInfoPopupModal() {
    if (g_infoPopupRequested) {
        ImGui::OpenPopup("##RuntimeInfoPopup");
        g_infoPopupRequested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.12f, 0.15f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.82f, 0.55f, 0.08f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::BeginPopupModal("##RuntimeInfoPopup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        ImFont *titleFont = F50 ? F50 : (font::inter_semibold ? font::inter_semibold : ImGui::GetFont());
        const float titleFontSize = (titleFont == F50) ? 18.0f : titleFont->FontSize * 1.10f;
        const ImVec2 headerCursor = ImGui::GetCursorScreenPos();
        const ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        const ImVec2 closeButtonSize(36.0f, 28.0f);
        ImGui::GetWindowDrawList()->AddText(titleFont, titleFontSize, headerCursor, IM_COL32(228, 213, 185, 255), "INFO");
        ImGui::SetCursorScreenPos(ImVec2(headerCursor.x + ImMax(0.0f, contentAvail.x - closeButtonSize.x), headerCursor.y));
        if (ImGui::Button("X##RuntimeInfoClose", closeButtonSize)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetCursorScreenPos(ImVec2(headerCursor.x, headerCursor.y + closeButtonSize.y));
        ImGui::Dummy(ImVec2(contentAvail.x, 8.0f));

        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.09f, 0.12f, 0.15f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.12f, 0.16f, 0.20f, 0.78f));
        if (ImGui::BeginTable("##RuntimeInfoTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 132.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            auto drawInfoRow = [](const char *icon, const char *field, const char *value) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (F107 != nullptr) ImGui::PushFont(F107);
                ImGui::TextColored(ImVec4(0.82f, 0.65f, 0.25f, 0.96f), "%s", icon);
                if (F107 != nullptr) ImGui::PopFont();
                ImGui::SameLine(0.0f, 8.0f);
                ImGui::TextColored(ImVec4(0.90f, 0.74f, 0.32f, 0.96f), "%s", field);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.88f, 0.82f, 0.72f, 0.94f), "%s", value);
            };
            drawInfoRow(ICON_FA_USER, "Developer", "Astral Premium");
            drawInfoRow(ICON_FA_GAMEPAD, "Game", "Call of Duty: Mobile");
            drawInfoRow(ICON_FA_MICROCHIP, "Architecture", "arm64-v8a");
            drawInfoRow(ICON_FA_UNLOCK_ALT, "Access", "Non-root");
            ImGui::EndTable();
        }
        ImGui::PopStyleColor(2);

        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::SetCursorPosX(ImMax(0.0f, ImGui::GetContentRegionAvail().x - 112.0f));
        if (ImGui::Button("CLOSE", ImVec2(112.0f, 40.0f))) {
            ImGui::CloseCurrentPopup();
        }
        RegisterPopupFocusWindow();
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

inline void RenderPriceListPopupModal() {
    if (g_priceListPopupRequested) {
        ImGui::OpenPopup("##RuntimePriceListPopup");
        g_priceListPopupRequested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.12f, 0.15f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.82f, 0.55f, 0.08f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImGui::BeginPopupModal("##RuntimePriceListPopup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
        ImFont *titleFont = F50 ? F50 : (font::inter_semibold ? font::inter_semibold : ImGui::GetFont());
        const float titleFontSize = (titleFont == F50) ? 18.0f : titleFont->FontSize * 1.10f;
        const ImVec2 headerCursor = ImGui::GetCursorScreenPos();
        const ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        const ImVec2 closeButtonSize(36.0f, 28.0f);
        ImGui::GetWindowDrawList()->AddText(titleFont, titleFontSize, headerCursor, IM_COL32(228, 213, 185, 255), "PRICELIST");
        ImGui::SetCursorScreenPos(ImVec2(headerCursor.x + ImMax(0.0f, contentAvail.x - closeButtonSize.x), headerCursor.y));
        if (ImGui::Button("X##RuntimePriceClose", closeButtonSize)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetCursorScreenPos(ImVec2(headerCursor.x, headerCursor.y + closeButtonSize.y));
        ImGui::Dummy(ImVec2(contentAvail.x, 8.0f));

        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.09f, 0.12f, 0.15f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.14f, 0.18f, 0.22f, 0.82f));
        if (ImGui::BeginTable("##RuntimePriceListTable", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("PHP", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Dollars", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableHeadersRow();

            int rowIndex = 0;
            auto drawRow = [&rowIndex](const char *duration, const char *php, const char *dollars) {
                ImGui::TableNextRow();
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, (rowIndex % 2 == 0) ? IM_COL32(22, 29, 36, 156) : IM_COL32(32, 40, 48, 176));
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.88f, 0.82f, 0.72f, 0.94f), "%s", duration);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.90f, 0.74f, 0.32f, 0.96f), "%s", php);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.90f, 0.74f, 0.32f, 0.96f), "%s", dollars);
                ++rowIndex;
            };
            drawRow("3 Day", "PHP150", "$3");
            drawRow("7 Days", "PHP300", "$6");
            drawRow("30 Days", "PHP1000", "$60");
            drawRow("60 Days", "PHP1800", "$110");
            ImGui::EndTable();
        }
        ImGui::PopStyleColor(2);

        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::SetCursorPosX(ImMax(0.0f, ImGui::GetContentRegionAvail().x - 112.0f));
        if (ImGui::Button("CLOSE", ImVec2(112.0f, 40.0f))) {
            ImGui::CloseCurrentPopup();
        }
        RegisterPopupFocusWindow();
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}
