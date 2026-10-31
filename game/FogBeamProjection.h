#ifndef OPENCMR2_FOG_BEAM_PROJECTION_H
#define OPENCMR2_FOG_BEAM_PROJECTION_H
// The fog pass's car lights (video.fog_lights): the ONE production source that
// projects this step's recorded lights (LightField) to the screen, keeping
// their view depth for the shader's depth gate. Called from StageObject's
// per-view draw; tests/fog_beam_render.cpp drives the same code into the real
// fog.frag through the GL executor. Render-only.
#include "FixedPoint.h"
#include "SceneNode.h"
#include "LightField.h"
#include "port/gfx.h"

void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView);

inline int FogBeam_ProjectLights(GfxFogLight *fogLights, int maxLights, SceneNode *view, float resXf, float resYf)
{
    int fogLightTotal = 0;
    FixVector vOut;
    FixMatrix invView;
    FixMatrix_Invert(&invView, &view->world);
    const float aspect = resYf > 0.0f ? resXf / resYf : 1.0f;
    for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX && fogLightTotal < maxLights; li++) {
        FogLightSource *src = &g_fogLightSources[li];
        FixVector vView;
        FixMatrix_TransformPoint(&vView, &src->pos, &invView);
        if (vView.z < 0x10000)
            continue;   // behind the camera
        FixMatrix_ProjectWorldPointToView((int *)&vOut, &src->pos, view);
        const float lampU = (float)(vOut.x >> 16) / resXf, lampV = (float)(vOut.y >> 16) / resYf;
        // The beam's screen wedge: project a point 30 world units along the
        // beam. Off-screen lamps stay: their wedge can still cross the frame
        // (the cockpit's own lamps sit below the windscreen line).
        FixVector beamEnd;
        beamEnd.x = src->pos.x + FixMul(src->dir.x, 0x1e0000);
        beamEnd.y = src->pos.y + FixMul(src->dir.y, 0x1e0000);
        beamEnd.z = src->pos.z + FixMul(src->dir.z, 0x1e0000);
        float endU = lampU, endV = lampV;
        if (src->dir.x != 0 || src->dir.y != 0 || src->dir.z != 0) {
            FixVector endView;
            FixMatrix_TransformPoint(&endView, &beamEnd, &invView);
            if (endView.z >= 0x10000) {
                FixMatrix_ProjectWorldPointToView((int *)&vOut, &beamEnd, view);
                endU = (float)(vOut.x >> 16) / resXf; endV = (float)(vOut.y >> 16) / resYf;
            } else {
                // The beam crosses the near plane (rear lamps from a chase
                // view): clip the lamp-to-end segment at the near plane in
                // view space. Projecting the raw end yields the behind-camera
                // sentinel (-100,-100 px), which mirrored the wedge toward the
                // upper-left corner - the red streak up the left bank.
                if (vView.z - endView.z > 0) {   // lamp in front, end behind
                    FixVector clip;
                    LightField_ClipBeamToNearPlane(&clip, &vView, &endView);
                    // Back to world, then through the standard projection
                    // (clip.z equals its in-front threshold, so it passes).
                    FixVector clipWorld;
                    FixMatrix_TransformPoint(&clipWorld, &clip, &view->world);
                    FixMatrix_ProjectWorldPointToView((int *)&vOut, &clipWorld, view);
                    endU = (float)(vOut.x >> 16) / resXf; endV = (float)(vOut.y >> 16) / resYf;
                }
            }
        }
        // Drop only when the whole beam (lamp to end, with wedge margin) is off screen.
        float u0 = lampU < endU ? lampU : endU, u1 = lampU > endU ? lampU : endU;
        float v0 = lampV < endV ? lampV : endV, v1 = lampV > endV ? lampV : endV;
        if (u1 < -0.25f || v1 < -0.25f || u0 > 1.25f || v0 > 1.25f)
            continue;
        GfxFogLight *fl = &fogLights[fogLightTotal++];
        fl->x = lampU;
        fl->y = lampV;
        fl->viewZ = vView.z / 65536.0f;
        fl->intensity = src->intensity / 65536.0f;
        fl->r = src->r / 65536.0f;
        fl->g = src->g / 65536.0f;
        fl->b = src->b / 65536.0f;
        float dx = (endU - lampU) * aspect, dy = endV - lampV;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 1e-4f) { fl->beamX = dx / len; fl->beamY = dy / len; fl->beamLen = len; }
        else { fl->beamX = 0.0f; fl->beamY = 0.0f; fl->beamLen = 0.0f; }
    }
    return fogLightTotal;
}
#endif
