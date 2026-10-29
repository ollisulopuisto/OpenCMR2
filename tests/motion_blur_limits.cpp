#include "render/motion_blur_limits.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
int main()
{
    CHECK(MotionBlurClampMix(-5) == 0 && MotionBlurClampMix(0) == 0 && MotionBlurClampMix(41) == 41);
    CHECK(MotionBlurClampMix(255) == kMotionBlurMixMax && MotionBlurClampMix(1000) == kMotionBlurMixMax);
    CHECK(MotionBlurClampMix(100 * 255 / 100) < 255);  // "100%" (Byte(): 100*255/100) must never reach full feedback
    // Simulate the feedback: after N frames the output has absorbed a step change of the live frame (freeze would stay at 0).
    double out = 0, m = kMotionBlurMixMax / 255.0;
    for (int i = 0; i < 60; i++) out = (1 - m) * 1.0 + m * out;
    CHECK(out > 0.9);
    double frozen = 0; for (int i = 0; i < 60; i++) frozen = 0.0 * 1.0 + 1.0 * frozen;
    CHECK(frozen == 0.0);
    std::puts("motion_blur_limits ok");
}
