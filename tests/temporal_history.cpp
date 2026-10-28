#include "render/temporal_history.h"
#include <cstdio>
#include <limits>
using namespace Temporal;
static int failures;
static void Check(bool v, const char *s) { if (!v) { std::printf("FAIL: %s\n", s); ++failures; } }
static Matrix Identity() { Matrix m; m.m[0]=m.m[5]=m.m[10]=m.m[15]=1; return m; }
int main() {
    Config c{640,480,1280,960,1,1}; History h; Matrix a=Identity(), b=a, old;
    Check(h.Begin(c,10) && !h.HasHistory(), "first frame reset");
    Check(!h.Record(7,a,old), "new object has no history"); h.End(true);
    Check(h.Begin(c,11) && h.HasHistory(), "consecutive submitted history");
    b.m[12]=0.1f; Check(h.Record(7,b,old) && old.m[12]==0, "previous transform retained"); h.End(true);
    Check(h.Begin(c,12), "empty frame begin"); h.End(true);
    Check(h.Begin(c,13) && !h.Record(7,b,old), "reappearing object has no stale history"); h.End(true);
    Check(h.Begin(c,15) && !h.HasHistory(), "skipped frame reset"); h.End(true);
    ++c.camera; Check(h.Begin(c,16) && !h.HasHistory(), "camera cut reset"); h.End(true);
    ++c.scene; Check(h.Begin(c,17) && !h.HasHistory(), "scene reset"); h.End(true);
    c.inputWidth=800; Check(h.Begin(c,18) && !h.HasHistory(), "resize reset"); h.End(false);
    Check(!h.HasHistory(), "failed submission reset");
    Check(h.Begin(c,19), "restart after failure"); h.Record(1,a,old);
    Check(!h.Record(1,b,old) && !h.HasHistory(), "duplicate identity fails closed"); h.End(true);
    Check(!h.HasHistory(), "duplicate frame cannot commit");
    Check(h.Begin(c,20), "nonfinite frame begin");
    Matrix bad=a; bad.m[4]=std::numeric_limits<float>::infinity();
    Check(!h.Record(4,bad,old) && !h.HasHistory(), "invalid transform fails closed");
    h.End(true); Check(!h.HasHistory(), "invalid transform cannot commit");
    c.inputWidth=0; Check(!h.Begin(c,20), "invalid dimensions rejected");
    Sample m; Check(Motion(a,a,0,0,0,640,480,m) && m.x==0 && m.y==0, "static motion zero");
    Check(Motion(b,a,0,0,0,640,480,m) && std::fabs(m.x+32)<0.001f, "current to previous horizontal pixels");
    b=a; b.m[13]=0.1f; Check(Motion(b,a,0,0,0,640,480,m) && std::fabs(m.y-24)<0.001f, "top-left vertical pixels");
    b.m[15]=-1; Check(!Motion(b,a,0,0,0,640,480,m), "behind camera rejected");
    b=a; b.m[0]=std::numeric_limits<float>::quiet_NaN(); Check(!Motion(b,a,1,0,0,640,480,m), "nonfinite rejected");
    Check(Jitter(0).x==0 && std::fabs(Jitter(0).y+1.f/6)<0.00001f, "first Halton sample");
    for(unsigned i=0;i<16;++i) { Sample j=Jitter(i); Check(std::fabs(j.x)<=0.5f && std::fabs(j.y)<=0.5f, "bounded jitter"); Check(j.x==Jitter(i+8).x && j.y==Jitter(i+8).y, "deterministic repeat"); }
    h.Reset(); Check(h.CurrentJitter().x==Jitter(0).x, "reset sequence");
    return failures ? 1 : 0;
}
