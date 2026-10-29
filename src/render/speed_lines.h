#ifndef OPENCMR2_SPEED_LINES_H
#define OPENCMR2_SPEED_LINES_H
// Speed lines: thin rays converging on the screen centre (vanishing point), drawn inside the speed streak pass.
// They share the speed streak strength (video.speed_streak), so there is no separate knob and no duplicate pass.
// Visual only. Mirrors the LINES_* constants in shaders/speedstreak.frag (tests/speed_lines_wiring.py checks parity).
// The clear zone is an artistic radial exclusion around the centre, not a car mask.
static const float kSpeedLinesInner = 0.34f;  // radius (screen heights, aspect corrected) with no lines
static const float kSpeedLinesFade = 0.22f;   // width of the fade from clear to full
static const float kSpeedLinesRays = 80.0f;   // ray slots around the centre; about half are absent
static const float kSpeedLinesOnset = 0.25f;  // streak strength (speed ratio x video.speed_streak) where lines start to appear
static const float kSpeedLinesFull = 0.80f;   // streak strength where they are fully shown
static const float kSpeedLinesOpacity = 0.55f; // brightest a line gets
#endif
