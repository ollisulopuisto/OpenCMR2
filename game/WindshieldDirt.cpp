// Glue between the windshield dirt map (WindshieldDirt.h) and the game:
// per-frame accumulation/wetness/wiping state and the cockpit-only overlay
// draw. The overlay replaces the old flat full-screen tint and the white
// 1-pixel wiper bar: dirt shows where it actually is, and the clearing arc
// tracks the cockpit model's real blade sweep (g_carWiperStates + the CIN
// profile's max angles, the same values CarInterior_AnimateWipers uses), so
// the glass clears exactly under the model's blades. Drawn through the TL
// path (both renderers, no new shader). Render-only.
#include <string.h>
#include "port/gfx.h"
#include "Graphics.h"
#include "CarInterior.h"
#include "CarParts.h"
#include "StageBlock.h"
#include "WindshieldDirt.h"
#include "GraphicsOptions.h"
#include "port/sys.h"

WindshieldMap g_windshieldMaps[WINDSHIELD_MAX_VIEWS];

int StageObject_GetViewWeatherField54(int index);
CarPartSet *StageTiming_GetCarReplayRecord(int index);

// Blade geometry in overlay uv: pivots below the dash line, a symmetric pair.
// The models sweep their own nodes about their own pivots; this overlay
// tracks their normalised angle, not their exact pivot - documented
// approximation, the per-model pivot projection is a native-asset follow-up.
static const float kPivotU[2] = { 0.34f, 0.66f };
static const float kPivotV = 1.02f;
static const float kBladeRadius = 0.72f;
static const float kBladeHalfWidth = 0.05f;   // radians
static const float kBladeMaxAngle = 1.15f;    // radians of sweep at full arc

// Per-frame update for one view's map. `slot` is the cockpit slot (the wiper
// state and CIN profile tables hold two: one per split-screen cockpit view).
void Windshield_UpdateView(int view, int slot)
{
    static int frameCounter = 0;
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int rain = StageObject_GetViewWeatherField54(slot);   // fixed, ~0..0x10000
    Windshield_UpdateWetness(m, rain * 255 / 0x8000);
    Windshield_Decay(m, frameCounter++);

    // Damage-linked cracks (video.windshield_cracks): follow the car's real
    // windscreen break state, so the cockpit glass shows what the exterior
    // model already shows. slot == local car index (same mapping the wiper
    // state uses). Break slot 0 is the first window in the CIN glass list;
    // front-vs-rear order of slots 0/1 is a documented assumption pending
    // native confirmation. Independent of the wiper toggle.
    if (Sys_GetOption("video.windshield_cracks", 1) != 0 && slot >= 0 && slot <= 1) {
        CarPartSet *parts = StageTiming_GetCarReplayRecord(slot);
        if (parts != NULL && (parts->partBroken[0] != 0 || parts->partDamaged[0] != 0))
            Windshield_StampCracks(m, parts->partBroken[0] != 0);
    }

    if (Sys_GetOption("video.windscreen_wipers", 1) == 0 || slot < 0 || slot > 1)
        return;   // option off: the model's blades park; glass keeps its dirt
    const CarWiperState *w = &g_carWiperStates[slot];
    short *pMax = g_carInteriorProfilePointers[slot].firstWiperMaxAngle;
    if (w->mode == 0 || w->angle == 0 || pMax == NULL || *pMax <= 0)
        return;   // parked or no profile: no clearing
    float t = (float)w->angle / (float)*pMax;   // both blades share this t
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int reversed = g_carWiperDirectionReversed[slot] != 0;
    for (int b = 0; b < 2; b++) {
        // Blades park pointing up; they sweep out toward their pillar.
        float a = (b == 0 ? -1.0f : 1.0f) * t * kBladeMaxAngle;
        if (reversed) a = -a;
        int dir = (w->sweepingForward ? 1 : -1) * (a >= 0.0f ? 1 : -1);
        if (dir == 0) dir = 1;
        Windshield_Wipe(m, kPivotU[b], kPivotV, kBladeRadius, a, kBladeHalfWidth, dir);
    }
}

