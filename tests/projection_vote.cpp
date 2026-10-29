// The projection planes must be recoverable from the matrix CGraphics::SetProjection builds (see render/projection_vote.h).
#include "render/projection_vote.h"
#include <cstdio>
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
int main()
{
    const float planes[][2] = { {0.1f, 150.0f}, {0.1f, 300.0f}, {0.5f, 1000.0f}, {1.0f, 150.0f}, {0.25f, 600.0f} };
    for (auto &p : planes) {
        float n = p[0], f = p[1], range = f - n;
        float m33 = f / range, m43 = -(n * f) / range;  // exactly as game/Graphics.cpp ~1666-1682
        float rn = 0, rf = 0;
        CHECK(Projection_NearFar(m33, 1.0f, m43, &rn, &rf));
        CHECK(std::fabs(rn - n) < 1e-3f * n);
        CHECK(std::fabs(rf - f) < 2e-3f * f);
        // the previous formula f = _43 / (_33 - 1) is -f: it can never satisfy f > n, so it never voted
        float oldF = m43 / (m33 - 1.0f);
        CHECK(oldF < 0.0f && !(oldF > n));
    }
    float n, f;
    CHECK(!Projection_NearFar(1.0f, 1.0f, -0.1f, &n, &f));      // not perspective (m33 == 1)
    CHECK(!Projection_NearFar(1.0007f, 0.0f, -0.1f, &n, &f));   // _34 != 1
    CHECK(!Projection_NearFar(1.0007f, 1.0f, 0.1f, &n, &f));    // _43 >= 0
    if (!failures) std::printf("projection_vote ok\n");
    return failures;
}
