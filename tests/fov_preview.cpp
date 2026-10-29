#include "FovPreview.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    CHECK(FovPreview_Effective(0, 80) == 80 && FovPreview_Effective(0, 10) == 30 && FovPreview_Effective(0, 400) == 150);  // off: saved setting, old clamp
    CHECK(FovPreview_Effective(25, 80) == 25 && FovPreview_Effective(40, 80) == 40 && FovPreview_Effective(75, 80) == 75 && FovPreview_Effective(90, 80) == 90);
    CHECK(FovPreview_Effective(5, 80) == 20 && FovPreview_Effective(200, 80) == 150);   // preview below 20 acts as 20
    CHECK(std::fabs(FovPreview_Scale(70) - 0x275c2) < 1e-6);                        // 70 is the original
    CHECK(FovPreview_Scale(90) < FovPreview_Scale(70) && FovPreview_Scale(25) > FovPreview_Scale(70));  // higher number, smaller zoom scale = wider
    CHECK(std::fabs(FovPreview_Magnification(40, 80) - 2.0) < 1e-9 && std::fabs(FovPreview_Magnification(90, 80) - 80.0 / 90) < 1e-9);
    // Measured angle: vertical = 2 atan(2 / fovY). Native cockpit frames: proj 2.79 -> 71.2 deg, 2.32 -> 81.6 deg (saved video.fov 80, base zoom 0.62, Speed FOV 40 at eased 51).
    CHECK(std::fabs(FovPreview_MeasuredDegrees(2.79) - 71.2) < 0.1 && std::fabs(FovPreview_MeasuredDegrees(2.32) - 81.6) < 0.1);
    CHECK(FovPreview_MeasuredDegrees(0) == 0 && FovPreview_MeasuredDegrees(4.0) < FovPreview_MeasuredDegrees(2.0));   // bigger proj scale = narrower
    CHECK(std::fabs(FovPreview_MeasuredDegrees(2.0) - 90.0) < 1e-9);   // proj 2 -> matrix element 1 = tan 45 -> 90 deg
    if (!fails) std::puts("fov_preview ok");
    return fails;
}
