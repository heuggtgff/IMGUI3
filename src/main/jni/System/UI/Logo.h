#ifndef LOGOSETTINGS_H
#define LOGOSETTINGS_H

static float g_LogoOpacity = 1.0f;
static float g_LogoSizeMultiplier = 1.0f;

inline float GetLogoOpacity()
{
    return g_LogoOpacity;
}

inline void SetLogoOpacity(float opacity)
{
    g_LogoOpacity = opacity;
}

inline float GetLogoSizeMultiplier()
{
    return g_LogoSizeMultiplier;
}

inline void SetLogoSizeMultiplier(float size)
{
    g_LogoSizeMultiplier = size;
}

#endif
