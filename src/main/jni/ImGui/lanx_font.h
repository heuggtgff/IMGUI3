#pragma once

// ---------------------------------------------------------------------------
// LANX menu fonts (ported from github.com/eliwoahzja/LANX,
// examples/example_win32_directx11/font.h + font_defines.h).
// The data lives in lanx_font.cpp; loading never touches the file system.
// ---------------------------------------------------------------------------

// icomoon codepoints used by the LANX menu/widgets.
#define ICON_DOWN_SMALL_LINE ""
#define ICON_FIRE_FILL ""

// Embedded font payloads (defined in lanx_font.cpp, global scope on purpose so
// the byte arrays are emitted exactly once and stay outside the project's
// font:: / c:: / custom:: namespaces).
extern unsigned char PoppinsSemiBold[155232];                 // Poppins-SemiBold.ttf (exact payload size)
extern const char icomoon_compressed_data_base85[618020 + 1]; // icomoon icons, base85 compressed

namespace lanx
{
    // Adds the LANX font set (Poppins + merged icomoon icons) to the current
    // atlas. Call it once, after the app's own fonts are registered and before
    // io.Fonts->Build().
    void LoadFonts();
}