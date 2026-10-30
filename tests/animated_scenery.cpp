// OpenCMR2: animated scenery idle cycles (crowds, trees) — texture frame
// cycling driven by [video] video.animated_scenery. Pure helpers are
// unit-tested here; the draw-loop wiring lives in Game.cpp.
#include "GraphicsOptions.h"
#include "SceneryMotion.h"
#include <cstdio>
#include <cmath>

static int failures;
static void Check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    using SceneryMotion::Offset;
    Check(Offset(0,0,10,0,1000,true)==0,"tree base fixed");
    Check(Offset(0,0,10,0,1000,false)==0,"all people feet fixed");
    Check(std::abs(Offset(10,0,10,0,1000,true))>.3f,"stronger tree crown motion");
    Check(std::abs(Offset(10,0,10,0,1000,false))>.2f,"visible spectator motion");
    Check(Offset(10,0,10,0,1000,true)==Offset(10,0,10,0,1000,true),"draws don't advance time");
    Check(Offset(10,0,0,0,1000,true)==0,"invalid extent refused");
    for(unsigned t=0;t<10000;t+=37){
        Check(std::abs(Offset(10,0,10,t*65536ull,1234,true))<=.450001f,"tree bounded 4.5 percent");
        Check(std::abs(Offset(10,0,10,t*65536ull,1234,false))<=.250001f,"people bounded 2.5 percent");
    }
    // Frame advance: timer counts down, frame wraps within frameCount.
    struct { int timer; int frame; int count; int wantFrame; int wantTimer; } cases[] = {
        { 5,  0, 4, 0, 4 },   // tick, no wrap
        { 1,  0, 4, 1, 5 },   // wrap to next frame, reset timer
        { 1,  3, 4, 0, 5 },   // wrap back to 0 at end
        { 10, 2, 4, 2, 9 },   // slow timer, no change
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int frame = cases[i].frame;
        int timer = cases[i].timer;
        frame = AnimFrame_Advance(frame, cases[i].count, &timer);
        Check(frame == cases[i].wantFrame, "anim frame advance");
        Check(timer == cases[i].wantTimer, "anim timer reset");
    }

    // Animated texture index: base + frame.
    Check(AnimTexture_Index(10, 0) == 10, "anim texture base frame 0");
    Check(AnimTexture_Index(10, 3) == 13, "anim texture base + 3");

    {
        using namespace SceneryMotion;
        Check(Excitement(0,0,40)>.99f,"adjacent fast car fully excites");
        Check(Excitement(100,0,40)==0,"far car does nothing");
        Check(Excitement(10,0,40)>Excitement(25,0,40),"closer is more excited");
        Check(Excitement(5,0,0)<Excitement(5,0,40),"faster is more excited");
        Check(Excitement(NAN,0,1)==0&&Excitement(0,0,INFINITY)==0,"non-finite refused");
        Check(ReactiveOffset(0,0,10,12345,77,1)==0,"reactive feet stay fixed");
        Check(ReactiveOffset(10,0,10,0,1000,0)==Offset(10,0,10,0,1000,false),"zero excitement unchanged");
        for(unsigned t=0;t<5000;t+=29)Check(std::abs(ReactiveOffset(10,0,10,t*65536ull,1234,1))<=1.0501f,"reactive bounded 10.5 percent");
    }
    {
        using namespace SceneryMotion;
        int differ = 0;
        for (float u = 0; u < 1; u += .1f) if (std::fabs(PersonOffset(10,0,10,u,3*65536ull,5) - PersonOffset(10,0,10,0,3*65536ull,5)) > .05f) differ++;
        Check(differ >= 4, "crowd sway differs along the card");
        Check(PersonTempo(5) != PersonTempo(9999), "tempo differs per placement");
        Check(PersonOffset(0,0,10,.4f,123456,7) == 0 && ReactivePersonOffset(0,0,10,.4f,123456,7,1) == 0, "people feet fixed");
        for (unsigned t = 0; t < 20000; t += 17) for (float u = 0; u <= 1; u += .05f) {
            Check(std::fabs(PersonOffset(10,0,10,u,t*65536ull,1234)) <= .250001f, "person sway bounded 2.5 percent");
            float du, dv;
            RustleUV(u,.6f,.9f,t*65536ull,99,1,&du,&dv);
            Check(std::fabs(du) <= .0081f && std::fabs(dv) <= .0081f, "rustle bounded 0.8 percent of the texture");
            RustleUV(u,.6f,.2f,t*65536ull,99,1,&du,&dv);
            Check(du == 0 && dv == 0, "trunk texture does not move");
        }
        float du, dv;
        RustleUV(0.f,.5f,.9f,5*65536ull,1,1,&du,&dv);
        Check(du == 0 && dv == 0, "texture edge does not move");
        RustleUV(.5f,.5f,.9f,5*65536ull,1,0,&du,&dv);
        Check(du == 0 && dv == 0, "strength 0 is off");
    }
    if (failures == 0) {
        puts("animated scenery: frame advance and texture indexing hold");
        return 0;
    }
    return 1;
}
