#include "ReplayChaseOrbit.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 ReplayChaseOrbit o;CHECK(o.Chase(4)&&o.Chase(5)&&o.Chase(6)&&!o.Chase(3)&&!o.Chase(7)&&!o.Chase(11));
 for(int k:{0xc8,0xd0,0xcb,0xcd,0x13})CHECK(o.Key(k));CHECK(!o.Key(0x11));
 double p[3]={0,2,-8},r[3]={1,0,0},u[3]={0,1,0},f[3]={0,0,1};o.Apply(p,r,u,f,8);CHECK(p[0]==0&&p[1]==2&&p[2]==-8);
 for(int i=0;i<100;i++)o.Step(false,true,true,false,.016);CHECK(o.pitch==1.3);
 o.Apply(p,r,u,f,8);CHECK(p[1]>2);double d=0;for(int i=0;i<3;i++){double target=(i==1?2:0);d+=(p[i]-target)*(p[i]-target);CHECK(std::abs(p[i]+8*f[i]-target)<1e-9);}CHECK(std::abs(d-64)<1e-8);
 double dot=0,rr=0,ff=0;for(int i=0;i<3;i++){dot+=r[i]*f[i];rr+=r[i]*r[i];ff+=f[i]*f[i];}CHECK(std::abs(dot)<1e-9&&std::abs(rr-1)<1e-9&&std::abs(ff-1)<1e-9);
 o.Reset();CHECK(o.yaw==0&&o.pitch==0);o.Step(true,true,true,true,.1);CHECK(o.yaw==0&&o.pitch==0);o.Step(false,true,false,false,100);CHECK(std::abs(o.yaw-.14)<1e-9);
 o.Reset();o.Step(false,true,false,false,.1);double p2[3]={0,0,-8},r2[3]={-1,0,0},u2[3]={0,1,0},f2[3]={0,0,1};o.Apply(p2,r2,u2,f2,8);CHECK(p2[0]>0); // screen-right orbit with reversed camera handedness
 puts("orbit geometry: chase4/5/6, aim/distance/orthonormality, identity/reset, clamp/opposite keys/handedness pass");
}
