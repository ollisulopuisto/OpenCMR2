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

// Irregular crowd sway. One card holds many people, so the sway varies smoothly across the card
// (u = 0..1 along its width): two waves with different tempos and wave numbers give neighbours different
// phases and speeds. Vertices at the same u move the same, so the card never tears. Feet stay fixed.
// Same 2.5 percent bound as Offset(). tempo varies per placement as well (0.75 to 1.3 times).
inline float PersonTempo(uint32_t phase){return 0.75f+0.55f*float((phase*2654435761u)>>8&0xffff)/65535.f;}
inline float PersonOffset(float y,float bottom,float height,float u,uint64_t ticks,uint32_t phase){
 if(!std::isfinite(y)||!std::isfinite(bottom)||!std::isfinite(height)||height<=0||!std::isfinite(u))return 0;
 float weight=(y-bottom)/height;if(weight<0)weight=0;if(weight>1)weight=1;
 float time=float((ticks%(65536ull*25*3600))/65536.0/25.0),p=(phase%6283)*.001f,tempo=PersonTempo(phase);
 float a=std::sin(time*2.4f*tempo+p+u*19.f),b=std::sin(time*3.7f*tempo+p*1.7f+u*31.f+1.3f);
 return (0.6f*a+0.4f*b)*height*.025f*weight*weight;
}
inline float ReactivePersonOffset(float y,float bottom,float height,float u,uint64_t ticks,uint32_t phase,float excitement){
 float base=PersonOffset(y,bottom,height,u,ticks,phase);
 if(!std::isfinite(excitement)||excitement<=0||height<=0||!std::isfinite(y)||!std::isfinite(bottom)||!std::isfinite(u))return base;
 if(excitement>1.f)excitement=1.f;
 float weight=(y-bottom)/height;if(weight<0)weight=0;if(weight>1)weight=1;
 float time=float((ticks%(65536ull*25*3600))/65536.0/25.0);
 return base*(1.f+2.f*excitement)+std::sin(time*9.f*PersonTempo(phase)+(phase%6283)*.001f+u*23.f)*height*.03f*excitement*weight*weight;
}

// Foliage rustle by moving the texture, not the geometry: a small smooth shift of the texture coordinates
// (u,v in 0..1 on the texture), crown only, in gusts. strength 1.0 = default size (about 0.8 percent of the texture).
// weightY = vertex height above the tree base / tree height. The trunk (low weightY) and the texture edges
// (where wrap or clamp would show other texels) do not move. Offsets depend only on (u,v), so vertices that
// share a texture coordinate move together and the mesh cannot crack.
inline void RustleUV(float u,float v,float weightY,uint64_t ticks,uint32_t phase,float strength,float *du,float *dv){
 *du=0;*dv=0;
 if(!std::isfinite(u)||!std::isfinite(v)||!std::isfinite(weightY)||!std::isfinite(strength)||strength<=0)return;
 float crown=(weightY-.35f)/.3f;if(crown<=0)return;if(crown>1.f)crown=1.f;
 float edge=std::fmin(std::fmin(u,1.f-u),std::fmin(v,1.f-v))/.06f;if(edge<=0)return;if(edge>1.f)edge=1.f;
 float time=float((ticks%(65536ull*25*3600))/65536.0/25.0),p=(phase%6283)*.001f;
 float gust=.35f+.65f*(.5f+.5f*std::sin(time*.45f+p)*std::sin(time*.19f+p*.6f+1.f));
 float amp=.008f*strength*gust*crown*edge;
 *du=amp*(.6f*std::sin(time*5.3f+u*37.f+v*23.f+p)+.4f*std::sin(time*8.9f-u*53.f+v*41.f+p*1.3f));
 *dv=amp*(.6f*std::sin(time*4.7f+u*29.f-v*43.f+p*.7f)+.4f*std::sin(time*7.7f+u*47.f+v*31.f+p*1.9f));
}
}
#endif
