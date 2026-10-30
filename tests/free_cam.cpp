#include "FreeCam.h"
#include "CameraOperator.h"   // the engine camera that ships: its axis formulas are the coordinate convention the free cam must reproduce
#include <cstdio>
#include <initializer_list>
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
    // Convention check against CameraOperator_Apply (game/CameraOperator.cpp): y up, f = (sin h, y, cos h), r = unit(f.z, 0, -f.x), u = f x r.
    // FreeCam must rebuild exactly those axes from the camera it starts from, hand +1, so turn/strafe/tilt match the real camera.
    for (double h : {0.0, 0.7, 2.0, -1.3, 3.0}) for (double pt : {0.0, 0.3, -0.4}) {
        double cf[3] = {std::cos(pt) * std::sin(h), std::sin(pt), std::cos(pt) * std::cos(h)};
        CameraOperator::V3 f = {(float)cf[0], (float)cf[1], (float)cf[2]};
        CameraOperator::V3 r = CameraOperator::Unit({f.z, 0, -f.x});
        CameraOperator::V3 u = {f.y * r.z - f.z * r.y, f.z * r.x - f.x * r.z, f.x * r.y - f.y * r.x};
        double cr[3] = {r.x, r.y, r.z};
        FreeCam e; double pos0[3] = {1, 2, 3}; e.Start(pos0, cf, cr);
        FreeCamAxes ax = e.Axes();
        CHECK(e.hand == 1);
        CHECK(std::fabs(ax.f[0] - f.x) < 1e-5 && std::fabs(ax.f[1] - f.y) < 1e-5 && std::fabs(ax.f[2] - f.z) < 1e-5);
        CHECK(std::fabs(ax.r[0] - r.x) < 1e-5 && std::fabs(ax.r[1] - r.y) < 1e-5 && std::fabs(ax.r[2] - r.z) < 1e-5);
        CHECK(std::fabs(ax.u[0] - u.x) < 1e-5 && std::fabs(ax.u[1] - u.y) < 1e-5 && std::fabs(ax.u[2] - u.z) < 1e-5);
    }
    puts("free cam ok");
    return 0;
}
