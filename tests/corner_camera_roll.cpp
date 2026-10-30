#include "CornerCameraRoll.h"
#include <cstdio>
#include <initializer_list>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 using namespace CornerCameraRoll;
 for(float dt:{.04f,1.f/60,1.f/120}){
  State l,r,off,stopped;float yaw=3.f;
  for(int i=0;i<int(3/dt);i++){
   yaw+=dt;float before=l.angle;
   l.Step(Wrap(yaw),1.5f,dt,100);r.Step(Wrap(-yaw),1.5f,dt,100);off.Step(yaw,1.5f,dt,0);stopped.Step(yaw,0,dt,100);
   CHECK(std::abs(l.angle+r.angle)<.0001f);CHECK(l.angle<=.0699f);CHECK(std::abs(l.angle-before)<.22f*dt);
   CHECK(off.angle==0&&stopped.angle==0);
  }
  CHECK(l.angle>.05f);
  for(int i=0;i<int(3/dt);i++)l.Step(Wrap(yaw),1.5f,dt,100);
  CHECK(std::abs(l.angle)<.0001f);
  l.Step(yaw,1.5f,0,100);CHECK(!l.ready);
 }
 puts("optional roll off identity, mirrored turns, four-degree bound, wrap, smooth settling/reset pass");
}