// Adds driving spatters to a view's map (rain spray, mud, dust). The amount
// follows the old global-tint scaling; convert it into a few localised drops.
void Windshield_AddDrivingSpatter(int view, int amount)
{
    if (amount <= 0) return;
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int drops = 1 + amount / 24;
    if (drops > 6) drops = 6;
    Windshield_AddSpatter(m, drops, 40 + amount, amount / 255.0f);
}

// ---- overlay draw ---------------------------------------------------------

static GfxTexture *g_windshieldTex;

extern Graphics *g_pGraphics;

// Draws the dirt map over the cockpit view. Called from the per-view race
// draw only when the view's camera is the cockpit (type 3); the HUD draws
// after this and stays clean. Alpha blend, the old tint's grime brown.
void Windshield_DrawOverlay(int view, int resX, int resY)
{
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    if (Windshield_TotalDirt(m) == 0 && Windshield_TotalCracks(m) == 0)
        return;
    if (g_windshieldTex == NULL) {
        g_windshieldTex = Gfx_CreateTexture(WINDSHIELD_MAP_W, WINDSHIELD_MAP_H, GFX_FORMAT_BGRA8, 1, 0);
        if (g_windshieldTex == NULL) return;
    }
    GfxLockedRect lr;
    if (!Gfx_LockTexture(g_windshieldTex, 0, 0, &lr)) return;
    for (int y = 0; y < WINDSHIELD_MAP_H; y++) {
        BYTE *row = (BYTE *)lr.pixels + y * lr.pitch;
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            int d = m->dirt[y * WINDSHIELD_MAP_W + x];
            int c = m->crack[y * WINDSHIELD_MAP_W + x];
            // Cracks catch the light: lerp the grime toward bright glass by
            // the crack mark, alpha combines both layers (standard over).
            row[x * 4 + 0] = (BYTE)(0x20 + (0xe0 - 0x20) * c / 255);   // B
            row[x * 4 + 1] = (BYTE)(0x28 + (0xe8 - 0x28) * c / 255);   // G
            row[x * 4 + 2] = (BYTE)(0x30 + (0xf0 - 0x30) * c / 255);   // R
            int a = 255 - (255 - (d > 224 ? 224 : d)) * (255 - c) / 255;
            row[x * 4 + 3] = (BYTE)(a > 235 ? 235 : a);
        }
    }
    Gfx_UnlockTexture(g_windshieldTex, 0, 0);

    Gfx_SetTexture(0, g_windshieldTex);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetDepthWrite(FALSE);
    GfxTLVertex v[4];
    for (int i = 0; i < 4; i++) {
        v[i].sz = 0; v[i].rhw = 1.0f;
        v[i].color = 0xffffffff; v[i].specular = 0;
    }
    v[0].sx = 0;           v[0].sy = 0;           v[0].tu = 0; v[0].tv = 0;
    v[1].sx = (float)resX; v[1].sy = 0;           v[1].tu = 1; v[1].tv = 0;
    v[2].sx = 0;           v[2].sy = (float)resY; v[2].tu = 0; v[2].tv = 1;
    v[3].sx = (float)resX; v[3].sy = (float)resY; v[3].tu = 1; v[3].tv = 1;
    Gfx_DrawPrimitiveTL(GFX_TRIANGLESTRIP, v, 4);
    Gfx_SetDepthWrite(TRUE);
    Gfx_SetAlphaBlend(FALSE);
    Gfx_SetTexture(0, NULL);
}

void Windshield_ResetAll(void)
{
    for (int i = 0; i < WINDSHIELD_MAX_VIEWS; i++)
        Windshield_Reset(&g_windshieldMaps[i], 0x51ed269bu + (uint32_t)i * 0x9e37u);
}
