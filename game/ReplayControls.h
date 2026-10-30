#ifndef OPENCMR2_REPLAY_CONTROLS_H
#define OPENCMR2_REPLAY_CONTROLS_H
// Replay playback controls (keyboard only, visual/time only): pause, slower, faster. The recorded controls are still re-run through the unchanged
// physics step by step; only the real time that feeds the step count is scaled, so a paused or slowed replay shows the same run, not a different one.
// No reverse: the replay is re-simulated from recorded inputs, so going back would mean restoring the start state and re-running, which is not done here.
#include <cmath>
#include <cstdio>
// DirectInput scancodes: , slower  . faster  / pause or resume  Tab next camera (the automatic trackside camera is part of the cycle).
static const int kReplayKeySlower = 0x33, kReplayKeyFaster = 0x34, kReplayKeyPause = 0x35, kReplayKeyCamera = 0x0f;
static const float kReplaySpeeds[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};
static const int kReplaySpeedCount = 6, kReplayNormalIndex = 2;
struct ReplayControls {
    int index = kReplayNormalIndex;   // into kReplaySpeeds
    bool paused = false;
    void Reset() { index = kReplayNormalIndex; paused = false; }
    void Slower() { paused = false; if (index > 0) --index; }
    void Faster() { paused = false; if (index < kReplaySpeedCount - 1) ++index; }
    void TogglePause() { paused = !paused; }
    float Scale() const { return paused ? 0.0f : kReplaySpeeds[index]; }
    void Label(char *out, size_t n) const {
        if (paused) std::snprintf(out, n, "paused");
        else std::snprintf(out, n, kReplaySpeeds[index] < 1.0f ? "speed %.2fx" : "speed %.0fx", kReplaySpeeds[index]);
    }
};
// Scaled sim clock. All users of the frame clock take differences from a fixed epoch, so this keeps a running lag: scale 1 passes real time through
// (offset constant), below 1 the lag grows, above 1 it shrinks. Non-decreasing for any scale >= 0.
struct ReplayClock {
    bool have = false; int lastReal = 0; double lag = 0;
    int Frame(int real, float scale) {
        if (!have) { have = true; lastReal = real; }
        int d = real - lastReal; lastReal = real;
        if (d > 0) lag += d * (1.0 - scale);
        return real - (int)std::floor(lag);
    }
};
inline void ReplayClock_FormatCentiseconds(int cs, char *out, size_t n)
{
    if (cs < 0) cs = 0;
    std::snprintf(out, n, "%d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
}
#endif
