#include "CameraOperator.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 using namespace CameraOperator;State a,b;
 V3 car={0,0,0};a.Step(car,0,0,1.5,6.5,2,.04);auto first=a.position;
 a.Step(car, .6f,1,1.5,6.5,2,.04);
 CHECK((a.position-first).Length()<.01f);CHECK(std::abs(a.heading)<.01f); // independent orbit, not attached
 float initialZoom=a.zoom;
 for(int i=0;i<60;i++)a.Step(car,.6f,1,1.5,6.5,2,.04);
 CHECK(a.zoom<initialZoom-.1f);
 for(int i=0;i<250;i++)a.Step(car,.6f,0,1.5,6.5,2,.04);
 CHECK(a.zoom>1.8f);CHECK(std::abs(Wrap(a.heading-.6f))<.03f);
 a.Reset();b.Reset();
 for(int i=0;i<150;i++){
  car.z=i*.2f;float h=i>40?.5f:0;
  a.Step(car,h,0,1,6.5,2,.04);b.Step(car,h,0,1,6.5,2,.04);
  CHECK(a.position.x==b.position.x&&a.zoom==b.zoom);CHECK(a.zoom>=1.15f&&a.zoom<=1.9f);
 }
 a.Step({1000,0,1000},.5,0,1,6.5,2,.04);CHECK((a.position-V3{1000,0,1000}).Length()<30);
 a.Step(car,NAN,0,1,6.5,2,.04);CHECK(!a.ready);
 a.Step(car,0,0,1,6.5,2,0);CHECK(!a.ready);
 puts("world camera position/orbit inertia, steering lens, settling, determinism/reset pass");
}
