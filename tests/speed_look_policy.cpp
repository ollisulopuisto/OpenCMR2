#include "SpeedLookPolicy.h"
#include "CameraOperator.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    for (int type = 0; type <= 10; type++) {
        CHECK(!SpeedLook_ScreenEffectsOff(type, false, false));        // driving keeps its effects in every normal view
        CHECK(SpeedLook_ScreenEffectsOff(type, false, true));          // drone off while driving
        CHECK(SpeedLook_ScreenEffectsOff(type, true, true));           // and in replay
        CHECK(SpeedLook_ScreenEffectsOff(type, true, false) == (type != 3)); // replay: only the interior view keeps them
    }
    CHECK(SpeedLook_VaporOff(true) && !SpeedLook_VaporOff(false));   // replay: vapor off in every view; driving: as set
    CHECK(SpeedLook_ScreenEffectsOff(4, false, false, false) && SpeedLook_ScreenEffectsOff(3, true, false, false));   // menu switch off: every view, driving and replay
    CHECK(!SpeedLook_ScreenEffectsOff(4, false, false, true) && !SpeedLook_ScreenEffectsOff(4, false, false));       // default on: unchanged
    // Drone lens: with the speed gain at 0 the lens must not depend on speed (the old gain 0.3 did).
    auto settled = [](float gain, float speed) {
        CameraOperator::State s; CameraOperator::V3 car{0, 0, 0}; CameraOperator::RoadPreview r;
        s.lensSpeedGain = gain;
        for (int i = 0; i < 400; i++) s.Step(car, 0, 0, speed, 6.5f, 2.f, .04f, &r);
        return s.zoom;
    };
    CHECK(settled(.3f, 1.5f) != settled(.3f, 0.f));   // red check: the old gain reacts to speed
    CHECK(settled(0.f, 1.5f) == settled(0.f, 0.f));   // green: no speed reaction
    puts("speed look policy ok");
    return 0;
}
