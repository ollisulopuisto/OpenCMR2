#ifndef OPENCMR2_FOV_PREVIEW_H
#define OPENCMR2_FOV_PREVIEW_H
#include <cmath>
// OpenCMR2 debug (visual only): a live override of video.fov, so the same camera can be looked at with FOV 25, 40, 75, 90 ...
// video.fov is the SilentPatch-style number: the projection zoom is scaled by 70 / fov, and a higher number is a wider view.
// It is an ABSOLUTE setting, not the Speed FOV %, which widens the view relative to it as the car speeds up. The preview
// replaces only the absolute number, 0 = off (the saved video.fov is used), and it is never saved by itself.
inline int FovPreview_Effective(int preview, int setting)
{
    if (preview <= 0) return setting < 30 ? 30 : setting > 150 ? 150 : setting;   // the old clamp for the saved setting
    return preview < 20 ? 20 : preview > 150 ? 150 : preview;                     // the preview may go a little narrower
}
// Zoom scale for an effective fov, the same formula the game used before (0x275c2 is the original's value at 70).
inline double FovPreview_Scale(int fov) { return 0x275c2 * 70.0 / fov; }
// How much bigger things look than at the saved setting (1.0 = same, below 1 = wider view).
inline double FovPreview_Magnification(int effective, int setting) { return (double)setting / effective; }
// The field of view in degrees that the projection matrix really gives. SetProjection stores element = 0.5 * fov (the fixed projection
// scale), and a perspective matrix element is 1 / tan(half angle), so the full angle is 2 * atan(2 / fov). 0 when not yet set.
inline double FovPreview_MeasuredDegrees(double fov)
{
    if (fov <= 0.0) return 0.0;
    return 2.0 * std::atan(2.0 / fov) * 180.0 / 3.14159265358979323846;
}
// One easing step of the shown fov number (16.16) toward the target, closing about half the gap in 400 ms so a preview change (or leaving the
// preview) is a smooth trombone shot, not a jump. elapsedMs outside 0..250 (first frame, pause, hitch) snaps. Both the projection and the dolly read the same shown value.
inline int FovPreview_Ease(int shown16, int target16, int elapsedMs)
{
    if (shown16 <= 0 || elapsedMs < 0 || elapsedMs > 250) return target16;
    return shown16 + (int)((long long)(target16 - shown16) * elapsedMs / (elapsedMs + 400));
}
#endif
