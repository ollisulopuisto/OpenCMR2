#ifndef OPENCMR2_TUNNEL_VISION_H
#define OPENCMR2_TUNNEL_VISION_H
// Tunnel vision look (visual only). The peripheral blur radius at full strength is this fraction of the back buffer height
// (0.03 is about 43 px at 1440 lines), and dof.frag keeps a clear circle that closes from 1.4 to kTunnelInnerFull (screen
// half-heights, aspect corrected) as the strength goes 0 to 1: dof.frag has the same numbers (mix(1.4, 0.5, tunnel)).
// Before: 0.014 and 0.7. Offline pixels on a native cockpit frame showed that look only softened the outer quarter of the frame by about 20 px,
// which a native test could not tell from tunnel vision off. Taps are sparse above about 60 px, so keep this below 0.04.
constexpr float kTunnelRadiusPerHeight = 0.03f;
constexpr float kTunnelInnerFull = 0.5f;
// The clear circle is squashed vertically (distance uses y * this, orientation free so Metal and GL agree), so the bonnet at the
// bottom centre and a chase car low in the frame stay sharp while the left and right edges blur.
constexpr float kTunnelVerticalScale = 0.5f;
// Experimental overdrive: video.tunnel_vision goes up to 150 (percent). The strength then reaches 1.5, the clear circle keeps closing past
// kTunnelInnerFull (never below kTunnelInnerMin half-heights) and the blur radius grows with it (0.045 of the height at 150, 65 px at 1440 lines:
// the 16 taps get sparse there). 100 and below look exactly as before. The depth-focus range still uses the strength capped at 1.
constexpr float kTunnelMaxStrength = 1.5f;
constexpr float kTunnelInnerMin = 0.1f;
#endif
