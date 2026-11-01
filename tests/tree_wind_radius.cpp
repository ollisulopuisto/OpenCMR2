#include "SceneryMotion.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#define Sys_Log(...) do{ std::printf(__VA_ARGS__); std::printf("\n"); }while(0)
// todo-01M6A4B9F5F35RK2KP8301M539 (rainy replay 14-22 fps): distance falloff
// for the tree wind math - ampScale fades both sway and gust rustle, and the
// caller skips WindPart entirely beyond the radius (static draw path).
int main(){
    using namespace SceneryMotion;
    // Baseline: ampScale defaults to 1 -> identical displacement to before.
    WindVertex wv=MakeWindVertex(0.5f,0.5f,1.f,0.f,10.f);
    WindFrame f=MakeWindFrame(10.f,1234567,777,1.f);
    float x0,u0,v0,x1,u1,v1;
    ApplyWind(wv,f,&x0,&u0,&v0);
    WindFrame g=f; g.ampScale=0.25f;
    ApplyWind(wv,g,&x1,&u1,&v1);
    assert(std::fabs(x1-x0*0.25f)<1e-6f);
    assert(std::fabs(u1-u0*0.25f)<1e-6f);
    assert(std::fabs(v1-v0*0.25f)<1e-6f);
    // Zero scale -> no displacement at all (matches the static draw path).
    g.ampScale=0.f; ApplyWind(wv,g,&x1,&u1,&v1);
    assert(x1==0.f&&u1==0.f&&v1==0.f);
    // Falloff curve used by the caller: full to 60% of radius, linear to zero.
    auto scale=[&](float d,float r){ if(d>=r)return 0.f; float full=r*0.6f; if(d<=full)return 1.f; return (r-d)/(r-full); };
    assert(scale(0.f,150.f)==1.f&&scale(89.9f,150.f)==1.f);
    float mid=scale(120.f,150.f); assert(mid>0.4f&&mid<0.6f);
    assert(scale(150.f,150.f)==0.f&&scale(200.f,150.f)==0.f);
    Sys_Log("tree_wind_radius ok");
    return 0;
}
