#pragma once

// ============================================================================
//  Crosshair.h  -  the menu's dot crosshair - and the triggerbot's crosshair
//
//  A single filled dot drawn over the game. There is ONE crosshair in this
//  build: where the dot sits comes from NewFeatures::CrosshairCentrePx() - the
//  render centre plus the user's X/Y offsets - and that same pixel is what the
//  triggerbot aims through, while the dot's own radius is the trigger's zone
//  (NewFeatures::A_FireCrosshairRadiusPx). The dot and the shot therefore cannot
//  drift apart, and nothing has to be switched on to connect them: the trigger
//  uses the dot whether or not the dot is drawn.
//
//  Settings live in Config.Crosshair (Hacks/Feature/Init_Hacks.h): Enabled
//  (draw the dot - on by default), Color (RGB + opacity in [3]), Size (dot
//  DIAMETER in px, which is also the trigger zone diameter), OffsetX, OffsetY.
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
    NewFeatures::CrosshairCentrePx(cx, cy);

    // A dot dragged outside the render area is invisible anyway; skipping it
    // keeps a stale offset from smearing a stray dot over the menu later.
    if (cx < 0.0f || cy < 0.0f || cx > screenW || cy > screenH) return;

    // Filled circle: ToColor takes the same float[4] the menu's colour picker
    // writes (0..1 per channel; alpha == the Opacity slider, so the dot fades
    // with the game behind it instead of always being opaque). The radius drawn
    // here (size * 0.5) IS NewFeatures::A_FireCrosshairRadiusPx(), the radius the
    // trigger measures its zone with - one circle, drawn and used.
    draw->AddCircleFilled(ImVec2(cx, cy), size * 0.5f, ToColor(Config.Crosshair.Color), 32);
}
