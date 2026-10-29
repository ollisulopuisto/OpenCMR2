#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Heading along +x, then an arbitrary heading: spawn is ahead along it (dot > 0) and 4..14 to the side, whatever the heading and random numbers.
    const float heads[][2] = {{1,0},{0,1},{-1,0},{0,-1},{0.6f,0.8f},{-0.3f,0.95f},{5,0}};   // length does not matter
    for (auto &h : heads) for (float u1 : {0.f, .5f, 1.f}) for (float u2 : {0.f, .5f, 1.f}) for (float u3 : {0.f, .9f}) {
        float dx, dz; CHECK(SpeedMote_SpawnOffset(h[0], h[1], 1.2f, u1, u2, u3, &dx, &dz));
        float len = std::sqrt(h[0]*h[0] + h[1]*h[1]), ux = h[0]/len, uz = h[1]/len;
        float along = dx*ux + dz*uz, across = std::fabs(-dx*uz + dz*ux);
        CHECK(along >= 12.f - 1e-3f && along <= 1.2f*36.f + 1e-3f);          // ahead
        CHECK(across >= 4.f - 1e-3f && across <= 14.f + 1e-3f);              // beside the road, not 27+ units out
    }
    // The old bug: feeding the side axis (Car::forward) as the heading puts the mote across the road. With the right heading it never does.
    { float dx, dz; SpeedMote_SpawnOffset(1,0,1.5f,.5f,.5f,.9f,&dx,&dz); CHECK(std::fabs(dx) > 20.f && std::fabs(dz) < 15.f); }   // 40 ahead along x, 9 to the side in z
    { float dx, dz; CHECK(!SpeedMote_SpawnOffset(0.f,0.05f,1.f,.5f,.5f,.5f,&dx,&dz)); }   // vertical or stalled heading: no spawn
    if (!fails) std::puts("speed_mote_spawn ok");
    return fails;
}
