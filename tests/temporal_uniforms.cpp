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
    std::printf(failures ? "FAILED\n" : "OK\n");
    return failures;
}
