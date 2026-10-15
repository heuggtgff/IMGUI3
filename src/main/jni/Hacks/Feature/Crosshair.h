#pragma once

// ============================================================================
//  Crosshair.h  -  the menu's own dot crosshair
//
//  A single filled dot drawn over the game. Where it sits comes from
//  NewFeatures::CrosshairDotCentrePx() - the SAME formula the triggerbot's zone
//  and centre ray use - so the dot and the point the trigger fires through can
//  never drift apart: there is one centre, and the render centre plus the
//  offsets is it (offsets default to 0, i.e. exactly the centre).
//
//  Settings live in Config.Crosshair (Hacks/Feature/Init_Hacks.h): Enabled,
//  TriggerbotUsesIt, Color (RGB + opacity in [3]), Size, OffsetX, OffsetY.
//
//  Include AFTER Hacks/Feature/DrawESP.h and Hacks/Feature/NewFeatures.h: this
//  uses DrawESP.h's ToColor() for the colour and the NewFeatures helpers for the
//  position.
// ============================================================================

static inline void DrawDotCrosshair(ImDrawList *draw, float screenW, float screenH)
{
    if (!Config.Crosshair.Enabled || draw == nullptr) return;

    const float size = Config.Crosshair.Size;
    if (!(size > 0.0f)) return;          // size 0 -> nothing to draw

    float cx = 0.0f, cy = 0.0f;
    NewFeatures::CrosshairDotCentrePx(cx, cy);

    // A dot dragged outside the render area is invisible anyway; skipping it
    // keeps a stale offset from smearing a stray dot over the menu later.
    if (cx < 0.0f || cy < 0.0f || cx > screenW || cy > screenH) return;

    // Filled circle: ToColor takes the same float[4] the menu's colour picker
    // writes (0..1 per channel; alpha == the Opacity slider, so the dot fades
    // with the game behind it instead of always being opaque).
    draw->AddCircleFilled(ImVec2(cx, cy), size * 0.5f, ToColor(Config.Crosshair.Color), 32);
}
