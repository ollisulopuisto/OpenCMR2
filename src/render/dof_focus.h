#ifndef OPENCMR2_DOF_FOCUS_H
#define OPENCMR2_DOF_FOCUS_H
// Depth-of-field focus on the followed car. The focus plane is where the car actually is, taken from the car's own draws:
// the view-space forward depth of each draw's world origin, averaged. Visual only. The mapping mirrors dof.frag, which measures
// focus on a log scale between the near and far plane (0 = near, 1 = far). Row-vector matrices (D3D convention, _34 = 1).
#include <math.h>
// View-space forward depth of world point p (view is 16 floats, row major).
inline float DofFocus_ViewDepth(const float *view, const float p[3]) { return p[0] * view[2] + p[1] * view[6] + p[2] * view[10] + view[14]; }
// Log-scale focus position for a view depth z; false when it is unusable (no near/far, or the point is not in front of the near plane).
inline bool DofFocus_Position(float z, float nearP, float farP, float *out)
{
    if (!(nearP > 0.0f) || !(farP > nearP) || !(z > nearP)) return false;
    float t = logf(z / nearP) / logf(farP / nearP);
    *out = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return true;
}
struct DofFocusAccum {
    double sum = 0.0; int count = 0;
    void Add(const float *view, const float p[3]) { float z = DofFocus_ViewDepth(view, p); if (z > 0.0f) { sum += z; ++count; } }
    // Mean view depth of the car's draws, or false when it has none in front of the camera (cockpit, car hidden).
    bool Depth(float *z) const { if (!count) return false; *z = (float)(sum / count); return true; }
};
#endif
