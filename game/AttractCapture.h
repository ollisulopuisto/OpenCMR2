#ifndef OPENCMR2_ATTRACT_CAPTURE_H
#define OPENCMR2_ATTRACT_CAPTURE_H
// --attract-capture DIR: start the attract demo at once and save the frame at fixed
// STAGE-CLOCK times (not wall clock), so an A/B pair compares the same point of the
// same deterministic-setup demo. Capture only; no gameplay or physics input.
#include "port/sys.h"
#include "port/gfx.h"
#include <stdio.h>
namespace AttractCapture {
struct State { bool init, enabled, started, done; int interval, count, next, quitIn; char dir[480]; FILE *manifest; };
inline State &S() { static State s = {false, false, false, false, 5, 7, 0, 0, "", NULL}; return s; }
inline bool Enabled() {
    State &s = S();
    if (!s.init) {
        s.init = true;
        const char *d = Sys_GetCaptureDir(&s.interval, &s.count);
        if (d && *d) { s.enabled = true; snprintf(s.dir, sizeof s.dir, "%s", d); }
    }
    return s.enabled;
}
inline unsigned MenuDelayMs() { return Enabled() ? 1500u : 30500u; }
inline bool Started() { return Enabled() && S().started; }
inline void OnDemoStart() {
    State &s = S();
    if (!Enabled() || s.started) return;
    s.started = true;
    char path[600];
    snprintf(path, sizeof path, "%s/capture.csv", s.dir);
    s.manifest = fopen(path, "w");
    if (s.manifest) { fprintf(s.manifest, "target_s,stage_clock_ms,country,stage,speed_raw\n"); fflush(s.manifest); }
    Sys_Log("capture: demo started, %d shots every %d s of stage clock into %s", s.count, s.interval, s.dir);
}
// Called when the main menu is entered again: the demo ended before all shots were taken.
inline void OnMainMenu() {
    State &s = S();
    if (Enabled() && s.started && !s.done) {
        Sys_Log("capture: demo ended after %d of %d shots", s.next, s.count);
        s.done = true;
        Sys_RequestQuit();
    }
}
// Called once per rendered race frame while the demo runs.
inline void Tick(int clockMs, int country, int stage, int speedRaw) {
    State &s = S();
    if (!Started() || s.done) return;
    if (s.next >= s.count) {
        if (--s.quitIn <= 0) { s.done = true; if (s.manifest) fclose(s.manifest); s.manifest = NULL; Sys_RequestQuit(); }
        return;
    }
    int target = s.next * s.interval;
    if (clockMs < target * 1000) return;
    char path[600];
    snprintf(path, sizeof path, "%s/demo-t%03d.png", s.dir, target);
    Gfx_RequestCapture(path);
    if (s.manifest) { fprintf(s.manifest, "%d,%d,%d,%d,%d\n", target, clockMs, country, stage, speedRaw); fflush(s.manifest); }
    s.next++;
    s.quitIn = 8;
}
}
#endif
