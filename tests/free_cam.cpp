#include "FreeCam.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }
static void Run(FreeCam &c, const FreeCamInput &in, double seconds) { for (int i = 0; i < (int)std::lround(seconds * 10); i++) c.Update(in, 0.1); }   // frames of 0.1 s
int main()
{
    double p[3] = {10, 5, -3}, f[3] = {0, 0, 1}, r[3] = {1, 0, 0};
    FreeCam c; FreeCamInput none = {};
    CHECK(!c.active);
    { FreeCamInput t = {}; t.forward = true; c.Update(t, 1); }                      // inactive: no movement
    CHECK(near(c.pos[0], 0));
    c.Start(p, f, r);
    CHECK(c.active && c.hand == 1 && near(c.yaw, 0) && near(c.pitch, 0));
    c.Update(none, 0.04);                                  // no keys: pose unchanged (works with zero sim steps)
    CHECK(near(c.pos[0], 10) && near(c.pos[2], -3));
    FreeCamInput w = {}; w.forward = true;
    Run(c, w, 0.5);
    CHECK(near(c.pos[2], -3 + 6) && near(c.pos[0], 10));
    FreeCamInput d = {}; d.right = true;
    Run(c, d, 0.5);                                      // strafe along the screen's right
    CHECK(near(c.pos[0], 16));
    FreeCamInput q = {}; q.up = true, q.fast = true;
    Run(c, q, 0.3);                                     // vertical, fast: 12*4*0.3
    CHECK(near(c.pos[1], 5 + 12 * 4 * 0.3));
    FreeCamInput tr = {}; tr.turnRight = true;
    double yaw0 = c.yaw; c.Update(tr, 0.1);
    CHECK(c.yaw > yaw0);                                   // turn right moves forward toward +x (the right axis here)
    FreeCamAxes a = c.Axes();
    CHECK(a.f[0] > 0);
    CHECK(near(a.f[0] * a.r[0] + a.f[1] * a.r[1] + a.f[2] * a.r[2], 0));   // orthogonal
    CHECK(near(a.f[0] * a.u[0] + a.f[1] * a.u[1] + a.f[2] * a.u[2], 0));
    // Mirrored engine axes: right is -x. Turning right must still turn toward the screen's right (-x).
    double rm[3] = {-1, 0, 0}; FreeCam m; m.Start(p, f, rm);
    CHECK(m.hand == -1);
    m.Update(tr, 0.1);
    CHECK(m.Axes().f[0] < 0);
    FreeCamInput s = {}; s.right = true; double x0 = m.pos[0]; m.Update(s, 0.1);
    CHECK(m.pos[0] < x0 + 1e-12 && std::fabs(m.pos[0] - x0) > 0.1);
    // Pitch clamps, long stalls are clamped, look keys.
    FreeCamInput lu = {}; lu.lookUp = true;
    for (int i = 0; i < 100; i++) c.Update(lu, 0.1);
    CHECK(c.pitch <= FreeCam::kMaxPitch + 1e-12 && c.pitch > 1.4);
    double before = c.pos[2]; c.Update(w, 50);
    CHECK(std::fabs(c.pos[2] - before) <= FreeCam::kMove * 0.1 + 1e-9);
    // Start from a looking-down camera: pitch comes from the camera.
    double fd[3] = {0, -0.5, std::sqrt(0.75)}; FreeCam k; k.Start(p, fd, r);
    CHECK(near(k.pitch, std::asin(-0.5)));
    puts("free cam ok");
    return 0;
}
