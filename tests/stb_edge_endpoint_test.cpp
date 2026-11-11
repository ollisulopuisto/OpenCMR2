// Synthetic near-vertical edge reproducing the iPad assertion's geometry.
// Keep checks active in release tests too. No proprietary artwork required.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STBTT_assert(condition) do { if (!(condition)) { std::fprintf(stderr, "stb assert: %s\n", #condition); std::exit(1); } } while (0)
#include "stb_truetype.h"
int main() {
    for (float sign : {-1.0f, 1.0f}) {
        std::vector<float> scan(1024), fill(1025);
        stbtt__active_edge edge{};
        edge.fx = 759.9998779296875f;
        edge.fdx = 1.0f / 6128.74267f;
        edge.fdy = 6128.74267f;
        edge.direction = sign;
        edge.sy = 423.0f;
        edge.ey = 424.625f;
        stbtt__fill_active_edges_new(scan.data(), fill.data() + 1, 1024, &edge, 424.0f);
        for (float value : scan)
            if (!std::isfinite(value) || std::fabs(value) > 1.01f) return 2;
        if (scan[759] * sign <= 0 || scan[760] * sign < 0 ||
            std::fabs(fill[761] - sign * 0.625f) > 1e-6f) return 3;
    }
    std::puts("near-vertical clipped endpoints: pass");
}
