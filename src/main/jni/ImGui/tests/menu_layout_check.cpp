// Host test for ComputeMenuLayout() and ComputeSidebarLayout() - the layout the
// menu window, its new sidebar and the hamburger rail are built from.
//
// The window is drawn in the game's render surface, which on a phone can be
// anything from ~640 px up, so the layout has to come from the display rather
// than from fixed pixels ("the menu is so big"). The tab switcher is now a
// sidebar inside that window plus a rail with the hamburger that folds it, and
// all six tab rows have to fit the window height at any size.
//
// Host test, nothing is linked from ImGui (ImVec2 / ImMax / ImClamp are all
// header-only), so no NDK and no menu runtime is needed:
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -I src/main/jni \
//       src/main/jni/ImGui/tests/menu_layout_check.cpp -o /tmp/menu_layout_check \
//       && /tmp/menu_layout_check
// Android.mk's wildcard for ImGui is $(LOCAL_PATH)/ImGui/*.c*, i.e. not
// recursive, so this file never reaches the APK.
#include <cstdio>

#include "ImGui/menu_layout.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

static void checkDisplay(float w, float h, const char *tag)
{
    const MenuLayout L = ComputeMenuLayout(ImVec2(w, h));
    char buf[256];

    snprintf(buf, sizeof buf, "%s: %gx%g the window is on screen", tag, w, h);
    CHECK(L.menuPos.x >= -0.05f && L.menuPos.y >= -0.05f &&
          L.menuPos.x + L.menuW <= w + 0.05f && L.menuPos.y + L.menuH <= h + 0.05f, buf);

    // Auto-resize: 92% of the display, floored for the two-column tabs and
    // capped by the surface (the 16 px margin).
    snprintf(buf, sizeof buf, "%s: %gx%g the window follows the display", tag, w, h);
    CHECK(L.menuW >= ImMin(360.0f, w) - 0.05f && L.menuH >= ImMin(320.0f, h) - 0.05f &&
          L.menuW <= w + 0.05f && L.menuH <= h + 0.05f, buf);
    snprintf(buf, sizeof buf, "%s: %gx%g the window uses most of the display", tag, w, h);
    CHECK(L.menuW >= w * 0.5f && L.menuH >= h * 0.5f, buf);

    snprintf(buf, sizeof buf, "%s: %gx%g the window is centred", tag, w, h);
    CHECK(L.menuPos.x > 0.0f && L.menuPos.y >= 0.0f, buf);
}

static void checkSidebar(float hostW, float hostH, const char *tag)
{
    char buf[256];

    // Open, fully unfolded.
    const SidebarLayout open = ComputeSidebarLayout(ImVec2(hostW, hostH), 1.0f);
    snprintf(buf, sizeof buf, "%s: %gx%g all six rows fit the window height", tag, hostW, hostH);
    CHECK(open.neededHeight() <= hostH + 0.05f, buf);

    snprintf(buf, sizeof buf, "%s: %gx%g the panel is a usable width and has a rail", tag, hostW, hostH);
    CHECK(open.panelW > 40.0f && open.panelW <= 148.0f &&
          open.railW >= 36.0f && open.railW <= 56.0f && open.gap > 0.0f, buf);

    snprintf(buf, sizeof buf, "%s: %gx%g rows keep a 24 px touch target", tag, hostW, hostH);
    CHECK(open.rowH >= 24.0f && open.rowH <= 42.0f &&
          open.sidePad * 2.0f < open.panelW, buf);

    snprintf(buf, sizeof buf, "%s: %gx%g the sidebar takes less than half the window", tag, hostW, hostH);
    CHECK(open.totalW() < hostW * 0.5f, buf);

    // Folded away: the panel is gone, the rail (hamburger) stays.
    const SidebarLayout folded = ComputeSidebarLayout(ImVec2(hostW, hostH), 0.0f);
    snprintf(buf, sizeof buf, "%s: %gx%g folding leaves only the hamburger rail", tag, hostW, hostH);
    CHECK(folded.panelW == 0.0f && folded.gap == 0.0f &&
          folded.railW == open.railW && folded.neededHeight() <= hostH + 0.05f, buf);

    // Half-way through the fold the panel is between the two.
    const SidebarLayout half = ComputeSidebarLayout(ImVec2(hostW, hostH), 0.5f);
    snprintf(buf, sizeof buf, "%s: %gx%g the fold animates the panel width", tag, hostW, hostH);
    CHECK(half.panelW > 0.0f && half.panelW < open.panelW, buf);
}

int main()
{
    // Landscape and portrait surfaces a phone game reports.
    checkDisplay(1280.0f, 720.0f,  "720p");
    checkDisplay(1920.0f, 1080.0f, "1080p");
    checkDisplay(2340.0f, 1080.0f, "1080p wide");
    checkDisplay(2400.0f, 1080.0f, "1080p 20:9");
    checkDisplay(3120.0f, 1440.0f, "1440p");
    checkDisplay(1080.0f, 2400.0f, "portrait");
    checkDisplay(720.0f,  1280.0f, "portrait small");
    checkDisplay(960.0f,  540.0f,  "540p");
    checkDisplay(640.0f,  480.0f,  "480p");

    // The auto-resize rule itself, on a surface where it is not clamped.
    {
        const MenuLayout big = ComputeMenuLayout(ImVec2(1920.0f, 1080.0f));
        CHECK(ImAbs(big.menuW - 1920.0f * 0.92f) < 0.05f &&
              ImAbs(big.menuH - 1080.0f * 0.92f) < 0.05f,
              "1080p gets the full 92% of the display");
        const MenuLayout small = ComputeMenuLayout(ImVec2(640.0f, 480.0f));
        CHECK(small.menuW < big.menuW && small.menuH < big.menuH,
              "... and a small surface gets a smaller window");
        // 92% of a 300 px surface would be under the floor, and the floor is
        // itself clamped by the margin: the window fills the surface instead of
        // hanging off it.
        const MenuLayout floored = ComputeMenuLayout(ImVec2(300.0f, 260.0f));
        CHECK(floored.menuW == 288.0f && floored.menuH == 248.0f,
              "a tiny surface gets the margin-clamped window, never a bigger one");
    }

    // The inside of the window, i.e. what the sidebar is laid out against:
    // the host area is the window minus the header and the paddings.
    // A finished menu window keeps the full-size panel.
    CHECK(ComputeSidebarLayout(ImVec2(852.0f, 470.0f), 1.0f).panelW == 148.0f,
          "a full-size window gets the widest sidebar panel");

    checkSidebar(852.0f, 470.0f, "big window");
    checkSidebar(700.0f, 380.0f, "medium window");
    checkSidebar(560.0f, 300.0f, "small window");
    checkSidebar(280.0f, 200.0f, "tiny window");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all menu-layout checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
