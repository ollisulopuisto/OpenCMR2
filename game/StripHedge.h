#ifndef OPENCMR2_STRIP_HEDGE_H
#define OPENCMR2_STRIP_HEDGE_H
#include "FixedStrip.h"
#include "MeshVertex.h"
#include <cmath>
#include <vector>
// Visual-only hedge extrusion for Finland1's multi-tree strip cards (fin01
// temp.sht tresh* fixed parts; treatment picked by the owner Nov 20). The
// flat cutout card - a whole forest line painted on one quad - gains a solid
// hedge body, all derived at draw time from the hash-verified source and its
// cutout alpha:
//
// 1. Back card: the quad copied and offset along the route-aware away
//    normal, darkened. Gives real parallax where the flat card vanished
//    edge-on.
// 2. Stepped top cap: a dark horizontal strip per contour column, following
//    the sheet's alpha silhouette, joining the front silhouette top to the
//    back card. The treeline keeps its uneven top.
// 3. Side caps: dark walls closing both ends down to the ground line.
//
// Source perimeter, road edge, contact, collision records, placement and the
// original vertices are never edited; every added vertex is draw-time only
// and appended after the FixedStrip tessellation it extends. Cap UVs anchor
// on a verified opaque texel so the runtime alpha test keeps them. The hedge
// body carries no wind weight, so the FixedStrip wind only ever moves the
// front silhouette, whose perimeter weights are already zero.
namespace StripHedge {
constexpr float kDepthScale = 0.18f;    // extrusion depth as a fraction of quad height
constexpr float kDepthMin = 0.8f, kDepthMax = 4.0f;   // world units
constexpr float kBackShade = 0.75f, kCapShade = 0.35f;
constexpr float kMinHeight = 1.5f;      // shorter quads are not forest strips
constexpr int kContourCols = 16;

inline bool Build(const MeshVertexF q[4], const unsigned short ids[4], const float awayIn[3],
                  const unsigned char *alpha, int aw, int ah, std::vector<FixedStrip::Vertex> &out) {
    if (!q || !ids || !awayIn || !alpha || aw < 8 || ah < 8 || aw > 1024 || ah > 1024) return false;
    float away[3] = {awayIn[0], awayIn[1], awayIn[2]};
    for (int k = 0; k < 3; k++) if (!std::isfinite(away[k])) return false;
    float al = std::sqrt(away[0] * away[0] + away[1] * away[1] + away[2] * away[2]);
    if (!std::isfinite(al) || al < 0.5f) return false;
    for (int k = 0; k < 3; k++) away[k] /= al;
    for (int k = 0; k < 4; k++) {
        const float *p = (const float *)&q[k];
        for (int c = 0; c < 6; c++) if (!std::isfinite(p[c])) return false;
    }
    float down[3] = {q[2].x - q[0].x, q[2].y - q[0].y, q[2].z - q[0].z};
    float height = std::sqrt(down[0] * down[0] + down[1] * down[1] + down[2] * down[2]);
    if (!std::isfinite(height) || height < kMinHeight) return false;
    float depth = height * kDepthScale;
    if (depth < kDepthMin) depth = kDepthMin;
    if (depth > kDepthMax) depth = kDepthMax;

    // Silhouette top per contour column, in sheet v-space (0 = sheet top).
    float tops[kContourCols + 1];
    for (int i = 0; i <= kContourCols; i++) {
        int gx = int((long long)i * (aw - 1) / kContourCols);
        int row = ah;
        for (int y = 0; y < ah; y++)
            if (alpha[y * aw + gx] >= 128) { row = y; break; }
        tops[i] = float(row) / ah;
    }
    for (int pass = 0; pass < 4; pass++) {
        float prev = tops[0];
        for (int i = 1; i < kContourCols; i++) {
            float cur = tops[i];
            tops[i] = (prev + tops[i] + tops[i + 1]) / 3;
            prev = cur;
        }
    }
    // Opaque anchor texel for cap UVs: the strongest texel survives the alpha test.
    int best = -1, ax = 0, ay = 0;
    for (int y = 0; y < ah; y++)
        for (int x = 0; x < aw; x++)
            if (alpha[y * aw + x] > best) { best = alpha[y * aw + x]; ax = x; ay = y; }
    if (best < 128) return false;
    const float anchorUV[2] = {(ax + 0.5f) / aw, (ay + 0.5f) / ah};

    float up[3] = {-down[0] / height, -down[1] / height, -down[2] / height};
    float lo[3], hi[3];
    for (int k = 0; k < 3; k++) {
        lo[k] = hi[k] = ((const float *)&q[0])[k];
        for (int b = 1; b < 4; b++) {
            float x = ((const float *)&q[b])[k];
            if (x < lo[k]) lo[k] = x;
            if (x > hi[k]) hi[k] = x;
        }
        float ext = away[k] * depth;
        if (ext < 0) lo[k] += ext; else hi[k] += ext;
    }
    auto emit = [&](float u, float v, float off, const float uv[2], float shade,
                    const float nrm[3]) {
        FixedStrip::Vertex o = {};
        for (int k = 0; k < 3; k++) {
            float top = ((const float *)&q[0])[k] * (1 - u) + ((const float *)&q[1])[k] * u;
            float bot = ((const float *)&q[2])[k] * (1 - u) + ((const float *)&q[3])[k] * u;
            ((float *)&o.source)[k] = top * (1 - v) + bot * v + away[k] * off;
            ((float *)&o.source)[3 + k] = nrm[k];
            o.lo[k] = lo[k]; o.hi[k] = hi[k];
        }
        o.source.textureCoordinates[0][0] = uv[0];
        o.source.textureCoordinates[0][1] = uv[1];
        // Live colours come from the quad's own corner vertices.
        o.indices[0] = ids[0]; o.indices[1] = ids[1]; o.indices[2] = ids[2]; o.indices[3] = ids[3];
        o.bary[0] = (1-u)*(1-v); o.bary[1] = u*(1-v);
        o.bary[2] = (1-u)*v; o.bary[3] = u*v;
        o.shade = shade;
        o.depth = 0; o.weight = 0;   // the hedge body does not join the front wind
        return o;
    };
    auto quad = [&](float u0, float v0, float o0, const float uv0[2],
                    float u1, float v1, float o1, const float uv1[2],
                    float u2, float v2, float o2, const float uv2[2],
                    float u3, float v3, float o3, const float uv3[2],
                    const float nrm[3], float shade) {
        // Emit both windings without changing the caller's cull state. Some
        // strip meshes are double-sided, others inherit sector culling.
        out.push_back(emit(u0, v0, o0, uv0, shade, nrm));
        out.push_back(emit(u1, v1, o1, uv1, shade, nrm));
        out.push_back(emit(u2, v2, o2, uv2, shade, nrm));
        out.push_back(emit(u0, v0, o0, uv0, shade, nrm));
        out.push_back(emit(u2, v2, o2, uv2, shade, nrm));
        out.push_back(emit(u3, v3, o3, uv3, shade, nrm));
        auto end=out.size();
        for(int t=0;t<2;t++){
            auto a=out[end-6+t*3],b=out[end-5+t*3],c=out[end-4+t*3];
            out.push_back(a);out.push_back(c);out.push_back(b);
        }
    };

    // Back card: full quad at the extrusion depth, original corner UVs and
    // normals, darkened.
    static const float cuv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    float backN[3] = {q[0].nx, q[0].ny, q[0].nz};
    quad(0, 0, depth, cuv[0], 1, 0, depth, cuv[1], 1, 1, depth, cuv[2], 0, 1, depth, cuv[3],
         backN, kBackShade);
    // Stepped top cap: one horizontal dark run per column at the lower of the
    // two endpoint contour heights, front silhouette top to the back card.
    for (int i = 0; i < kContourCols; i++) {
        float u0 = float(i) / kContourCols, u1 = float(i + 1) / kContourCols;
        float v = std::max(tops[i],tops[i+1]);
        if (v >= 1.0f) continue;   // empty column: silhouette reaches the ground
        quad(u0, v, 0, anchorUV, u1, v, 0, anchorUV, u1, v, depth, anchorUV, u0, v, depth, anchorUV,
             up, kCapShade);
    }
    // Close the vertical risers between successive horizontal top runs.
    for(int i=1;i<kContourCols;i++){
        float vl=std::max(tops[i-1],tops[i]),vr=std::max(tops[i],tops[i+1]);
        if(std::fabs(vl-vr)<1e-6f)continue;
        float u=float(i)/kContourCols;
        quad(u,vl,0,anchorUV,u,vr,0,anchorUV,u,vr,depth,anchorUV,u,vl,depth,anchorUV,up,kCapShade);
    }
    // Side caps: dark walls from the ground line to each end's contour height.
    float sideN[3] = {down[1] * away[2] - down[2] * away[1], down[2] * away[0] - down[0] * away[2],
                      down[0] * away[1] - down[1] * away[0]};
    float sl = std::sqrt(sideN[0] * sideN[0] + sideN[1] * sideN[1] + sideN[2] * sideN[2]);
    if (sl > 1e-5f) for (int k = 0; k < 3; k++) sideN[k] /= sl;
    if (tops[0] < 1.0f)
        quad(0, tops[0], 0, anchorUV, 0, tops[0], depth, anchorUV, 0, 1, depth, anchorUV, 0, 1, 0, anchorUV,
             sideN, kCapShade);
    if (tops[kContourCols] < 1.0f)
        quad(1, tops[kContourCols], 0, anchorUV, 1, tops[kContourCols], depth, anchorUV, 1, 1, depth, anchorUV, 1, 1, 0, anchorUV,
             sideN, kCapShade);
    return true;
}
}
#endif
