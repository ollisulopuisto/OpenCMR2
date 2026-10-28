#ifndef OPENCMR2_WORLD_DUST_H
#define OPENCMR2_WORLD_DUST_H
#include "port/gfx.h"
#include <cmath>
// Dedicated visual world-space pool. Birth anchor and a bounded emission
// impulse are captured once. No owner/camera pointer or later car velocity.
namespace WorldDust {
constexpr int Capacity=128;
struct Cloud {float x=0,y=0,z=0,age=0,r=0,g=0,b=0,strength=0,vx=0,vz=0;};
struct Pool {
 Cloud clouds[Capacity]={};unsigned next=0;
 void Clear(){for(auto &c:clouds)c={};next=0;}
 void Emit(float x,float y,float z,float r,float g,float b,float vx=0,float vz=0){
  for(auto &c:clouds)if(c.strength>0&&c.age<1.f){float dx=x-c.x,dz=z-c.z;
   if(dx*dx+dz*dz<16.f){c.strength=std::fmin(1.f,c.strength+.12f);return;}}
  auto &c=clouds[next++%Capacity];c={x,y,z,0,r,g,b,.65f,vx,vz};
  // Visual kick, not a physics force. Bound extreme replay/impact velocities.
  float speed=std::sqrt(vx*vx+vz*vz);
  if(speed>8.f){c.vx*=8.f/speed;c.vz*=8.f/speed;}
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
   // Exponential drag gives a horizontal wake that stops, not a moving
   // cloud attached to the car. Analytic age sampling is timestep independent.
   float travel=.85f*(1.f-std::exp(-t/.85f));
   l.centreRadius[0]=c.x+c.vx*travel;
   // Lift no longer grows in lockstep with radius into a vertical column.
   l.centreRadius[1]=c.y+.35f+std::fmin(t,5.f)*.22f;
   l.centreRadius[2]=c.z+c.vz*travel;l.centreRadius[3]=radius;
   l.colourDensity[0]=c.r;l.colourDensity[1]=c.g;l.colourDensity[2]=c.b;
   l.colourDensity[3]=c.strength*(.7f+.6f*variety)*std::fmin(1.f,t*3.f)*std::fmax(0.f,1.f-t/12.f);
  }return n;
 }
};
}
#endif
