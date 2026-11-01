#include "touch_overlay.h"

#include "platform/platform.h"
#include "port/gfx.h"
#include "port/input.h"

#include <math.h>
#include <string.h>
#include <string.h>

// ---- recording -------------------------------------------------------------

static GfxTexture *s_ring;
static GfxTexture *s_dot;

static GfxTexture *MakeTexture(void (*fill)(unsigned char *, int), int size)
{
    GfxTexture *texture = Gfx_CreateTexture(size, size, GFX_FORMAT_BGRA8, 1, 0);
    if (texture == NULL)
        return NULL;
    unsigned char *contiguous = new unsigned char[size * size * 4];
    fill(contiguous, size);
    GfxLockedRect locked;
    if (!Gfx_LockTexture(texture, 0, 0, &locked)) {
        delete[] contiguous;
        Gfx_DestroyTexture(texture);
        return NULL;
    }
    for (int y = 0; y < size; y++)
        memcpy((unsigned char *)locked.pixels + y * locked.pitch, contiguous + y * size * 4, size * 4);
    Gfx_UnlockTexture(texture, 0, 0);
    delete[] contiguous;
    return texture;
}

static void DrawSpriteQuad(GfxTexture *texture, float cx, float cy, float rx, float ry, DWORD color)
{
    GfxTLVertex v[4];
    const float uvs[4][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
    const float xs[4] = {cx - rx, cx + rx, cx - rx, cx + rx};
    const float ys[4] = {cy - ry, cy - ry, cy + ry, cy + ry};
    for (int i = 0; i < 4; i++) {
        v[i].sx = xs[i];
        v[i].sy = ys[i];
        v[i].sz = 0.0f;
        v[i].rhw = 1.0f;
        v[i].color = color;
        v[i].specular = 0;
        v[i].tu = uvs[i][0];
        v[i].tv = uvs[i][1];
    }
    const WORD indices[6] = {0, 1, 2, 2, 1, 3};
    Gfx_SetTexture(0, texture);
    Gfx_DrawIndexedPrimitiveTL(GFX_TRIANGLELIST, v, 4, indices, 6);
}

void TouchOverlay_RecordFrame(int contentW, int contentH, bool stretch)
{
    if (!Input_TouchOverlayActive())
        return;
    TouchPadView views[4];
    int windowW = 0, windowH = 0;
    int padCount = Input_GetTouchPads(views, 4, &windowW, &windowH);
    if (padCount == 0)
        return;
    if (windowW <= 0 || windowH <= 0 || contentW <= 0 || contentH <= 0)
        return;
    if (s_ring == NULL)
        s_ring = MakeTexture(TouchOverlay_FillRingBGRA, 128);
    if (s_dot == NULL)
        s_dot = MakeTexture(TouchOverlay_FillDotBGRA, 64);
    if (s_ring == NULL || s_dot == NULL)
        return;

    // Pads arrive in drawable window pixels; the recorded frame lives in the
    // offscreen content buffer and is letterboxed by the executor's final
    // blit, so map through the same presentation rect (see the bar policy in
    // touch_overlay.h) and draw in content space.
    Gfx_PushState();
    Gfx_SetVirtualScreen(contentW, contentH);
    Gfx_SetLighting(0);
    Gfx_SetFog(0);
    Gfx_SetDepthTest(0);
    Gfx_SetDepthWrite(0);
    Gfx_SetAlphaTest(0);
    Gfx_SetAlphaBlend(1);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageAlphaArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageMinFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetStageMagFilter(0, GFX_FILTER_LINEAR);

    // Placeholder look (unsettled design): a faint rim at the anchor and a
    // brighter thumb dot at the live finger position.
    for (int i = 0; i < padCount; i++) {
        const TouchPadView *pad = &views[i];
        if (!pad->active)
            continue;
        // Provisional, configurable bar policy (touch_overlay.h): clamp the
        // visual to the picture edge, or hide the pad while in a bar.
        const char *policy = Platform_GetSetting("input.touch_bar_policy", "clamp");
        bool hide = strcmp(policy, "hide") == 0;
        float anchor[4], current[4];
        int anchorClamped =
            TouchOverlay_MapPadToContent(pad->anchorX, pad->anchorY, pad->radiusX, pad->radiusY, windowW,
                                         windowH, contentW, contentH, stretch, anchor);
        int currentClamped =
            TouchOverlay_MapPadToContent(pad->currentX, pad->currentY, pad->radiusX * 0.4f, pad->radiusY * 0.4f,
                                         windowW, windowH, contentW, contentH, stretch, current);
        if (hide && (anchorClamped || currentClamped))
            continue;
        // Fixed classic pads idle at their region center; the thumb dot only
        // appears while the pad is actually held.
        bool held = pad->pressed != 0;
        DrawSpriteQuad(s_ring, anchor[0], anchor[1], anchor[2], anchor[3], held ? 0x70FFFFFF : 0x40FFFFFF);
        if (held)
            DrawSpriteQuad(s_dot, current[0], current[1], current[2], current[3], 0x90FFFFFF);
    }
    Gfx_PopState();
}
