// Scheduler regression for --attract-capture. The stage clock is centiseconds (+4 per 25 Hz step,
// +-1 jitter): an earlier version compared it to milliseconds, so only 2 of 7 shots happened before the demo ended.
#include "AttractCapture.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)

static std::string g_dir; static int g_interval = 5, g_count = 7;
static std::vector<std::string> g_paths; static unsigned g_serial = 0; static int g_pendingSave = -1;
static bool g_quit = false;
extern "C" {
const char *Sys_GetCaptureDir(int *i, int *c) { *i = g_interval; *c = g_count; return g_dir.c_str(); }
void Sys_RequestQuit(void) { g_quit = true; }
void Sys_Log(const char *, ...) {}
void Gfx_RequestCapture(const char *p) { g_paths.push_back(p); g_pendingSave = 1; } // saved by the next presented frame
unsigned Gfx_GetCaptureSerial(void) { return g_serial; }
}

static void Reset() {
    AttractCapture::State &s = AttractCapture::S();
    if (s.manifest) fclose(s.manifest);
    s = AttractCapture::State{false, false, false, false, false, 5, 7, 0, 0, 0, 0, {}, "", NULL};
    g_paths.clear(); g_serial = 0; g_pendingSave = -1; g_quit = false;
}
// One call per 25 Hz tick: clock = 4*n + jitter in -1..2 (rand()%4-1 in the original), the renderer saves one frame after a request.
static int Run(int endCs, bool endEarlyAfterShots = false, int stopAfterShots = -1) {
    int n = 0; unsigned seed = 12345;
    for (; ; ++n) {
        seed = seed * 1103515245u + 12345u;
        int clock = 4 * n + (int)((seed >> 16) % 4) - 1; if (clock < 0) clock = 0;
        if (clock > endCs) break;
        if (g_pendingSave == 0) { g_serial++; } if (g_pendingSave >= 0) g_pendingSave--; // serial changes before the next Tick
        AttractCapture::Tick(clock, 1, 2, 50000, n * 1000, 0, 0, 5, 0);
        if (g_quit) break;
        if (stopAfterShots >= 0 && (int)g_paths.size() >= stopAfterShots && g_pendingSave < 0) break;
    }
    return n;
}
static std::vector<std::string> Lines(const std::string &path) {
    std::vector<std::string> v; FILE *f = fopen(path.c_str(), "r"); char b[600];
    while (f && fgets(b, sizeof b, f)) { v.push_back(b); } if (f) fclose(f); return v;
}
int main() {
    char tmpl[] = "/tmp/attractcapXXXXXX"; CHECK(mkdtemp(tmpl)); g_dir = tmpl;

    // 1. Full run: shots at 0,5,...,30 s of stage clock, then the quit request.
    Reset(); AttractCapture::OnDemoStart();
    CHECK(AttractCapture::Started());
    Run(100 * 100); // far longer than needed: must quit itself
    CHECK(g_quit);
    CHECK(g_paths.size() == 7);
    for (int i = 0; i < 7; i++) { char e[600]; snprintf(e, sizeof e, "%s/demo-t%03d.png", g_dir.c_str(), i * 5); CHECK(g_paths[i] == e); }
    { AttractCapture::State &s = AttractCapture::S(); if (s.manifest) { fclose(s.manifest); s.manifest = NULL; } }
    auto rows = Lines(g_dir + "/capture.csv");
    CHECK(rows.size() == 8);
    CHECK(rows[0].find("request_clock_cs,saved_clock_cs") != std::string::npos);
    for (int i = 0; i < 7; i++) {
        int t, req, saved, ticks; CHECK(sscanf(rows[1 + i].c_str(), "%d,%d,%d,%d", &t, &req, &saved, &ticks) == 4);
        CHECK(t == i * 5);
        CHECK(req >= t * 100 && req <= t * 100 + 8); // requested on the first tick at or after the target (4 cs steps, jitter +-1)
        CHECK(saved >= req && saved - req <= 12 && ticks >= 1 && ticks <= 3);
        int cols = 1; for (char c : rows[1 + i]) if (c == ',') cols++; CHECK(cols == 12);
    }

    // 2. The demo ends 60 s of stage clock in: all 7 shots are already done at 30 s (the old build had 2 here).
    Reset(); AttractCapture::OnDemoStart(); Run(60 * 100, false, 7);
    CHECK(g_paths.size() == 7);
    { int old = 0; for (int t = 0; t < 7; t++) if (t * 5 * 1000 <= 60 * 100) old++; CHECK(old == 2); } // the old ms comparison: 0 s and 5 s only (5000 cs = 50 s)
    CHECK(g_paths.size() > 2);

    // 3. Boundary: the 5 s shot is not requested at 499 cs and is at 500 cs.
    Reset(); AttractCapture::OnDemoStart();
    AttractCapture::Tick(0, 1, 2, 0, 0, 0, 0, 5, 0); CHECK(g_paths.size() == 1);
    g_serial++; AttractCapture::Tick(4, 1, 2, 0, 0, 0, 0, 5, 0); // saves shot 0
    AttractCapture::Tick(499, 1, 2, 0, 0, 0, 0, 5, 0); CHECK(g_paths.size() == 1);
    AttractCapture::Tick(500, 1, 2, 0, 0, 0, 0, 5, 0); CHECK(g_paths.size() == 2);

    // 4. Demo ends early (main menu re-entered): quits and logs, no more shots.
    Reset(); AttractCapture::OnDemoStart(); Run(100 * 100, false, 2);
    CHECK(g_paths.size() == 2); AttractCapture::OnMainMenu(); CHECK(g_quit);

    // 5. Interval/count honoured.
    Reset(); g_interval = 2; g_count = 3; AttractCapture::OnDemoStart(); Run(100 * 100);
    CHECK(g_paths.size() == 3 && g_quit);
    { char e[600]; snprintf(e, sizeof e, "%s/demo-t004.png", g_dir.c_str()); CHECK(g_paths[2] == e); }
    puts("attract_capture ok"); return 0;
}
