// Behavioural test for the Finland1 multi-tree-strip hedge extrusion
// (game/StripHedge.h): a flat cutout strip card gains a back card offset away
// from the route, a stepped dark top cap following the sheet's alpha contour
// and dark side caps. Source geometry is never edited; the hedge is additive
// draw-time geometry. Runs before the implementation exists (failing first).
#include "port/types.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "StripHedge.h"

static MeshVertexF corner(float x, float y, float z, float u, float v) {
    MeshVertexF m = {};
    m.x = x; m.y = y; m.z = z;
    m.diffuseColour = 0xff808080u;
    m.textureCoordinates[0][0] = u; m.textureCoordinates[0][1] = v;
    return m;
}

int main() {
    // Vertical 8-wide, 4-tall quad in the z=0 plane. UV-corner order:
    // c[0]=(u0,v0)=top-left, c[1]=top-right, c[2]=bottom-left, c[3]=bottom-right.
    MeshVertexF q[4] = {
        corner(0, 4, 0, 0, 0), corner(8, 4, 0, 1, 0),
        corner(0, 0, 0, 0, 1), corner(8, 0, 0, 1, 1)};
    MeshVertexF original[4];
    memcpy(original, q, sizeof q);
    unsigned short ids[4] = {10, 11, 12, 13};
    float away[3] = {0, 0, 1};

    // 64x32 alpha grid: left half opaque to the very top, right half only to
    // mid height. v=0 is the sheet top, like the FixedStrip wind weighting.
    std::vector<unsigned char> alpha(64 * 32, 0);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 64; x++) {
            int limit = x < 32 ? 32 : 16;   // opaque from this row down
            if (y >= 32 - limit) alpha[y * 64 + x] = 255;
        }

    std::vector<FixedStrip::Vertex> out;
    if (!StripHedge::Build(q, ids, away, alpha.data(), 64, 32, out)) {
        puts("strip hedge: Build declined a valid strip quad");
        return 1;
    }
    if (memcmp(original, q, sizeof q)) { puts("strip hedge: source quad edited"); return 1; }
    if (out.size() % 3) { puts("strip hedge: output is not a triangle list"); return 1; }

    float height = 4.0f;
    float depth = height * StripHedge::kDepthScale;
    if (depth < StripHedge::kDepthMin) depth = StripHedge::kDepthMin;
    if (depth > StripHedge::kDepthMax) depth = StripHedge::kDepthMax;

    bool backCard = false, leftCapTall = false, rightCapShort = false, sideCaps = false;
    bool capShaded = false, capWindFree = true, badAnchor = false, nonFinite = false;
    for (const auto &v : out) {
        const float *p = (const float *)&v.source;
        for (int k = 0; k < 6; k++) if (!std::isfinite(p[k])) nonFinite = true;
        // Back card: offset along +away by exactly depth, keeps foliage shading.
        if (std::fabs(v.source.z - depth) < 1e-4 && v.shade > 0.5f && v.shade < 1.0f &&
            v.source.y > 3.9f) backCard = true;
        // Stepped top cap: horizontal (constant y) runs at the contour level.
        if (v.shade <= 0.5f) {
            capShaded = true;
            if (v.weight != 0.0f) capWindFree = false;
            if (std::fabs(v.source.y - 4.0f) < 0.2f && v.source.x < 4.0f) leftCapTall = true;
            if (std::fabs(v.source.y - 2.0f) < 0.4f && v.source.x > 4.0f && v.source.y < 3.0f)
                rightCapShort = true;
            if (v.source.y < 1.0f) sideCaps = true;   // cap wall reaching the ground line
            // Cap UVs must sit on an opaque texel (the draw alpha-tests).
            int ax = int(v.source.textureCoordinates[0][0] * 64);
            int ay = int(v.source.textureCoordinates[0][1] * 32);
            ax = ax < 0 ? 0 : ax > 63 ? 63 : ax;
            ay = ay < 0 ? 0 : ay > 31 ? 31 : ay;
            if (alpha[ay * 64 + ax] < 128) badAnchor = true;
        }
    }
    if (nonFinite) { puts("strip hedge: non-finite vertex"); return 1; }
    if (!backCard) { puts("strip hedge: no shaded back card at the extrusion depth"); return 1; }
    if (!capShaded) { puts("strip hedge: no darkened cap geometry"); return 1; }
    if (!capWindFree) { puts("strip hedge: cap vertices carry wind weight"); return 1; }
    if (!leftCapTall) { puts("strip hedge: top cap does not follow the tall left contour"); return 1; }
    if (!rightCapShort) { puts("strip hedge: top cap does not drop with the short right contour"); return 1; }
    if (!sideCaps) { puts("strip hedge: no side cap down to the ground line"); return 1; }
    if (badAnchor) { puts("strip hedge: cap UV anchored on a transparent texel"); return 1; }

    bool fourth=false,uv11=false;
    for(const auto&v:out){
        if(v.indices[3]!=ids[3]){puts("lost fourth live colour corner");return 1;}
        float sum=0;for(float w:v.bary)sum+=w;
        if(std::fabs(sum-1)>1e-5){puts("colour weights do not sum to one");return 1;}
        fourth|=v.bary[3]>.99f;
        uv11|=v.shade==StripHedge::kBackShade&&v.source.textureCoordinates[0][0]==1&&v.source.textureCoordinates[0][1]==1;
        if(v.source.z < -1e-5 || v.source.z > depth+1e-5){puts("extrusion escaped away-only bounds");return 1;}
    }
    if(!fourth||!uv11){puts("lost fourth corner colour or back-card UV");return 1;}
    auto previous=out.size();
    std::vector<unsigned char> transparent(64*32,0);
    if(StripHedge::Build(q,ids,away,transparent.data(),64,32,out)||out.size()!=previous){puts("transparent input changed output");return 1;}
    if(StripHedge::Build(q,ids,nullptr,alpha.data(),64,32,out)){puts("accepted null away vector");return 1;}
    if(!StripHedge::Build(q,ids,away,alpha.data(),64,32,out)||out.size()!=previous*2){puts("append contract broken");return 1;}

    // Decline cases: no alpha, degenerate height, non-finite input.
    out.clear();
    if (StripHedge::Build(q, ids, away, nullptr, 0, 0, out)) { puts("strip hedge: accepted null alpha"); return 1; }
    MeshVertexF flat[4] = {corner(0, 1, 0, 0, 0), corner(8, 1, 0, 1, 0), corner(0, 0, 0, 0, 1), corner(8, 0, 0, 1, 1)};
    if (StripHedge::Build(flat, ids, away, alpha.data(), 64, 32, out)) { puts("strip hedge: accepted a degenerate 1-tall quad"); return 1; }
    float bad[3] = {0, 0, std::nanf("")};
    if (StripHedge::Build(q, ids, bad, alpha.data(), 64, 32, out)) { puts("strip hedge: accepted a non-finite away vector"); return 1; }

    puts("strip hedge: additive back card, alpha-contour stepped top cap, side caps, opaque UV anchor, immutable source and decline guards pass");
    return 0;
}
