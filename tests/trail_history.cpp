#include "../game/TrailHistory.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
struct P { int id; };
int main(){
    TrailHistory<P> h; h.Init(128);
    CHECK(h.cap == 128);
    for (int i = 0; i < 100; ++i) h.Push(P{i}, i * 10, 0);
    CHECK(h.count == 100 && h.At(99).id == 99 && h.At(0).id == 0);
    // all near: every consecutive pair queued, newest first
    int first = -1, n = 0;
    n = h.Walk(0, 0, 1000000, 1000, 100000, [&](const P &c, const P &p){ if (first < 0) first = c.id; return c.id == p.id + 1; });
    CHECK(n == 99 && first == 99);
    // budget
    CHECK(h.Walk(0, 0, 1000000, 10, 100000, [&](const P &, const P &){ return true; }) == 10);
    // wrap: capacity is hard, count never exceeds it, newest stays correct
    for (int i = 100; i < 1000; ++i) h.Push(P{i}, i * 10, 0);
    CHECK(h.count <= 128 && h.count >= 128 - kTrailChunk + 1);
    CHECK(h.At(h.count - 1).id == 999);
    bool contiguous = true; for (int i = 1; i < h.count; ++i) if (h.At(i).id != h.At(i - 1).id + 1) contiguous = false;
    CHECK(contiguous);
    // culling: points are at x = id*10; reference at the newest, radius 100 -> only a few chunks near
    int seen = 0;
    h.Walk(9990, 0, 100, 1000, 100000, [&](const P &, const P &){ ++seen; return true; });
    CHECK(seen > 0 && seen <= kTrailChunk + 1);
    // far reference draws nothing
    CHECK(h.Walk(-1000000, 0, 10, 1000, 100000, [&](const P &, const P &){ return true; }) == 0);
    CHECK(h.Walk(0, 0, 100000000, 1000, 7, [&](const P &, const P &){ return false; }) == 0);
    { int v = 0; h.Walk(0, 0, 100000000, 1000, 7, [&](const P &, const P &){ ++v; return false; }); CHECK(v == 7); }
    h.Clear(); CHECK(!h.Active() && h.count == 0);
    std::printf(fails ? "FAILED\n" : "trail_history ok\n");
    return fails;
}
