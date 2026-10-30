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

// Cached form of Offset() + RustleUV() for one tree vertex. Everything that depends only on the vertex (its height weight and texture
// coordinates) is computed once; per tree and frame only a few sin/cos remain, so the per-vertex loop has no trigonometry.
// sin(T+X) = sin(T)cos(X) + cos(T)sin(X) with T the time/phase term and X the (u,v) term.
struct WindVertex{float u,v,w2,ce,s1,c1,s2,c2,s3,c3,s4,c4;};
inline WindVertex MakeWindVertex(float u,float v,float y,float bottom,float height){
 WindVertex o={u,v,0,0,0,1,0,1,0,1,0,1};
 if(std::isfinite(y)&&std::isfinite(bottom)&&std::isfinite(height)&&height>0){float w=(y-bottom)/height;if(w<0)w=0;if(w>1)w=1;o.w2=w*w;}
 float weightY=height>0?(y-bottom)/height:0;
 if(!std::isfinite(u)||!std::isfinite(v)||!std::isfinite(weightY))return o;
 float crown=(weightY-.35f)/.3f;if(crown<=0)return o;if(crown>1.f)crown=1.f;
 float edge=std::fmin(std::fmin(u,1.f-u),std::fmin(v,1.f-v))/.06f;if(edge<=0)return o;if(edge>1.f)edge=1.f;
 o.ce=crown*edge;
 float x1=u*37.f+v*23.f,x2=-u*53.f+v*41.f,x3=u*29.f-v*43.f,x4=u*47.f+v*31.f;
 o.s1=std::sin(x1);o.c1=std::cos(x1);o.s2=std::sin(x2);o.c2=std::cos(x2);o.s3=std::sin(x3);o.c3=std::cos(x3);o.s4=std::sin(x4);o.c4=std::cos(x4);
 return o;
}
// Per tree and frame.
struct WindFrame{float offset,gustAmp,s1,c1,s2,c2,s3,c3,s4,c4;};
inline WindFrame MakeWindFrame(float height,uint64_t ticks,uint32_t phase,float strength){
 WindFrame f;float time=float((ticks%(65536ull*25*3600))/65536.0/25.0),p=(phase%6283)*.001f;
 f.offset=std::sin(time*1.2f+p)*height*.045f;
 float gust=.35f+.65f*(.5f+.5f*std::sin(time*.45f+p)*std::sin(time*.19f+p*.6f+1.f));
 f.gustAmp=(std::isfinite(strength)&&strength>0)?.008f*strength*gust:0.f;
 float t1=time*5.3f+p,t2=time*8.9f+p*1.3f,t3=time*4.7f+p*.7f,t4=time*7.7f+p*1.9f;
 f.s1=std::sin(t1);f.c1=std::cos(t1);f.s2=std::sin(t2);f.c2=std::cos(t2);f.s3=std::sin(t3);f.c3=std::cos(t3);f.s4=std::sin(t4);f.c4=std::cos(t4);
 return f;
}
inline void ApplyWind(const WindVertex&w,const WindFrame&f,float *dx,float *du,float *dv){
 *dx=f.offset*w.w2;*du=0;*dv=0;
 if(w.ce<=0||f.gustAmp<=0)return;
 float amp=f.gustAmp*w.ce;
 *du=amp*(.6f*(f.s1*w.c1+f.c1*w.s1)+.4f*(f.s2*w.c2+f.c2*w.s2));
 *dv=amp*(.6f*(f.s3*w.c3+f.c3*w.s3)+.4f*(f.s4*w.c4+f.c4*w.s4));
}
// One part of one tree: dst[k]=src[k] moved by the cached wind terms. Pure function of its inputs, so
// it can run on any thread and gives the same bytes in any order.
template<class V>
inline void WindPart(const V *src,V *dst,const WindVertex *w,size_t n,const WindFrame &f){
 for(size_t k=0;k<n;k++){float dx,du,dv;ApplyWind(w[k],f,&dx,&du,&dv);dst[k]=src[k];dst[k].x+=dx;dst[k].u0+=du;dst[k].v0+=dv;}
}
}
#endif
