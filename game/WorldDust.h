#ifndef OPENCMR2_WORLD_DUST_H
#define OPENCMR2_WORLD_DUST_H
#include "port/gfx.h"
#include <cmath>
// Dedicated visual world-space pool. No owner/camera pointer or inherited car
// velocity. Ground anchor is captured once at wheel contact and never rebased.
namespace WorldDust {
constexpr int Capacity=128;
struct Cloud {float x=0,y=0,z=0,age=0,r=0,g=0,b=0,strength=0;};
struct Pool {
 Cloud clouds[Capacity]={};unsigned next=0;
 void Clear(){for(auto &c:clouds)c={};next=0;}
 void Emit(float x,float y,float z,float r,float g,float b){
  for(auto &c:clouds)if(c.strength>0&&c.age<1.f){float dx=x-c.x,dz=z-c.z;
   if(dx*dx+dz*dz<16.f){c.strength=std::fmin(1.f,c.strength+.12f);return;}}
  auto &c=clouds[next++%Capacity];c={x,y,z,0,r,g,b,.65f};
 }
 void Step(float dt){for(auto &c:clouds)if(c.strength>0){c.age+=dt;if(c.age>=12.f)c={};}}
 int Sample(GfxDustLobe *out,int max)const{
  if(!out||max<=0)return 0;
  int active=0;for(const auto &c:clouds)if(c.strength>0)active++;
  int n=0,index=0,last=-1;
  // Evenly retain the old trail across the bounded pool, not just the first
  // visible/nearest particles. Camera visibility never kills a stored cloud.
  for(const auto &c:clouds)if(c.strength>0){
   int bucket=index++*max/(active?active:1);
   if(bucket==last||n>=max)continue;last=bucket;
   auto &l=out[n++];float t=c.age;
   // Position-derived variety is fixed for the cloud's lifetime. No camera
   // term and no global random stream: alternate billows break a smooth tube.
   float variety=.5f+.5f*std::sin(c.x*4.13f+c.z*7.71f);
   float radius=(1.5f+std::fmin(t,8.f)*1.25f)*(.8f+.5f*variety);
   l.centreRadius[0]=c.x;l.centreRadius[1]=c.y+radius*(.38f+.18f*variety);l.centreRadius[2]=c.z;l.centreRadius[3]=radius;
   l.colourDensity[0]=c.r;l.colourDensity[1]=c.g;l.colourDensity[2]=c.b;
   l.colourDensity[3]=c.strength*(.7f+.6f*variety)*std::fmin(1.f,t*3.f)*std::fmax(0.f,1.f-t/12.f);
  }return n;
 }
};
}
#endif
