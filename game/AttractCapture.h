#ifndef OPENCMR2_ATTRACT_CAPTURE_H
#define OPENCMR2_ATTRACT_CAPTURE_H
// --attract-capture DIR: start the attract demo at once and save the frame at fixed
// STAGE-CLOCK times (not wall clock), so an A/B pair compares the same point of the
// same deterministic-setup demo. Capture only; no gameplay or physics input.
#include "port/sys.h"
#include "port/gfx.h"
#include <stdio.h>
namespace AttractCapture {
struct Row { int target, clock, country, stage, speed, x, y, z, view, drone; };
struct State { bool init, enabled, started, done, pending; int interval, count, next, quitIn, frames; unsigned serial; Row row; char dir[480]; FILE *manifest; };
inline State &S() { static State s = {false, false, false, false, false, 5, 7, 0, 0, 0, 0, {}, "", NULL}; return s; }
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
    if (s.manifest) { fprintf(s.manifest, "target_s,request_clock_ms,saved_clock_ms,tick_frames_to_save,country,stage,speed_raw,car_x,car_y,car_z,view_type,drone_view\n"); fflush(s.manifest); }
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
// Called once per rendered race frame while the demo runs. The renderer saves the next
// presented frame after the request, so a row is written once the save is seen: it holds the
// state at the request, the clock when the save was seen, and how many ticks that took.
inline void Tick(int clockMs, int country, int stage, int speedRaw, int x, int y, int z, int view, int drone) {
    State &s = S();
    if (!Started() || s.done) return;
    if (s.pending) {
        s.frames++;
        if (Gfx_GetCaptureSerial() != s.serial) {
            s.pending = false;
            if (s.manifest) { fprintf(s.manifest, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", s.row.target, s.row.clock, clockMs, s.frames,
                s.row.country, s.row.stage, s.row.speed, s.row.x, s.row.y, s.row.z, s.row.view, s.row.drone); fflush(s.manifest); }
        } else if (s.frames > 120) { s.pending = false; Sys_Log("capture: shot %d not saved after 120 frames", s.row.target); }
        return;
    }
    if (s.next >= s.count) {
        if (--s.quitIn <= 0) { s.done = true; if (s.manifest) fclose(s.manifest); s.manifest = NULL; Sys_RequestQuit(); }
        return;
    }
    int target = s.next * s.interval;
    if (clockMs < target * 1000) return;
    char path[600];
    snprintf(path, sizeof path, "%s/demo-t%03d.png", s.dir, target);
    s.serial = Gfx_GetCaptureSerial();
    Gfx_RequestCapture(path);
    s.row = {target, clockMs, country, stage, speedRaw, x, y, z, view, drone};
    s.pending = true;
    s.frames = 0;
    s.next++;
    s.quitIn = 8;
}
}
#endif
