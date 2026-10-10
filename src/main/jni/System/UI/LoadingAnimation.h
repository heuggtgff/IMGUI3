#include "ImGui/Call_ImGui.h"
#include "../../ImGui/imgui_settings.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cfloat>

extern ImFont* F50;
namespace font { extern ImFont* inter_semibold; }

namespace ui_loading {

struct Stream   { float x, y, speed; uint32_t seed; int len; };
struct Particle { float x, y, vx, vy, size, life, maxLife; };

static bool  g_Active   = false;
static float g_Timer    = 0.0f;
static float g_Duration = 7.0f;
static float g_Time     = 0.0f;
static bool  g_Inited   = false;

static constexpr int   kStreams   = 8;
static constexpr int   kStreamLen = 6;
static constexpr int   kParticles = 30;
static constexpr int   kBars      = 20;
static constexpr float kCellH     = 13.0f;

static Stream   g_Streams[kStreams];
static Particle g_Parts[kParticles];
static float    g_RingRot[4]  = {0.0f, 0.0f, 0.0f, 0.0f};
static bool     g_RingLock[4] = {false, false, false, false};

namespace C {
    // --- PURPLE THEME ---
    static const ImVec4 Purple     = {0.65f, 0.35f, 1.00f, 1.000f};
    static const ImVec4 PurpleBrt  = {0.80f, 0.50f, 1.00f, 1.000f};
    static const ImVec4 PurpleDeep = {0.45f, 0.20f, 0.80f, 1.000f};
    static const ImVec4 White      = {0.961f, 0.941f, 0.882f, 1.000f};
    static const ImVec4 TextDim    = {0.600f, 0.550f, 0.700f, 1.000f};
    static const ImVec4 BgDeep     = {0.040f, 0.010f, 0.080f, 1.000f};
    static const ImVec4 BgPanel    = {0.080f, 0.030f, 0.150f, 1.000f};
    static const ImVec4 BgCard     = {0.120f, 0.050f, 0.220f, 1.000f};
    static const ImVec4 Border     = {0.650f, 0.350f, 1.000f, 0.600f};
    
    static ImU32 U(ImVec4 c, float a = 1.0f) { return IM_COL32((int)(c.x*255),(int)(c.y*255),(int)(c.z*255),(int)(c.w*a*255)); }
    static ImU32 UA(ImVec4 c, float a)        { return IM_COL32((int)(c.x*255),(int)(c.y*255),(int)(c.z*255),(int)(a*255)); }
}

static const char* kHex  = "0123456789ABCDEF";
static const char* kHash = "DARMODZ LOADED SUCESSFULLY";

static uint32_t H32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x;
}
static float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
static float EaseOut(float t) { t = Clamp01(t); float i = 1.0f - t; return 1.0f - i*i*i; }
static float Pulse(float t, float f) { return std::sin(t * f * 6.2831853f) * 0.5f + 0.5f; }
static float Frac01(float v) { float f = v - std::floor(v); return f < 0.0f ? f + 1.0f : f; }

static void PolyRing(ImDrawList* dl, ImVec2 c, float r, float rot,
                     int sides, ImU32 col, float th) {
    float step = 6.2831853f / (float)sides;
    ImVec2 prev = {c.x + std::cos(rot)*r, c.y + std::sin(rot)*r};
    for (int i = 1; i <= sides; i++) {
        float a = rot + i * step;
        ImVec2 next = {c.x + std::cos(a)*r, c.y + std::sin(a)*r};
        dl->AddLine(prev, next, col, th);
        prev = next;
    }
}

static void StarRing(ImDrawList* dl, ImVec2 c, float r1, float r2, float rot,
                     int points, ImU32 col, float th) {
    int total = points * 2;
    float step = 6.2831853f / (float)total;
    for (int i = 0; i < total; i++) {
        float a0 = rot + i * step, a1 = rot + (i+1) * step;
        float R0 = (i % 2 == 0) ? r1 : r2;
        float R1 = (i % 2 == 0) ? r2 : r1;
        dl->AddLine({c.x + std::cos(a0)*R0, c.y + std::sin(a0)*R0},
                    {c.x + std::cos(a1)*R1, c.y + std::sin(a1)*R1}, col, th);
    }
}

