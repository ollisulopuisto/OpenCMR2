#ifndef OPENCMR2_TUNNEL_VISION_H
#define OPENCMR2_TUNNEL_VISION_H
// Tunnel vision look (visual only). The peripheral blur radius at full strength is this fraction of the back buffer height
// (0.03 is about 43 px at 1440 lines), and dof.frag keeps a clear circle that closes from 1.4 to kTunnelInnerFull (screen
// half-heights, aspect corrected) as the strength goes 0 to 1: dof.frag has the same numbers (mix(1.4, 0.5, tunnel)).
// Before: 0.014 and 0.7. Offline pixels on a native cockpit frame showed that look only softened the outer quarter of the frame by about 20 px,
// which a native test could not tell from tunnel vision off. Taps are sparse above about 60 px, so keep this below 0.04.
constexpr float kTunnelRadiusPerHeight = 0.03f;
constexpr float kTunnelInnerFull = 0.5f;
#endif
