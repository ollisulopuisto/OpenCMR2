// Snow surface material (video.snow_material): the snow ground responds to
// light as a surface. Pins the per-surface acceptance rules (collision-mesh
// snow ids, on-ground tolerance, upward face normal), the shared-vertex guard
// (a vertex shared by snow and non-snow triangles is never lifted, so a
// tagged triangle cannot brighten its non-snow neighbour) and the
// vertex-tag aggregation.
#include "port/types.h"
#include "SnowMaterial.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void check(bool ok, const char *msg) {
    if (!ok) { fprintf(stderr, "snow_material: %s\n", msg); exit(1); }
}

int main() {
    // Surface ids: exactly the raw snow ids the wheel particles use.
    check(SnowMaterial_IsSnowSurface(0x17) && SnowMaterial_IsSnowSurface(0x29) &&
          SnowMaterial_IsSnowSurface(0x18), "snow ids 0x17/0x29/0x18 must be snow");
    check(!SnowMaterial_IsSnowSurface(0x05) && !SnowMaterial_IsSnowSurface(0x00) &&
          !SnowMaterial_IsSnowSurface(0x16) && !SnowMaterial_IsSnowSurface(0x19),
          "gravel/tarmac/other ids must not be snow");

    // Acceptance: every condition is required.
    check(SnowMaterial_AcceptTriangle(1, 0x17, 0x10000, 0x10000, 0.9f),
          "snow hit on the ground facing up must accept");
    check(!SnowMaterial_AcceptTriangle(0, 0x17, 0x10000, 0x10000, 0.9f),
          "a query miss must reject (beyond the collision corridor)");
    check(!SnowMaterial_AcceptTriangle(1, 0x05, 0x10000, 0x10000, 0.9f),
          "non-snow surface must reject");
    check(!SnowMaterial_AcceptTriangle(1, 0x17, 0x10000 + SNOW_MATERIAL_HEIGHT_TOLERANCE + 1, 0x10000, 0.9f),
          "floating above the ground must reject (tree canopy over snow)");
    check(SnowMaterial_AcceptTriangle(1, 0x17, 0x10000 + SNOW_MATERIAL_HEIGHT_TOLERANCE, 0x10000, 0.9f),
          "the exact tolerance must still accept");
    check(!SnowMaterial_AcceptTriangle(1, 0x17, 0x10000, 0x10000, 0.4f),
          "a steep face must reject (cliff, trunk)");
    check(!SnowMaterial_AcceptTriangle(1, 0x17, 0x10000, 0x10000, -0.9f),
          "a downward face must reject");

    // Aggregation: snow-only vertices tag; a shared vertex with a non-snow
    // triangle stays clear; unused and non-snow-only vertices stay clear.
    // Two quads sharing an edge: quad A (tris 0,1) snow, quad B (tris 2,3) not.
    //   0---1---2
    //   | A | B |
    //   3---4---5
    unsigned short idx[12] = {0, 1, 4,  0, 4, 3,  1, 2, 5,  1, 5, 4};
    unsigned char triSnow[4] = {1, 1, 0, 0};
    unsigned char flags[6] = {0, 0, 0, 0, 0, 0};
    int tagged = SnowMaterial_AggregateVertexTags(idx, triSnow, 4, flags, 6);
    check(tagged == 2, "only the unshared snow vertices must tag");
    check(SnowMaterial_VertexIsSnow(flags[0]) && SnowMaterial_VertexIsSnow(flags[3]),
          "outer snow vertices must tag");
    check(!SnowMaterial_VertexIsSnow(flags[1]) && !SnowMaterial_VertexIsSnow(flags[4]),
          "vertices shared with a non-snow triangle must not tag");
    check(!SnowMaterial_VertexIsSnow(flags[2]) && !SnowMaterial_VertexIsSnow(flags[5]),
          "non-snow vertices must not tag");

    // All-snow mesh tags every vertex.
    unsigned char allSnow[4] = {1, 1, 1, 1};
    memset(flags, 0, sizeof flags);
    check(SnowMaterial_AggregateVertexTags(idx, allSnow, 4, flags, 6) == 6,
          "an all-snow quad strip must tag every vertex");

    // No snow anywhere tags nothing.
    unsigned char noSnow[4] = {0, 0, 0, 0};
    memset(flags, 0, sizeof flags);
    check(SnowMaterial_AggregateVertexTags(idx, noSnow, 4, flags, 6) == 0,
          "a snow-free mesh must tag nothing");

    printf("snow_material: ok\n");
    return 0;
}
