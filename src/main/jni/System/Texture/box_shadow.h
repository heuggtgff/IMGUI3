#pragma once

#include "ImGui/imgui.h"

struct RectangleShadowSettings
{
    bool linear = false;
    float sigma = 6.0f;

    ImVec2 padding = ImVec2(60, 60);
    ImVec2 rectPos = ImVec2(50, 50);
    ImVec2 rectSize = ImVec2(120, 120);

    ImVec2 shadowOffset = ImVec2(0, 2);
    ImVec2 shadowSize = ImVec2(150, 70);

    ImColor shadowColor = ImColor(180, 0, 0, 220);

    int rings = 4;
    int spacingBetweenRings = 7;
    int samplesPerCornerSide = 2;
    int spacingBetweenSamples = 16;

    int totalVertices = 0;
    int totalIndices = 0;

    bool enableDebugVisualization = false;
};

void drawRectangleShadowVerticesAdaptive(RectangleShadowSettings& settings, bool foreground = false);
