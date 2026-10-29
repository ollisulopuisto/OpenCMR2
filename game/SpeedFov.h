#ifndef SPEED_FOV_H
#define SPEED_FOV_H
// OpenCMR2 (visual only): pure maths of video.speed_fov, split out so it can be tested without the renderer.
// Speed ratio: car speed (16.16 per step) over 1.5, the same "full speed" the tunnel-vision ramp uses, clamped 0..1.
// Reversing counts as 0.
inline int SpeedFov_Target(int carSpeed)
{
    float ratio = carSpeed / 65536.f / 1.5f;
    ratio = ratio < 0.f ? 0.f : ratio > 1.f ? 1.f : ratio;
    return (int)(ratio * 65536.f);
}
// Diagnostic hold (video.speed_fov_hold, set only by --test-preset overdo): pretend full speed so the widening can be A/B tested
// standing still by changing the speed FOV percent. Off = the real speed target.
inline int SpeedFov_HeldTarget(int target, int hold) { return hold ? 65536 : target; }
// Same hold for the tunnel vision speed ratio (0..1, may exceed 1 before clamping): full speed while held, else the real ratio.
inline float SpeedFov_HeldRatio(float ratio, int hold) { return hold ? 1.0f : ratio; }
// One easing step toward target (16.16). elapsedMs outside 0..250 (pause, first frame, camera restart) snaps to target.
inline int SpeedFov_Ease(int shown, int target, int elapsedMs)
{
    if (elapsedMs < 0 || elapsedMs > 250) return target;
    return shown + (int)((long long)(target - shown) * elapsedMs / (elapsedMs + 400));
}
// Divides the projection zoom so the view widens by pct percent at full speed (pct 0..40, shown 16.16 ratio).
inline int SpeedFov_Zoom(int zoom, int pct, int shown)
{
    if (pct < 0) pct = 0;
    if (pct > 40) pct = 40;
    long long gain = (long long)pct * shown / 100;
    return (int)((long long)zoom * 65536 / (65536 + gain));
}
#endif
