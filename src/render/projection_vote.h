#ifndef OPENCMR2_PROJECTION_VOTE_H
#define OPENCMR2_PROJECTION_VOTE_H
#include <cmath>
// Recovers the near/far planes from the game's left-handed D3D-style projection (CGraphics::SetProjection):
//   _33 = f / (f - n), _43 = -n * f / (f - n), _34 = 1.
// Since _33 - 1 = n / (f - n): n = -_43 / _33 and f = -_43 / (_33 - 1). (The earlier code used +_43 / (_33 - 1), which is -f,
// so the "f > n" check always failed, no draw ever voted and the DoF pass always used the raw-depth rule.)
inline bool Projection_NearFar(float m33, float m34, float m43, float *outNear, float *outFar)
{
    if (!(m34 == 1.0f && m33 > 1.0f && m43 < 0.0f)) return false;
    float n = -m43 / m33, f = -m43 / (m33 - 1.0f);
    if (!(n > 0.0f && f > n)) return false;
    *outNear = n; *outFar = f;
    return true;
}
#endif