static void RuneTicks(ImDrawList* dl, ImVec2 c, float r, float rot,
                      int count, int majorEvery, ImU32 colMaj, ImU32 colMin) {
    float step = 6.2831853f / (float)count;
    for (int i = 0; i < count; i++) {
        float a = rot + i * step;
        bool  major = (i % majorEvery == 0);
        float inner = r + 2.0f;
        float outer = r + (major ? 11.0f : 5.0f);
        float ca = std::cos(a), sa = std::sin(a);
        dl->AddLine({c.x + ca*inner, c.y + sa*inner},
                    {c.x + ca*outer, c.y + sa*outer},
                    major ? colMaj : colMin, major ? 1.8f : 0.9f);
        if (major)
            dl->AddCircleFilled({c.x + ca*(outer + 3.5f), c.y + sa*(outer + 3.5f)},
                                2.0f, colMaj, 4);
    }
}

static void DrawClover(ImDrawList* dl, ImVec2 c, float r,
                       ImU32 fillCol, ImU32 lineCol, float rot) {
    float leafR  = r * 0.50f;
    float offset = r * 0.46f;
    for (int i = 0; i < 4; i++) {
        float a = rot + i * 1.5707963f;
        ImVec2 lc = {c.x + std::cos(a)*offset, c.y + std::sin(a)*offset};
        dl->AddCircleFilled(lc, leafR, fillCol, 32);
    }
    dl->AddCircleFilled(c, r * 0.25f, fillCol, 24);
    for (int i = 0; i < 4; i++) {
        float a = rot + i * 1.5707963f;
        ImVec2 lc = {c.x + std::cos(a)*offset, c.y + std::sin(a)*offset};
        dl->AddCircle(lc, leafR, lineCol, 32, 1.4f);
        dl->AddLine(c, {c.x + std::cos(a)*(offset + leafR*0.45f),
                        c.y + std::sin(a)*(offset + leafR*0.45f)},
                    lineCol, 0.9f);
    }
}

static void GrimoireBrackets(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float alpha) {
    const float L = 22.0f, t = 1.8f;
    ImU32 cg = C::UA(C::Purple,    alpha);
    ImU32 cb = C::UA(C::PurpleBrt, alpha * 0.55f);
    dl->AddLine(mn,            {mn.x+L, mn.y},   cg, t);
    dl->AddLine(mn,            {mn.x, mn.y+L},   cg, t);
    dl->AddLine({mx.x-L,mn.y}, {mx.x, mn.y},     cg, t);
    dl->AddLine({mx.x, mn.y},  {mx.x, mn.y+L},   cg, t);
    dl->AddLine({mn.x,mx.y-L}, {mn.x, mx.y},     cg, t);
    dl->AddLine({mn.x, mx.y},  {mn.x+L, mx.y},   cg, t);
    dl->AddLine({mx.x,mx.y-L}, {mx.x, mx.y},     cg, t);
    dl->AddLine({mx.x-L,mx.y}, {mx.x, mx.y},     cg, t);
    const float d = 4.0f;
    ImVec2 corners[4] = {mn, {mx.x, mn.y}, {mn.x, mx.y}, mx};
    for (auto& cor : corners) {
        dl->AddCircleFilled(cor, d, cb, 4);
        dl->AddCircle(cor, d + 2.2f, cg, 4, 1.0f);
    }
    float mxH = (mn.x + mx.x) * 0.5f, myH = (mn.y + mx.y) * 0.5f;
    dl->AddLine({mxH-8.f, mn.y}, {mxH+8.f, mn.y}, C::UA(C::Purple, alpha*0.35f), 1.0f);
    dl->AddLine({mxH-8.f, mx.y}, {mxH+8.f, mx.y}, C::UA(C::Purple, alpha*0.35f), 1.0f);
    dl->AddLine({mn.x, myH-8.f}, {mn.x, myH+8.f}, C::UA(C::Purple, alpha*0.35f), 1.0f);
    dl->AddLine({mx.x, myH-8.f}, {mx.x, myH+8.f}, C::UA(C::Purple, alpha*0.35f), 1.0f);
}

