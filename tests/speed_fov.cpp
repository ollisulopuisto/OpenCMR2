#include "SpeedFov.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    CHECK(SpeedFov_Target(0)==0);
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
    // zoom: off and at rest unchanged, full speed 20% widens, clamp at 40
    CHECK(SpeedFov_Zoom(65536,0,65536)==65536);
    CHECK(SpeedFov_Zoom(65536,20,0)==65536);
    CHECK(SpeedFov_Zoom(65536,20,65536)==54613);
    CHECK(SpeedFov_Zoom(65536,99,65536)==SpeedFov_Zoom(65536,40,65536));
    CHECK(SpeedFov_Zoom(65536,20,65536)<65536);
    if(!fails) std::puts("speed_fov ok");
    return fails;
}
