#ifndef OPENCMR2_COCKPIT_EYE_SHIFT_H
#define OPENCMR2_COCKPIT_EYE_SHIFT_H
// Cockpit eye shift (visual only): distance, 16.16 car-model units, to move the eye along the camera's forward axis. 0 when the view is not wider than saved
// or the setting is 0. Full setting (+-100) at x2.0 widening moves kCockpitEyeShiftMax. The driver mount offset is 0x3333 (0.2) high, so 0.25 is a modest move.
#include <stdint.h>
static const int kCockpitEyeShiftMax = 0x4000;   // 0.25 model units at 100% and x2.0 widening; a guess to tune natively, not a measured limit
inline int CockpitEyeShift_Amount(int widening16, int pct)
{
    if (pct == 0 || widening16 <= 65536) return 0;
    if (pct > 100) pct = 100;
    if (pct < -100) pct = -100;
    long long excess = widening16 - 65536; if (excess > 65536) excess = 65536;   // x1 .. x2 maps to 0..1
    return (int)(excess * kCockpitEyeShiftMax / 65536 * pct / 100);
}
#endif
