#ifndef OPENCMR2_MOTION_BLUR_LIMITS_H
#define OPENCMR2_MOTION_BLUR_LIMITS_H
// Motion blur feeds its own output back as the "previous frame": out = mix(current, previous, m); previous = out.
// At m = 1.0 (255) the output is the previous frame forever, so the picture freezes. Keep a little of the new frame:
// 242/255 (about 0.949) is the strongest mix that still updates (the picture converges on the live frame in a few dozen frames).
static const int kMotionBlurMixMax = 242;
inline int MotionBlurClampMix(int mix) { return mix < 0 ? 0 : mix > kMotionBlurMixMax ? kMotionBlurMixMax : mix; }
#endif
