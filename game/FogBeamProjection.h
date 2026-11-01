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

// proj33/proj43: the game's projection _33/_43 (CGraphics::SetProjection), so
// each light's depth gate can be computed in depth-buffer space without the
// renderer's near/far vote.
inline int FogBeam_ProjectLights(GfxFogLight *fogLights, int maxLights, SceneNode *view, float resXf, float resYf,
                                 float proj33, float proj43)
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
        // Normalize the beam direction before extending it: the beam-end
        // projection below assumes a unit vector, and the asset data feeding
        // this lives outside the repo, so enforce the prerequisite here
        // instead of trusting it. Zero vectors and dirs whose length rounds
        // to zero (tiny vectors) fall back to no beam; FixVecLength /
        // FixVecScaleRecip use 64-bit intermediates. The classic glow path
        // keeps its own convention and is untouched.
        FixVector dirN = src->dir;
        bool hasBeam = src->dir.x != 0 || src->dir.y != 0 || src->dir.z != 0;
        if (hasBeam) {
            int dl = FixVecLength(&dirN);
            if (dl <= 0) { hasBeam = false; dirN.x = dirN.y = dirN.z = 0; }
            else FixVecScaleRecip(&dirN, &dirN, dl);
        }
        // The beam's screen wedge: project a point 30 world units along the
        // beam. Off-screen lamps stay: their wedge can still cross the frame
        // (the cockpit's own lamps sit below the windscreen line).
        FixVector beamEnd;
        beamEnd.x = src->pos.x + FixMul(dirN.x, 0x1e0000);
        beamEnd.y = src->pos.y + FixMul(dirN.y, 0x1e0000);
        beamEnd.z = src->pos.z + FixMul(dirN.z, 0x1e0000);
        FixVector endView = {0, 0, 0};
        if (hasBeam)
            FixMatrix_TransformPoint(&endView, &beamEnd, &invView);
        const bool lampIn = vView.z >= 0x10000, endIn = hasBeam && endView.z >= 0x10000;
        if (!lampIn && !endIn)
            continue;   // the whole beam sits behind the camera
        FixVector anchorView = vView;
        if (!lampIn) {
            // Bonnet/bumper cameras sit within a unit of the lamps: the lamp is
            // at/behind the near plane but its beam reaches into view. Anchor at
            // the near-plane crossing instead of dropping the light.
            if (endView.z - vView.z <= 0)
                continue;
            LightField_ClipBeamToNearPlane(&anchorView, &endView, &vView);
        }
        {   FixVector anchorWorld;
            FixMatrix_TransformPoint(&anchorWorld, &anchorView, &view->world);
            FixMatrix_ProjectWorldPointToView((int *)&vOut, &anchorWorld, view);
        }
        const float lampU = (float)(vOut.x >> 16) / resXf, lampV = (float)(vOut.y >> 16) / resYf;
        float endU = lampU, endV = lampV;
        if (hasBeam) {
            if (endIn) {
                FixMatrix_ProjectWorldPointToView((int *)&vOut, &beamEnd, view);
                endU = (float)(vOut.x >> 16) / resXf; endV = (float)(vOut.y >> 16) / resYf;
            } else if (vView.z - endView.z > 0) {   // lamp in front, end behind
                // The beam crosses the near plane (rear lamps from a chase
                // view): clip the lamp-to-end segment at the near plane in
                // view space. Projecting the raw end yields the behind-camera
                // sentinel (-100,-100 px), which mirrored the wedge toward the
                // upper-left corner - the red streak up the left bank.
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
        // Drop only when the whole beam (lamp to end, with wedge margin) is off screen.
        float u0 = lampU < endU ? lampU : endU, u1 = lampU > endU ? lampU : endU;
        float v0 = lampV < endV ? lampV : endV, v1 = lampV > endV ? lampV : endV;
        if (u1 < -0.25f || v1 < -0.25f || u0 > 1.25f || v0 > 1.25f)
            continue;
        GfxFogLight *fl = &fogLights[fogLightTotal++];
        fl->x = lampU;
        fl->y = lampV;
        // Depth gate in depth-buffer space: the raw depth of a point half a
        // world unit BEHIND the anchor, from the game's own projection -
        // valid on both renderers even when the renderer's near/far vote is
        // ambiguous. The half-unit self-occlusion bias makes the car's own
        // bodywork around the lamp (within ~+-1 unit of it: the bonnet and
        // dash panels the halo would paint over) fail the gate, while the
        // fog over the open road - all farther than the lamp - still passes
        // (user: "the light bubble should appear in FRONT of the car").
        // Only an unknown projection (proj43 >= 0) carries no gate.
        {   float zGate = anchorView.z / 65536.0f + 0.5f;
            float d = (proj43 < 0.0f) ? proj33 + proj43 / zGate : 0.0f;
            fl->depthRef = d > 1.0f ? 1.0f : d; }
        fl->intensity = src->intensity / 65536.0f;
        fl->r = src->r / 65536.0f;
        fl->g = src->g / 65536.0f;
        fl->b = src->b / 65536.0f;
        // Wedge = transverse view-space DIRECTION (continuous across
        // near-plane crossings: the projected end's behind-camera position
        // flips side as the beam sweeps past the camera, the transverse
        // component does not) x TRUE foreshortened LENGTH from the projected
        // beam end (endU/endV above, already clipped at the near plane, so
        // the behind-camera sentinel can never reach here). The previous
        // formula derived the length as transverse * (30/depth) * 0.5, which
        // re-amplified any off-axis angle past ~2 degrees back to a
        // full-length wedge pointing straight up/down/sideways - the
        // 2026-11-01 "headlights pointing all over" driving-view report.
        // Foreshortening supplies the stability: a beam swinging across the
        // view axis shrinks to zero true length as it goes axial, so the
        // transverse direction's sign ambiguity there is invisible; the
        // extra fade band (below ~6 degrees off-axis) removes the rest.
        {   float transverse = 0.0f, bdvx = 0.0f, bdvy = 0.0f;
            if (hasBeam) {
                FixVector bd;
                FixMatrix_RotateVector(&bd, &dirN, &invView);
                const double bx = bd.x, by = bd.y, bz = bd.z;
                const double bl = sqrt(bx * bx + by * by + bz * bz);
                if (bl > 0.0) {
                    transverse = (float)(sqrt(bx * bx + by * by) / bl);
                    bdvx = (float)(bx / bl); bdvy = (float)(by / bl);
                }
            }
            float dx = (endU - lampU) * aspect, dy = endV - lampV;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.6f) len = 0.6f;   // screen-fraction cap
            float band = transverse / 0.10f;
            len *= band > 1.0f ? 1.0f : band;
            if (len > 1e-4f && (bdvx != 0.0f || bdvy != 0.0f)) {
                float bx = bdvx * aspect, by = bdvy;
                float bl = sqrtf(bx * bx + by * by);
                fl->beamX = bx / bl; fl->beamY = by / bl; fl->beamLen = len;
            } else { fl->beamX = 0.0f; fl->beamY = 0.0f; fl->beamLen = 0.0f; }
        }
        // Glow bubbles removed (user steering, 1 Nov 2026: "keep just the
        // beams"). The halo slots stay in the uniform layout (both render
        // paths share it) but are pinned to the lamp anchor with the lamp's
        // own depth gate; the fog shader no longer reads them.
        fl->haloX = lampU; fl->haloY = lampV; fl->haloRef = fl->depthRef;
    }
    return fogLightTotal;
}
#endif
