#pragma once
// imgui_internal.h is where ImMin / ImMax / ImClamp live (imgui.h has the
// ImVec2 helpers but not these), and Main.cpp already pulls it in.
#include "imgui.h"
#include "imgui_internal.h"

// ---------------------------------------------------------------------------
// Menu layout - where the @ThaxxylHax window goes, and where its sidebar rows go
// ---------------------------------------------------------------------------
// The window is drawn in the display's own pixels, and that display is the
// game's render surface, not the phone's panel: it can be anything from roughly
// 640 px up. A fixed 900x600 window therefore read as "the menu is so big" on a
// small surface and as a corner widget on a big one, and the skin / character
// lists - the tabs that actually need room - were the ones squeezed. The window
// follows the display instead: 92% of it, with a 16 px margin all round, a floor
// for the two-column tabs and a hard ceiling of the surface itself.
//
// The tab switcher is a sidebar inside the window (it replaced the floating
// radial wheel), so the window no longer shares the screen with a second
// overlay: it is centred on its own and the sidebar lives in its own column.
struct MenuLayout
{
    float  menuW, menuH;    // the @ThaxxylHax window
    ImVec2 menuPos;
};

inline MenuLayout ComputeMenuLayout(const ImVec2 &display)
{
    MenuLayout L{};
    const float pad    = ImMax(6.0f, ImMin(display.x, display.y) * 0.02f);
    const float availW = ImMax(1.0f, display.x - pad * 2.0f);
    const float availH = ImMax(1.0f, display.y - pad * 2.0f);

    // 92% of the display, but never past the margin, never below the minimum a
    // two-column tab can render in, and never bigger than the surface itself -
    // on a surface smaller than the minimum the margin wins, because a window
    // wider than the screen is worse than a cramped one.
    L.menuW = ImClamp(display.x * 0.92f, ImMin(360.0f, availW), availW);
    L.menuH = ImClamp(display.y * 0.92f, ImMin(320.0f, availH), availH);

    // Centred, and never hanging off the screen.
    L.menuPos.x = ImClamp((display.x - L.menuW) * 0.5f, 0.0f, ImMax(0.0f, display.x - L.menuW));
    L.menuPos.y = ImClamp((display.y - L.menuH) * 0.5f, 0.0f, ImMax(0.0f, display.y - L.menuH));
    return L;
}

// ---------------------------------------------------------------------------
// Sidebar layout - the tab switcher column
// ---------------------------------------------------------------------------
// Two columns: the rail (the hamburger that folds the panel) and the panel with
// the six tab rows. `anim` is the fold animation, 1 = open.
struct SidebarLayout
{
    float railW;     // the hamburger column, always present
    float panelW;    // the tab panel, 0 when folded away
    float rowH;      // one tab row
    float rowGap;    // between two rows
    float topPad;    // above the first row / below the last
    float sidePad;   // left/right inside the panel
    float gap;       // panel -> tab content
    int   rows;      // tab count

    // Height the rows need at their current size, and the width the sidebar
    // takes out of the window (rail + panel + the gap to the content).
    float rowsHeight() const   { return rows * rowH + (rows - 1) * rowGap; }
    float neededHeight() const { return topPad * 2.0f + rowsHeight(); }
    float totalW() const       { return railW + panelW * (panelW > 1.0f ? 1.0f : 0.0f) + gap; }
};

inline SidebarLayout ComputeSidebarLayout(const ImVec2 &hostSize, float anim)
{
    SidebarLayout S{};
    S.railW   = 44.0f;
    S.rowGap  = 6.0f;
    S.topPad  = 8.0f;
    S.sidePad = 6.0f;
    S.rows    = 6;

    // A fifth of the window, clamped, folded away with the animation. On a
    // narrow window (the 320 px minimum menu) the 96 px floor would take more
    // than half of it, so the floor itself scales with the window.
    const float panelMin = ImMin(96.0f, hostSize.x * 0.28f);
    S.panelW = ImClamp(hostSize.x * 0.20f, panelMin, 148.0f) * ImClamp(anim, 0.0f, 1.0f);
    if (S.panelW < 1.0f) S.panelW = 0.0f;
    S.gap = (S.panelW > 0.0f) ? 10.0f : 0.0f;

    // The rows shrink together, down to a 24 px touch target, so all six always
    // fit the window height instead of the last ones falling off the bottom.
    const float availH  = ImMax(1.0f, hostSize.y - S.topPad * 2.0f);
    const float maxRowH = (availH - (float)(S.rows - 1) * S.rowGap) / (float)S.rows;
    S.rowH = ImClamp(maxRowH, 24.0f, 42.0f);

    return S;
}
