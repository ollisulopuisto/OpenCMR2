#ifndef _SNOW_MATERIAL_H
#define _SNOW_MATERIAL_H

#include <string.h>

// Snow surface material (video.snow_material, prototype): the snow GROUND
// responds to light as a surface - sun-directional glints plus a boost inside
// car light beams - by lifting the diffuse colour of the tagged snow vertices
// of the visible terrain meshes. Per-surface identification comes from the
// collision mesh: a visible triangle counts as snow only when the collision
// query at its centroid reports one of the raw snow surface ids the wheel
// particles use (0x17 snow, 0x29 snow-packed, 0x18 ice), the triangle lies on
// the ground, and its face normal points up. Everything else - non-snow
// ground, trees, rocks, buildings, anything beyond the collision corridor -
// keeps its original colours by construction. A vertex shared by a snow and a
// non-snow triangle is never lifted, so a tagged triangle cannot brighten its
// non-snow neighbour. Colours are recomputed from a saved original every
// frame (no cumulative lift) and restored when the option is turned off or
// the stage exits. Render-only: nothing here feeds the car, the camera or
// physics. Covered by tests/snow_material.cpp.

// Raw collision-mesh surface ids that read as snow (see game/SnowGlint.h).
inline int SnowMaterial_IsSnowSurface(unsigned short surface) {
    return surface == 0x17 || surface == 0x29 || surface == 0x18;
}

// Height tolerance for "the visible triangle lies on the ground it was
// matched to": half a world unit (16.16).
#define SNOW_MATERIAL_HEIGHT_TOLERANCE 0x8000

// Whether one visible triangle is snow-covered ground. hit: the collision
// query found a triangle at the centroid. surface: its raw surface id.
// centroidY/groundY: 16.16 heights of the visible centroid and the ground.
// faceNormalY: world-space Y of the triangle face normal (float, -1..1).
inline int SnowMaterial_AcceptTriangle(int hit, unsigned short surface,
                                       int centroidY, int groundY, float faceNormalY) {
    int dy;
    if (!hit || !SnowMaterial_IsSnowSurface(surface)) return 0;
    dy = centroidY - groundY;
    if (dy < 0) dy = -dy;
    if (dy > SNOW_MATERIAL_HEIGHT_TOLERANCE) return 0;   // floats above the ground (tree, bridge)
    if (faceNormalY < 0.5f) return 0;                    // not upward-facing (cliff, trunk, wall)
    return 1;
}

// Folds per-triangle snow flags into per-vertex tags under the shared-vertex
// guard: a vertex is snow only when at least one incident triangle is snow
// and NONE is non-snow. triVertexIndex: 3 indices per triangle (mesh order).
// vertexFlags holds SNOW_MATERIAL_VERTEX_HAS_SNOW / _HAS_OTHER bits per
// vertex and must be zeroed by the caller; returns the final tag count.
#define SNOW_MATERIAL_VERTEX_HAS_SNOW  1
#define SNOW_MATERIAL_VERTEX_HAS_OTHER 2
inline int SnowMaterial_AggregateVertexTags(const unsigned short *triVertexIndex,
                                            const unsigned char *triSnow, int triCount,
                                            unsigned char *vertexFlags, int vertexCount) {
    int t, tagged = 0, i;
    for (t = 0; t < triCount; t++) {
        unsigned char bit = triSnow[t] ? SNOW_MATERIAL_VERTEX_HAS_SNOW : SNOW_MATERIAL_VERTEX_HAS_OTHER;
        for (i = 0; i < 3; i++) {
            unsigned short v = triVertexIndex[t * 3 + i];
            if (v < vertexCount) vertexFlags[v] |= bit;
        }
    }
    for (i = 0; i < vertexCount; i++) {
        if (vertexFlags[i] == SNOW_MATERIAL_VERTEX_HAS_SNOW) tagged++;
    }
    return tagged;
}

// A vertex is lifted only when every incident triangle is snow.
inline int SnowMaterial_VertexIsSnow(unsigned char vertexFlags) {
    return vertexFlags == SNOW_MATERIAL_VERTEX_HAS_SNOW;
}

#include "SnowGlint.h"

// Peak per-channel lift at amount 100: a visible brightening that stays below
// full white so the surface keeps its texture.
#define SNOW_MATERIAL_LIFT_MAX 90

// Combined light response 0..1 of one snow vertex: the sun glint (specular
// against the facet-jittered normal, weather light gated) plus the car-beam
// cone boost summed from the light field. Pure, shared by the game loop and
// the rendered-pixel test.
inline float SnowMaterial_Response(float specular, float sunLevel, float beamBoost) {
    float r = specular * sunLevel + beamBoost * 0.8f;
    return r > 1.f ? 1.f : r;
}

// The lifted vertex colour: original ARGB plus an equal white lift per colour
// channel, alpha preserved. Recomputed from the saved original every frame, so
// it never accumulates; scale 0 (option off) returns the original unchanged.
inline DWORD SnowMaterial_LiftedDiffuse(DWORD original, float response, float scale) {
    int lift = (int)(response * scale);
    int r = ((original >> 16) & 0xff) + lift;
    int g = ((original >> 8) & 0xff) + lift;
    int b = (original & 0xff) + lift;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (original & 0xff000000) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
}

struct SceneNode;
void SnowMaterial_SetStageScene(SceneNode *pRoot);
void SnowMaterial_UpdateFrame(void);
void SnowMaterial_Reset(void);
void SnowMaterial_GetStats(int *pTaggedVertices, int *pApplied);

#endif
