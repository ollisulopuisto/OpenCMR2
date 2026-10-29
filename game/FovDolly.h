#ifndef OPENCMR2_FOV_DOLLY_H
#define OPENCMR2_FOV_DOLLY_H
// FOV dolly ("trombone shot"): when the chase camera's field of view is wider than the saved video.fov (by speed FOV or by the
// FOV preview), the camera moves toward the car so the car keeps about the same size on screen while the background stretches.
// Visual only: it scales the chase camera's offset from the car and never touches the car, its physics or the road.
// Pure maths, no game state. Magnifications are 16.16: 65536 = same view as the saved setting, above = wider view (smaller things).
#include <stdint.h>
static const int kFovDollyMinScale = 0x7333;   // never closer than 0.45 of the saved distance, so the camera cannot reach the car body
// Total widening from the saved view: speed FOV widening times effective/saved video.fov (a higher number is a wider view).
inline int FovDolly_Widening(int speedMag16, int savedFov, int effectiveFov)
{
    if (savedFov <= 0 || effectiveFov <= 0 || speedMag16 <= 0) return 65536;
    long long w = (long long)speedMag16 * effectiveFov / savedFov;
    return w < 65536 ? 65536 : w > 0x7fffffffLL ? 0x7fffffff : (int)w;   // a narrower view never pushes the camera away
}
// Offset scale 16.16 for amount 0..100 percent of the full compensation (full = 1 / widening), clamped to kFovDollyMinScale..1.
inline int FovDolly_Scale(int widening16, int amountPct)
{
    if (widening16 <= 65536) return 65536;
    if (amountPct < 0) amountPct = 0;
    if (amountPct > 100) amountPct = 100;
    long long full = (65536LL * 65536) / widening16;                       // full compensation scale, below 65536
    long long scale = 65536 - (65536 - full) * amountPct / 100;
    return scale < kFovDollyMinScale ? kFovDollyMinScale : scale > 65536 ? 65536 : (int)scale;
}
#endif
