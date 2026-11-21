#ifndef OPENCMR2_WINDSHIELD_CRACK_PANE_H
#define OPENCMR2_WINDSHIELD_CRACK_PANE_H

#include "port/types.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "WindshieldGlassSelect.h"
#include "port/gfx.h"
#include <cmath>

namespace WindshieldCrackPane {

// Per-car diagnostic aggregation for the latest cockpit draw. Later intact
// groups in that draw must not erase success. Mirrors do not call this path.
struct Status {
    int drawn = 0;
    const char *reason = "overlay: no live damaged exterior glass";
    void Record(int count, const char *failure) {
        drawn += count;
        reason = drawn ? "overlay drawn" : failure;
    }
};

// Native selectors 0/3/5 mean intact/damaged/broken. Never manufacture damage
// from a previous viewport update while the live exterior still uses intact.
inline int DamageSelector(int live)
{
    return live == 3 || live == 5 ? live : -1;
}

inline void SetState(const GfxMatrix &world)
{
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &world);
    Gfx_SetLighting(FALSE);
    Gfx_SetFog(FALSE);
    Gfx_SetAlphaTest(FALSE);
    Gfx_SetDepthTest(TRUE);
    Gfx_SetDepthWrite(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetBodyDirt(0, 0, 0, 0);
    Gfx_SetDistanceFade(0, 0);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageAlphaArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageAlphaOp(1, GFX_TOP_DISABLE);
    // Earlier reflection draws use camera-space UVs and a COUNT3 transform.
    // These source vertices instead carry the exterior material's UV0.
    Gfx_SetStageTexCoordIndex(0, 0);
    Gfx_SetStageTextureTransform(0, GFX_TTFF_DISABLE);
    Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
    Gfx_SetStageMinFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetStageMagFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetStageMipFilter(0, GFX_MIPFILTER_NONE);
}

// Shared by the actual cockpit draw and the synthetic renderer regression.
// Lookup returns only currently drawable texture surfaces. Source is immutable.
template<class Lookup>
inline int Draw(const Mesh *mesh, unsigned group, const GfxMatrix &world, Lookup lookup)
{
    if (!mesh || !mesh->pVertexData || !mesh->pTriangles || !group ||
        mesh->field_0x10 < 1 || mesh->field_0x10 > 65536 ||
        mesh->triangleCount < 1 || mesh->triangleCount > 65536)
        return 0;
    const float *matrix = reinterpret_cast<const float *>(&world);
    for (int i = 0; i < 16; i++)
        if (!std::isfinite(matrix[i])) return 0;

    enum { MaxTriangles = 24 };
    GfxVertex vertices[MaxTriangles * 3];
    GfxTexture *textures[MaxTriangles];
    int count = 0;
    const auto *source = reinterpret_cast<const MeshVertexF *>(mesh->pVertexData);
    // Inner faces first, any face as the fallback, preserving the resolver's
    // group. Two side groups can legitimately use different damage materials.
    for (int pass = 0; pass < 2 && count == 0; pass++) {
        for (int i = 0; i < mesh->triangleCount && count < MaxTriangles; i++) {
            const auto &triangle = mesh->pTriangles[i];
            if (!WgTriInGroup(triangle.flags, group) || (pass == 0 && (triangle.flags & 0x8000)))
                continue;
            const int selector = DamageSelector(triangle.field_0x2c);
            if (selector < 0) continue;
            const int index = triangle.textures[selector];
            if (index < 0 || index >= 2048) continue;
            GfxTexture *texture = lookup(index);
            if (!texture) continue;
            bool valid = true;
            for (int k = 0; k < 3; k++) {
                if (triangle.vertexIndex[k] >= mesh->field_0x10) {
                    valid = false;
                    break;
                }
                const auto &v = source[triangle.vertexIndex[k]];
                const float values[] = { v.x, v.y, v.z, v.nx, v.ny, v.nz,
                    v.textureCoordinates[0][0], v.textureCoordinates[0][1] };
                for (float value : values)
                    if (!std::isfinite(value)) valid = false;
                vertices[count * 3 + k] = { v.x, v.y, v.z, v.nx, v.ny, v.nz,
                    0xffffffff, 0, v.textureCoordinates[0][0], v.textureCoordinates[0][1], 0, 0 };
            }
            if (valid) textures[count++] = texture;
        }
    }
    if (!count) return 0;
    Gfx_PushState();
    SetState(world);
    for (int i = 0; i < count; i++) {
        Gfx_SetTexture(0, textures[i]);
        Gfx_DrawPrimitive(GFX_TRIANGLELIST, vertices + i * 3, 3);
    }
    Gfx_PopState();
    return count;
}

} // namespace WindshieldCrackPane
#endif
