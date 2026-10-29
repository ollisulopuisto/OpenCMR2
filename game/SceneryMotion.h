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
}
#endif
