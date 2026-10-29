#ifndef OPENCMR2_SCENERY_MOTION_H
#define OPENCMR2_SCENERY_MOTION_H
#include <cmath>
#include <cstdint>
// Stage simulation clock only. Source geometry remains immutable; base fixed.
namespace SceneryMotion {
inline float Offset(float y,float bottom,float height,uint64_t ticks,uint32_t phase,bool tree){
 if(!std::isfinite(y)||!std::isfinite(bottom)||!std::isfinite(height)||height<=0)return 0;
 float weight=(y-bottom)/height;if(weight<0)weight=0;if(weight>1)weight=1;
 float time=float((ticks%(65536ull*25*3600))/65536.0/25.0);
 return std::sin(time*(tree?1.2f:2.4f)+(phase%6283)*.001f)*height*(tree?.045f:.025f)*weight*weight;
}

// Reactive spectators: excitement 0..1 from the player car's distance (world
// units) and speed. Constants are untuned guesses; no native Mac check yet.
inline float Excitement(float dx,float dz,float speed){
 if(!std::isfinite(dx)||!std::isfinite(dz)||!std::isfinite(speed))return 0;
 const float radius=35.f;
 float d=std::sqrt(dx*dx+dz*dz);if(d>=radius)return 0;
 float near=1.f-d/radius;
 float fast=std::fabs(speed)/30.f;if(fast<.25f)fast=.25f;if(fast>1.f)fast=1.f;
 return near*near*fast;
}
// Same feet-fixed weighting as Offset: larger sway plus a head bob, scaled by excitement.
inline float ReactiveOffset(float y,float bottom,float height,uint64_t ticks,uint32_t phase,float excitement){
 float base=Offset(y,bottom,height,ticks,phase,false);
 if(!std::isfinite(excitement)||excitement<=0||height<=0||!std::isfinite(y)||!std::isfinite(bottom))return base;
 if(excitement>1.f)excitement=1.f;
 float weight=(y-bottom)/height;if(weight<0)weight=0;if(weight>1)weight=1;
 float time=float((ticks%(65536ull*25*3600))/65536.0/25.0);
 return base*(1.f+2.f*excitement)+std::sin(time*9.f+(phase%6283)*.001f)*height*.03f*excitement*weight*weight;
}
}
#endif
