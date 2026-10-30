#include "SpeedFov.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    CHECK(SpeedFov_Target(0)==0);
    CHECK(SpeedFov_HeldRatio(0.f,1)==1.f&&SpeedFov_HeldRatio(0.37f,0)==0.37f&&SpeedFov_HeldRatio(0.37f,1)==1.f);   // tunnel hold only forces the ratio
    {   // Projection maths, same formulas as View_SetPlayerProjection (FixedPoint.cpp) and CGraphics::SetProjection (scale 0.5):
        // fovY = FixMul(0x2147a, FixMul(zoom, fovScale)); _22 = 0.5 * fovY. A car of height h at a fixed camera distance d
        // spans _22*h/d of the half screen, so its pixel size scales with _22 and the vertical angle is 2*atan(1/_22).
        auto fixmul=[](int a,int b){return (int)(((long long)a*b)>>16);};
        const int baseZoom=40632, fovScale=(int)(0x275c2*70.0/80.0);   // OSD base zoom 0.62; video.fov 80
        auto m22=[&](int zoom){return 0.5f*fixmul(0x2147a,fixmul(zoom,fovScale))/65536.f;};
        float rest=m22(SpeedFov_Zoom(baseZoom,0,0)), held=m22(SpeedFov_Zoom(baseZoom,40,SpeedFov_HeldTarget(0,1)));
        CHECK(std::fabs(rest-1.39f)<0.02f);                                // proj scale about 2.78 at rest, implied by the OSD frames
        CHECK(std::fabs(held/rest-1.f/1.4f)<0.005f);                       // car pixel size and scenery scale by 1/1.4
        float restDeg=2*std::atan(1/rest)*57.29578f, heldDeg=2*std::atan(1/held)*57.29578f;
        CHECK(restDeg>70.f&&restDeg<73.f&&heldDeg>88.f&&heldDeg<92.f);     // vertical field of view about 71 to 90 degrees
        float real=m22(SpeedFov_Zoom(baseZoom,40,44564));                  // eased speed 0.68 as in a native frame
        CHECK(std::fabs(real*2-2.19f)<0.05f);                              // OSD proj scale 2.19
    }
    CHECK(SpeedFov_HeldTarget(0,1)==65536&&SpeedFov_HeldTarget(1234,0)==1234&&SpeedFov_HeldTarget(1234,1)==65536);   // hold only forces the target
    CHECK(SpeedFov_Target(-65536)==0);                 // reversing
    CHECK(SpeedFov_Target(98304)==65536);              // 1.5/step = full
    CHECK(SpeedFov_Target(1<<30)==65536);              // clamp
    int h=SpeedFov_Target(49152); CHECK(h>32000&&h<33500);
    // ease: monotonic toward target, never overshoots, snaps on pause/first frame
    int s=0; for(int i=0;i<200;i++){ int n=SpeedFov_Ease(s,65536,16); CHECK(n>=s&&n<=65536); s=n; }
    CHECK(s>65000);
    s=65536; for(int i=0;i<200;i++){ int n=SpeedFov_Ease(s,0,16); CHECK(n<=s&&n>=0); s=n; }
    CHECK(s<600);
    CHECK(SpeedFov_Ease(1234,50000,0)==1234);          // zero elapsed: no change
    CHECK(SpeedFov_Ease(1234,50000,251)==50000);
    CHECK(SpeedFov_Ease(1234,50000,-5)==50000);
    // roughly half a second to cover most of the gap
    s=0; for(int i=0;i<30;i++) s=SpeedFov_Ease(s,65536,16); CHECK(s>35000&&s<50000);
    // zoom: off and at rest unchanged, full speed 20% widens, clamp at the max
    CHECK(SpeedFov_Zoom(65536,0,65536)==65536);
    CHECK(SpeedFov_Zoom(65536,20,0)==65536);
    CHECK(SpeedFov_Zoom(65536,20,65536)==54613);
    CHECK(SpeedFov_Zoom(65536,999,65536)==SpeedFov_Zoom(65536,kSpeedFovMaxPct,65536) && SpeedFov_Zoom(65536,100,65536)==32768);
    CHECK(SpeedFov_Zoom(65536,20,65536)<65536);
    {   // testing range above 150: finite, monotonic, never zero or negative, view stays under 180 degrees at the widest saved/preview combination
        CHECK(kSpeedFovMaxPct==500);
        auto fixmul=[](int a,int b){return (int)(((long long)a*b)>>16);};
        const int fovScale=(int)(0x275c2*70.0/80.0);
        int prev=1<<30;
        for(int pct=0;pct<=500;pct+=5){
            int z=SpeedFov_Zoom(40632,pct,65536);CHECK(z>0&&z<=prev);prev=z;
            float m22=0.5f*fixmul(0x2147a,fixmul(z,fovScale))/65536.f;
            CHECK(m22>0.05f);                                            // far above fixed-point resolution (1/65536)
            CHECK(2*std::atan(1/m22)*57.29578f<179.f);                  // finite projection, under 180 degrees
        }
        CHECK(SpeedFov_Zoom(65536,500,65536)==10922);   // x6 at the top
        CHECK(SpeedFov_Zoom(65536,501,65536)==SpeedFov_Zoom(65536,500,65536));
    }
    if(!fails) std::puts("speed_fov ok");
    return fails;
}