inline void Start(float durationSeconds = 7.0f) {
    g_Active   = true;
    g_Timer    = 0.0f;
    g_Time     = 0.0f;
    g_Duration = durationSeconds < 2.0f ? 2.0f : durationSeconds;
    g_Inited   = false;
    for (int i = 0; i < 4; i++) { g_RingRot[i] = 0.0f; g_RingLock[i] = false; }
}
inline bool  IsActive()    { return g_Active; }
inline float GetProgress() { return g_Duration <= 0.0f ? 1.0f : Clamp01(g_Timer / g_Duration); }

inline bool RenderWindow(ImTextureID tex = nullptr) {
    (void)tex;
    if (!g_Active) return false;

    ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    g_Time  += dt;
    g_Timer += dt;
    const float progress = GetProgress();

    const ImVec2 vc = ImGui::GetMainViewport()->GetCenter();
    float pw = io.DisplaySize.x - 40.0f;
    pw = pw < 640.0f ? 640.0f : (pw > 760.0f ? 760.0f : pw);
    float ph = io.DisplaySize.y - 60.0f;
    ph = ph < 380.0f ? 380.0f : (ph > 460.0f ? 460.0f : ph);
    ImGui::SetNextWindowPos(vc, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({pw, ph}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    bool finished = false;
    if (ImGui::Begin("##grimoire_loader", nullptr,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize        | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove))
    {
        ImVec2 wp = ImGui::GetWindowPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float rnd = 14.0f;
        ImVec2 mn = wp, mx = {wp.x + pw, wp.y + ph};
        const float padI = 10.0f;
        ImVec2 inn = {mn.x + padI, mn.y + padI}, inx = {mx.x - padI, mx.y - padI};
        const float innerW = inx.x - inn.x, innerH = inx.y - inn.y;
        const float leftW  = innerW * 0.40f;
        const float divX   = inn.x + leftW;
        const float rightX = divX + 22.0f, rightEnd = inx.x - 18.0f;

        dl->AddRectFilled(mn, mx, C::U(C::BgDeep), rnd);
        dl->AddRectFilled(inn, inx, C::U(C::BgPanel), rnd - 4.0f);

        dl->AddRectFilledMultiColor(inn, {inx.x, inn.y + innerH*0.38f},
            C::UA(C::PurpleDeep, 0.20f), C::UA(C::PurpleDeep, 0.20f),
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,0));

        dl->AddRectFilledMultiColor(inn, {inn.x + 28.0f, inx.y},
            IM_COL32(0,0,0,80), IM_COL32(0,0,0,0), IM_COL32(0,0,0,0), IM_COL32(0,0,0,80));
        dl->AddRectFilledMultiColor({inx.x - 28.0f, inn.y}, inx,
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,80), IM_COL32(0,0,0,80), IM_COL32(0,0,0,0));
        dl->AddRectFilledMultiColor({inn.x, inx.y - 28.0f}, inx,
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,0), IM_COL32(0,0,0,80), IM_COL32(0,0,0,80));

        dl->AddRectFilled(inn, {inx.x, inn.y + 2.5f},
                          C::U(C::Purple), 2.0f, ImDrawFlags_RoundCornersTop);

        dl->AddRect(inn, inx, C::UA(C::Border, 0.90f), rnd - 4.0f, 0, 1.2f);
        dl->AddRect({inn.x+3.5f, inn.y+3.5f}, {inx.x-3.5f, inx.y-3.5f},
                    C::UA(C::Border, 0.28f), rnd - 7.0f, 0, 0.8f);

        GrimoireBrackets(dl, inn, inx, 0.42f + 0.14f * Pulse(g_Time, 0.55f));

        float scanY = inn.y + Frac01(g_Time * 0.10f) * innerH;
        dl->AddRectFilledMultiColor({inn.x, scanY - 8.0f}, {inx.x, scanY + 8.0f},
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,0),
            C::UA(C::Purple, 0.08f), C::UA(C::Purple, 0.08f));
        dl->AddLine({inn.x, scanY}, {inx.x, scanY}, C::UA(C::Purple, 0.18f), 1.0f);

        const float leftAx = inn.x + 10.0f, leftAw = leftW - 20.0f;
        const float sTop = inn.y + 12.0f, sBot = inx.y - 12.0f;
        const float leftCx = inn.x + leftW * 0.5f;
        float sealR = leftW * 0.40f;
        if (sealR > innerH * 0.20f) sealR = innerH * 0.20f;
        const float sealCy = inn.y + innerH * 0.33f;

        if (!g_Inited) {
            g_Inited = true;
            uint32_t s = 0x9e3779b1u;
            for (int i = 0; i < kStreams; i++) {
                s = H32(s + (uint32_t)i * 2654435761u);
                g_Streams[i].x     = leftAx + (i + 0.5f) * leftAw / (float)kStreams;
                g_Streams[i].y     = sTop + (H32(s) & 0xFFFFu) / 65535.0f * (sBot - sTop);
                g_Streams[i].speed = 14.0f + (H32(s >> 3) & 0xFFu) / 255.0f * 22.0f;
                g_Streams[i].seed  = H32(s >> 5);
                g_Streams[i].len   = kStreamLen;
            }
            for (int i = 0; i < kParticles; i++) {
                s = H32(s + (uint32_t)i * 40503u + 1u);
                g_Parts[i].x       = inn.x + (H32(s) & 0xFFFFu) / 65535.0f * innerW;
                g_Parts[i].y       = inn.y + (H32(s >> 4) & 0xFFFFu) / 65535.0f * innerH;
                g_Parts[i].vx      = -2.0f + (H32(s >> 7) & 0xFFu) / 255.0f * 4.0f;
                g_Parts[i].vy      = -(4.0f + (H32(s >> 9) & 0xFFu) / 255.0f * 10.0f);
                g_Parts[i].size    = 0.9f + (H32(s >> 11) & 0xFFu) / 255.0f * 1.4f;
                g_Parts[i].life    = (H32(s >> 13) & 0xFFFFu) / 65535.0f * 4.0f;
                g_Parts[i].maxLife = 2.5f + (H32(s >> 15) & 0xFFu) / 255.0f * 3.5f;
            }
        }

        for (int i = 0; i < kParticles; i++) {
            Particle& p = g_Parts[i];
            p.x += p.vx * dt; p.y += p.vy * dt; p.life += dt;
            if (p.life >= p.maxLife || p.y < inn.y) {
                uint32_t h = H32((uint32_t)i * 2246822519u ^ (uint32_t)(g_Time * 53.0f));
                p.x = inn.x + (h & 0xFFFFu) / 65535.0f * innerW;
                p.y = inx.y - (H32(h >> 6) & 0xFFu) / 255.0f * 14.0f;
                p.vy = -(4.0f + (H32(h >> 9) & 0xFFu) / 255.0f * 10.0f);
                p.vx = -2.0f + (H32(h >> 12) & 0xFFu) / 255.0f * 4.0f;
                p.life = 0.0f;
                p.maxLife = 2.5f + (H32(h >> 15) & 0xFFu) / 255.0f * 3.5f;
            }
            float a = p.life / p.maxLife;
            float fade = (a < 0.2f) ? (a / 0.2f) : (1.0f - (a - 0.2f) / 0.8f);
            dl->AddCircleFilled({p.x, p.y}, p.size * 0.6f, C::UA(C::PurpleBrt, fade * 0.55f), 4);
            dl->AddCircleFilled({p.x, p.y}, p.size,        C::UA(C::Purple,    fade * 0.20f), 6);
        }

        ImFont* body = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
        ImFont* disp = F50 ? F50 : body;

        for (int i = 0; i < kStreams; i++) {
            Stream& st = g_Streams[i];
            st.y += st.speed * dt;
            if (st.y - st.len * kCellH > sBot) {
                st.y = sTop - (H32(st.seed ^ (uint32_t)g_Time) & 0xFFu) / 255.0f * 40.0f;
                st.seed = H32(st.seed);
            }
            for (int j = 0; j < st.len; j++) {
                float gy = st.y - j * kCellH;
                if (gy < sTop - kCellH || gy > sBot) continue;
                uint32_t h = H32(st.seed + (uint32_t)j * 131u + (uint32_t)(gy * 0.3f));
                char ch = kHex[h & 15u];
                float a = 1.0f - (float)j / (float)st.len;
                ImU32 col = (j == 0)
                    ? C::UA(C::White,    0.72f * a)
                    : C::UA(C::PurpleDeep, 0.50f * a * a);
                dl->AddText(body, 12.0f, {st.x - 3.0f, gy}, col, &ch, &ch + 1);
            }
        }

        dl->AddCircleFilled({leftCx, sealCy}, sealR * 1.70f, C::UA(C::PurpleDeep, 0.15f), 64);
        dl->AddCircleFilled({leftCx, sealCy}, sealR * 1.10f, C::UA(C::PurpleDeep, 0.25f), 48);

        float auraPulse = 0.88f + 0.12f * Pulse(g_Time, 0.65f);
        dl->AddCircle({leftCx, sealCy}, sealR * 1.60f * auraPulse,
                      C::UA(C::Purple, 0.10f * auraPulse), 64, 1.0f);

        const float beamH = sealR * 1.85f;
        dl->AddRectFilledMultiColor(
            {leftCx - 1.0f, sealCy - beamH}, {leftCx + 1.0f, sealCy},
            C::UA(C::Purple, 0.0f), C::UA(C::Purple, 0.0f),
            C::UA(C::PurpleBrt, 0.32f), C::UA(C::PurpleBrt, 0.32f));
        dl->AddRectFilledMultiColor(
            {leftCx - 1.0f, sealCy}, {leftCx + 1.0f, sealCy + beamH},
            C::UA(C::PurpleBrt, 0.32f), C::UA(C::PurpleBrt, 0.32f),
            C::UA(C::Purple, 0.0f), C::UA(C::Purple, 0.0f));
        dl->AddRectFilledMultiColor(
            {leftCx - beamH, sealCy - 1.0f}, {leftCx, sealCy + 1.0f},
            C::UA(C::Purple, 0.0f), C::UA(C::PurpleBrt, 0.22f),
            C::UA(C::PurpleBrt, 0.22f), C::UA(C::Purple, 0.0f));
        dl->AddRectFilledMultiColor(
            {leftCx, sealCy - 1.0f}, {leftCx + beamH, sealCy + 1.0f},
            C::UA(C::PurpleBrt, 0.22f), C::UA(C::Purple, 0.0f),
            C::UA(C::Purple, 0.0f), C::UA(C::PurpleBrt, 0.22f));

        static const float lockAt[4]  = {0.25f, 0.50f, 0.75f, 0.95f};
        static const float ringSpd[4] = {0.90f, -0.65f, 0.45f, -0.28f};
        static const float rScale[4]  = {1.00f,  0.76f, 0.56f,  0.40f};
        static const int   snapSides[4] = {8, 12, 5, 4};

        for (int i = 0; i < 4; i++) {
            if (progress >= lockAt[i]) g_RingLock[i] = true;
            if (!g_RingLock[i]) {
                g_RingRot[i] += dt * ringSpd[i];
            } else {
                float snapStep = 6.2831853f / (float)snapSides[i];
                float snap = std::floor(g_RingRot[i] / snapStep + 0.5f) * snapStep;
                float k = dt * 8.0f; if (k > 1.0f) k = 1.0f;
                g_RingRot[i] += (snap - g_RingRot[i]) * k;
            }

            float glow = g_RingLock[i]
                ? (0.82f + 0.18f * Pulse(g_Time, 1.4f))
                : (0.38f + 0.16f * Pulse(g_Time, 0.8f + i*0.25f));
            ImU32 col = g_RingLock[i] ? C::UA(C::PurpleBrt, glow) : C::UA(C::Purple, glow);
            float th  = g_RingLock[i] ? 2.0f : 1.2f;
            float r   = sealR * rScale[i];

            switch (i) {
                case 0: PolyRing(dl, {leftCx, sealCy}, r, g_RingRot[i], 8, col, th); break;
                case 1: StarRing(dl, {leftCx, sealCy}, r, r*0.62f, g_RingRot[i], 6, col, th); break;
                case 2: PolyRing(dl, {leftCx, sealCy}, r, g_RingRot[i], 5, col, th); break;
                case 3: PolyRing(dl, {leftCx, sealCy}, r, g_RingRot[i] + 0.7853982f, 4, col, th); break;
            }
        }

        RuneTicks(dl, {leftCx, sealCy}, sealR * 1.15f, g_Time * 0.06f,
                  48, 6, C::UA(C::Purple, 0.68f), C::UA(C::PurpleDeep, 0.36f));

        float cloverR   = sealR * 0.30f;
        float cloverRot = g_Time * 0.22f;
        float cloverP   = 0.80f + 0.20f * Pulse(g_Time, 1.55f);
        DrawClover(dl, {leftCx, sealCy},
                   cloverR * cloverP,
                   C::UA(C::PurpleDeep, 0.78f),
                   C::UA(C::PurpleBrt,  0.92f),
                   cloverRot);
        dl->AddCircleFilled({leftCx, sealCy}, cloverR * 0.22f * cloverP,
                            C::UA(C::White, 0.92f), 12);
        dl->AddCircle({leftCx, sealCy}, cloverR * 0.42f * cloverP,
                      C::UA(C::PurpleBrt, 0.52f * cloverP), 12, 1.2f);

        const float eqY = inx.y - 40.0f, eqH = 22.0f;
        const float eqW = leftAw * 0.80f, eqX0 = leftCx - eqW * 0.5f;
        const float bw = (eqW / (float)kBars) * 0.55f, gap = eqW / (float)kBars;
        for (int i = 0; i < kBars; i++) {
            float n = std::fabs(std::sin(g_Time * 3.5f + i * 0.58f)) * 0.65f + 0.35f;
            n *= (0.25f + 0.75f * progress);
            float h = 3.0f + n * eqH;
            float bx = eqX0 + i * gap + (gap - bw) * 0.5f;
            dl->AddRectFilledMultiColor(
                {bx, eqY + eqH - h}, {bx + bw, eqY + eqH},
                C::UA(C::White, 0.80f), C::UA(C::White, 0.80f),
                C::UA(C::Purple,  0.60f), C::UA(C::Purple,  0.60f));
        }

        char pctBuf[8];
        std::snprintf(pctBuf, sizeof(pctBuf), "%d", (int)(progress * 100.0f + 0.5f));
        float pctSz = disp->CalcTextSizeA(40.0f, FLT_MAX, 0.0f, pctBuf).x;
        dl->AddText(disp, 40.0f, {leftCx - pctSz*0.5f, sealCy + sealR + 18.0f},
                    C::U(C::White), pctBuf);
        const char* gaugeLbl = "MANA";
        float glSz = body->CalcTextSizeA(10.0f, FLT_MAX, 0.0f, gaugeLbl).x;
        dl->AddText(body, 10.0f, {leftCx - glSz*0.5f, sealCy + sealR + 62.0f},
                    C::UA(C::TextDim, 0.85f), gaugeLbl);

        dl->AddLine({divX, inn.y + 22.0f}, {divX, inx.y - 22.0f},
                    C::UA(C::Border, 0.65f), 1.0f);
        float divMid = (inn.y + inx.y) * 0.5f;
        dl->AddCircleFilled({divX, divMid}, 3.5f, C::UA(C::Purple, 0.55f), 4);

        float cy = inn.y + 24.0f;
        {
            const char* p1 = "DARMODZ";
            const char* p2 = " V3";
            float s1 = disp->CalcTextSizeA(26.0f, FLT_MAX, 0.0f, p1).x;
            float s2 = disp->CalcTextSizeA(26.0f, FLT_MAX, 0.0f, p2).x;
            float tx = rightX + ((rightEnd - rightX) - s1 - s2) * 0.5f;
            dl->AddText(disp, 26.0f, {tx, cy},      C::U(C::White), p1);
            dl->AddText(disp, 26.0f, {tx + s1, cy}, C::U(C::Purple),  p2);
            cy += 30.0f;
            const char* sub = "Call of Duty: 1.6.57";
            float ss = body->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, sub).x;
            dl->AddText(body, 12.0f,
                        {rightX + ((rightEnd - rightX) - ss) * 0.5f, cy},
                        C::UA(C::TextDim, 0.90f), sub);
            cy += 24.0f;
        }

        {
            const char* prompt = "Protection Activating";
            dl->AddText(body, 11.0f, {rightX, cy}, C::UA(C::Purple, 0.72f), prompt);
            cy += 20.0f;
        }

        struct LogLine { const char* text; float a; float b; };
        static const LogLine lines[8] = {
            { "Establishing secure channel",     0.00f, 0.14f },
            { "Validating license token",        0.12f, 0.30f },
            { "Handshake acknowledged",          0.28f, 0.42f },
            { "Mapping module table",            0.40f, 0.56f },
            { "Synchronizing runtime hooks",     0.54f, 0.72f },
            { "Verifying integrity seal",        0.70f, 0.86f },
            { "Compositing interface layer",     0.84f, 0.96f },
            { "Bootstrap complete",              0.95f, 1.00f },
        };
        const float lineH = 19.0f;
        bool curOn = (std::fmod(g_Time * 2.0f, 1.0f) < 0.5f);

        for (int i = 0; i < 8; i++) {
            float ly = cy + i * lineH;
            ImVec2 dot = {rightX + 6.0f, ly + lineH * 0.5f};
            const char* txt = lines[i].text;
            int len = 0; while (txt[len]) len++;

            if (progress >= lines[i].b) {
                dl->AddCircleFilled(dot, 3.4f, C::UA(C::Purple,    0.90f), 4);
                dl->AddCircle     (dot, 6.0f,  C::UA(C::PurpleDeep, 0.50f), 4, 1.2f);
                dl->AddText(body, 12.5f, {rightX + 18.0f, ly + 2.0f},
                            C::UA(C::White, 0.90f), txt);
            } else if (progress >= lines[i].a) {
                float lp = Clamp01((progress - lines[i].a) / (lines[i].b - lines[i].a));
                float pr = 0.70f + 0.30f * Pulse(g_Time, 2.0f);
                dl->AddCircle     (dot, 4.2f * pr, C::UA(C::Purple,    pr),        12, 1.4f);
                dl->AddCircleFilled(dot, 2.0f,      C::UA(C::PurpleBrt, 0.80f * pr), 8);
                int shown = (int)(lp * (float)len);
                dl->AddText(body, 12.5f, {rightX + 18.0f, ly + 2.0f},
                            C::U(C::White), txt, txt + shown);
                if (curOn) {
                    float tw = body->CalcTextSizeA(12.5f, FLT_MAX, 0.0f, txt, txt + shown).x;
                    dl->AddRectFilled(
                        {rightX + 18.0f + tw + 1.0f, ly + 3.0f},
                        {rightX + 18.0f + tw + 7.0f, ly + 13.0f},
                        C::UA(C::Purple, 0.90f));
                }
            } else {
                dl->AddCircle(dot, 3.0f, C::UA(C::Border, 0.80f), 4, 1.2f);
                dl->AddText(body, 12.5f, {rightX + 18.0f, ly + 2.0f},
                            C::UA(C::TextDim, 0.50f), txt);
            }
        }
        cy += 8.0f * lineH + 8.0f;

        {
            const char* tag = "SEAL ";
            dl->AddText(body, 11.0f, {rightX, cy}, C::UA(C::TextDim, 0.80f), tag);
            float tagW = body->CalcTextSizeA(11.0f, FLT_MAX, 0.0f, tag).x;
            int L = 0; while (kHash[L]) L++;
            int revealed = (int)(progress * (float)L);
            uint32_t frameSeed = (uint32_t)(g_Time * 12.0f);
            float hx = rightX + tagW;
            const float adv = 7.6f;
            for (int i = 0; i < L; i++) {
                char ch;
                ImU32 col;
                if (i < revealed) {
                    ch = kHash[i]; col = C::UA(C::White, 0.90f);
                } else {
                    ch = kHex[H32((uint32_t)i ^ frameSeed) & 15u];
                    col = C::UA(C::PurpleDeep, 0.55f);
                }
                dl->AddText(body, 11.0f, {hx + i * adv, cy}, col, &ch, &ch + 1);
            }
            cy += 22.0f;
        }

        const float barH = 8.0f;
        const float barW = rightEnd - rightX;
        dl->AddRectFilled({rightX, cy}, {rightEnd, cy + barH},
                          C::U(C::BgCard), barH * 0.5f);
        dl->AddRect({rightX, cy}, {rightEnd, cy + barH},
                    C::UA(C::Border, 0.65f), barH * 0.5f, 0, 0.8f);

        float fillW = barW * EaseOut(progress);
        if (fillW > 0.5f) {
            dl->AddRectFilledMultiColor(
                {rightX, cy}, {rightX + fillW, cy + barH},
                C::UA(C::PurpleDeep, 0.90f), C::UA(C::PurpleBrt, 0.95f),
                C::UA(C::PurpleBrt,  0.95f), C::UA(C::PurpleDeep, 0.90f));
            dl->AddRectFilledMultiColor(
                {rightX, cy}, {rightX + fillW, cy + barH * 0.45f},
                C::UA(C::White, 0.0f), C::UA(C::White, 0.16f),
                C::UA(C::White, 0.16f), C::UA(C::White, 0.0f));
            dl->AddCircleFilled({rightX + fillW, cy + barH * 0.5f},
                                barH * 0.82f, C::UA(C::PurpleBrt, 0.80f), 12);
        }

        for (float frac : {0.33f, 0.66f}) {
            float mx2 = rightX + barW * frac;
            dl->AddLine({mx2, cy - 2.0f}, {mx2, cy + barH + 2.0f},
                        C::UA(C::Border, 0.55f), 1.0f);
        }

        const char* stage = (progress < 0.34f) ? "Verifying License"
                          : (progress < 0.68f) ? "Syncing Modules"
                          : (progress < 0.95f) ? "Preparing Interface"
                          :                      "Sealed";
        dl->AddText(body, 11.0f, {rightX, cy + barH + 7.0f},
                    C::UA(C::TextDim, 0.90f), stage);
        char pb[8];
        std::snprintf(pb, sizeof(pb), "%d%%", (int)(progress * 100.0f + 0.5f));
        float pbs = body->CalcTextSizeA(11.0f, FLT_MAX, 0.0f, pb).x;
        dl->AddText(body, 11.0f, {rightEnd - pbs, cy + barH + 7.0f},
                    C::U(C::Purple), pb);

        float glowA = 0.05f + 0.04f * Pulse(g_Time, 0.45f);
        dl->AddRect(mn, mx, C::UA(C::Purple,    glowA),        rnd,     0, 4.0f);
        dl->AddRect({mn.x+1.0f, mn.y+1.0f}, {mx.x-1.0f, mx.y-1.0f},
                    C::UA(C::PurpleBrt, glowA * 0.50f), rnd - 1.0f, 0, 2.0f);
    }
    ImGui::End();

    if (g_Timer >= g_Duration) { g_Active = false; g_Timer = 0.0f; finished = true; }
    return finished;
}

}