#ifndef OPENCMR2_DOF_GATE_H
#define OPENCMR2_DOF_GATE_H
// Which parts of the depth-of-field/tunnel-vision pass run. Tunnel vision (speed-driven peripheral blur) is independent of the
// depth blur: it runs with DoF off or dof_blur 0, and then the depth part has zero radius (no unrequested depth blur).
// A frame that is not a race world (menus, the setup/repair car preview) never runs the pass: the depth focus plane and the last
// speed are race settings, and the HUD/menu text is drawn after the pass, so only the 3D car preview would be blurred.
struct DofGate { bool depthBlur, tunnel, runPass; int reason; };  // reason: 1 dof off, 2 no depth, 3 nothing to do, 4 pass unavailable (set by caller), 6 not a race frame
inline DofGate Dof_Gate(bool dofEnabled, int focalRange, int maxBlur, float speedTarget, float speedSmooth, bool depthAvailable, bool raceFrame = true)
{
    DofGate g;
    if (!raceFrame) { g.depthBlur = false; g.tunnel = false; g.runPass = false; g.reason = 6; return g; }
    g.depthBlur = dofEnabled && focalRange > 0 && maxBlur > 0;
    g.tunnel = speedTarget > 0.0f || speedSmooth > 0.001f;
    g.runPass = (g.depthBlur || g.tunnel) && depthAvailable;
    g.reason = (!dofEnabled && !g.tunnel) ? 1 : !depthAvailable ? 2 : (!g.depthBlur && !g.tunnel) ? 3 : 4;
    return g;
}
#endif
