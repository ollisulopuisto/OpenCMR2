#include "render/gfx_internal.h"
#include "render/temporal_uniforms.h"
#include <cstdio>
static int failures;
static void Check(bool v, const char *s) { if (!v) { std::printf("FAIL: %s\n", s); ++failures; } }
static void Ident(GfxMatrix &m) { for (int i = 0; i < 16; i++) (&m._11)[i] = (i % 5 == 0) ? 1.f : 0.f; }
int main() {
    GfxCommand c; GfxDrawState s; TemporalFrame f; TemporalTracker t;
    Ident(s.view); Ident(s.projection); s.projection._34 = 1.f; s.projection._43 = -1.f; s.projection._33 = 0.f; s.projection._44 = 0.f;
    s.screenW = 640; s.screenH = 480; s.viewport.x = s.viewport.y = 0; s.viewport.width = 640; s.viewport.height = 480;
    c.raceWorld = false;
    Check(!t.Prepare(c, s, 640, 480, f) && !f.active, "non-race world is inactive");
    c.raceWorld = true;
    Check(t.Prepare(c, s, 640, 480, f) && f.active && !f.hasHistory && f.u.params[0] == 0.f, "first frame has no history");
    t.Commit(f);
    Check(t.Prepare(c, s, 640, 480, f) && f.hasHistory && f.u.params[0] > 0.f, "second frame uses history");
    s.viewport.width = 320;
    Check(!t.Prepare(c, s, 640, 480, f), "partial viewport is inactive");
    s.viewport.width = 640;
    Check(t.Prepare(c, s, 640, 480, f) && !f.hasHistory, "history dropped after inactive frame");
    t.Commit(f);
    s.view._41 = 1000.f; // camera teleport
    Check(t.Prepare(c, s, 640, 480, f) && !f.hasHistory, "camera cut resets history");
    t.Commit(f);
    Check(t.Prepare(c, s, 640, 480, f) && !f.hasHistory == false, "stable camera keeps history");
    Check(t.Prepare(c, s, 800, 600, f) && !f.hasHistory, "resize resets history");
    {
        TemporalTracker t2; TemporalFrame g; float a[3]={1,2,3}, b[3]={4,2,3};
        t2.Prepare(c,s,640,480,g); t2.AddObject(g,1,a); Check(g.u.carDelta[0][3]==0.f,"new car has no history"); t2.Commit(g);
        t2.Prepare(c,s,640,480,g); t2.AddObject(g,1,b); t2.AddObject(g,1,a);
        Check(g.u.carDelta[0][3]==1.f && g.u.carDelta[0][0]==3.f,"car delta from previous position, first call wins");
        t2.AddObject(g,0,a); t2.AddObject(g,9,a); Check(g.u.carCenter[1][3]==0.f,"invalid ids ignored");
    }
    {
        TemporalTracker t3; TemporalFrame g;
        s.projection._33 = 1.0f; s.projection._43 = -0.1f; s.projection._34 = 1.f;
        s.view._41 = 0;
        Check(t3.Prepare(c, s, 640, 480, g) && g.u.target[2] == -0.1f && g.u.params[3] == 1.0f, "projection constants reach the uniforms");
        t3.Commit(g);
        s.projection._43 = -0.2f;
        Check(t3.Prepare(c, s, 640, 480, g) && !g.hasHistory, "projection change drops history");
    }
    std::printf(failures ? "FAILED\n" : "OK\n");
    return failures;
}
