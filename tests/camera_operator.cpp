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
 // Ridge between camera and player: endpoints alone would miss it.
 a.Reset();a.Step({0,0,0},0,0,0,6.5,2,.04);auto horizontal=a.position;
 a.ConstrainTerrain({0,0,0},.04,[](V3 p,float&h){h=(p.z<-4&&p.z>-8)?5.f:0.f;return true;});
 CHECK(a.position.y>horizontal.y+.1f||a.position.z>horizontal.z+1.f);CHECK((a.position-V3{0,0,0}).Length()<25.f);
 // Flat steady-state car is around lower third: more frame ahead than behind.
 State framed;for(int i=0;i<300;i++)framed.Step({0,0,i*.25f},0,0,1.5,6.5,2,.04);
 V3 f=framed.FramedForward(),r=Unit({f.z,0,-f.x}),up={-f.y*r.z, f.z*r.x-f.x*r.z,-f.y*r.x};
 V3 delta=framed.lastCar+V3{0,1,0}-framed.position;
 auto dot=[](V3 x,V3 y){return x.x*y.x+x.y*y.y+x.z*y.z;};
 float y=.5f-.5f*1.3f*framed.zoom*dot(delta,up)/dot(delta,f);
 CHECK(y>.62f&&y<.69f);CHECK((framed.position-framed.lastCar).Length()<17.f);
 float lift=a.terrainLift; a.position.y-=lift;
 a.ConstrainTerrain({0,0,0},.04,[](V3,float&h){h=-100;return true;});CHECK(a.terrainLift>=lift-.081f);
 // Repeated hill/raised-road passes must not accumulate previous lift or
 // mistake an aim point buried in the next crest for a 100m obstruction.
 State hill;V3 drive;
 for(int i=0;i<600;i++){
  drive.z+=.25f;drive.y=4.f*std::exp(-(drive.z-42)*(drive.z-42)/50.f);
  if(hill.ready)hill.position.y-=hill.terrainLift;
  hill.Step(drive,0,0,1.5,6.5,2,.04);
  hill.ConstrainTerrain(drive,.04,[](V3 p,float&h){h=4.f*std::exp(-(p.z-42)*(p.z-42)/50.f);return true;});
  CHECK(std::isfinite(hill.position.y));CHECK(hill.terrainLift<8.f);CHECK((hill.position-drive).Length()<25.f);
 }
 auto saved=hill.position;hill.ConstrainTerrain(drive,.04,[](V3,float&){return false;});
 CHECK(std::isfinite(hill.position.y)); // unknown samples do not invent ground
 // Route-derived right bend chooses left, mirrored bend chooses right.
 V3 nodes[5]={{0,0,-10},{0,0,10},{10,0,20},{25,0,25},{40,0,25}};
 auto preview=PreviewRoad({0,0,0},1,5,false,[&](int i){return nodes[i];});CHECK(preview.valid&&preview.bend>.3f);
 State rightBend,leftBend;auto mirrored=preview;mirrored.bend=-preview.bend;mirrored.near.x=-preview.near.x;mirrored.far.x=-preview.far.x;
 for(int i=0;i<120;i++){rightBend.Step({0,0,0},-.5f,0,1.5,6.5,2,.04,&preview);leftBend.Step({0,0,0},.5f,0,1.5,6.5,2,.04,&mirrored);}
 CHECK(rightBend.position.x<-2.f);CHECK(leftBend.position.x>2.f);
 CHECK(!PreviewRoad({0,0,0},-1,5,false,[&](int i){return nodes[i];}).valid);
 CHECK(!PreviewRoad(nodes[4],4,5,false,[&](int i){return nodes[i];}).valid);
 // Degenerate, far-away and finish previews fail closed to original inertia.
 CHECK(!PreviewRoad({1000,0,1000},1,5,false,[&](int i){return nodes[i];}).valid);
 CHECK(!PreviewRoad({},0,5,false,[](int){return V3{};}).valid);
 V3 loopNodes[5]={{0,0,0},{0,0,20},{20,0,20},{20,0,0},{10,0,0}};
 CHECK(PreviewRoad({15,0,0},4,5,true,[&](int i){return loopNodes[i];}).valid);
 a.Step(car,NAN,0,1,6.5,2,.04);CHECK(!a.ready);
 a.Step(car,0,0,1,6.5,2,0);CHECK(!a.ready);
 {
  // Speed-dependent distance: slow pulls back, fast flies closer, and it is smoothed.
  State slow,fast;
  for(int i=0;i<400;i++){slow.Step({0,0,i*.0f},0,0,0,6.5,2,.04);fast.Step({0,0,i*.0f},0,0,1.5,6.5,2,.04);}
  CHECK(slow.pullback>1.25f);CHECK(fast.pullback<.85f);
  State change;for(int i=0;i<200;i++)change.Step({0,0,0},0,0,0,6.5,2,.04);
  float before=change.pullback;change.Step({0,0,0},0,0,1.5,6.5,2,.04);
  CHECK(std::abs(change.pullback-before)<.02f); // one step cannot jump
  CHECK(change.pullback>=.8f&&change.pullback<=1.3f);
 }
 {
  // Speed lens: positive gain tightens, negative widens, both smooth and bounded.
  State tight,wide;wide.lensSpeedGain=-.3f;
  for(int i=0;i<400;i++){tight.Step({0,0,0},0,0,1.5,6.5,2,.04);wide.Step({0,0,0},0,0,1.5,6.5,2,.04);}
  CHECK(tight.zoom>1.7f);CHECK(wide.zoom<1.4f);CHECK(wide.zoom>=1.15f&&tight.zoom<=1.9f);
 }
 {
  // Speed shake: absent at rest, small and bounded at speed, off when amount is 0.
  State rest,fast,off;off.shakeAmount=0;float maxRest=0,maxFast=0,maxOff=0,maxJump=0,last=0;
  for(int i=0;i<600;i++){rest.Step({0,0,0},0,0,0,6.5,2,.016);fast.Step({0,0,0},0,0,1.5,6.5,2,.016);off.Step({0,0,0},0,0,1.5,6.5,2,.016);
   maxRest=std::max(maxRest,std::abs(rest.shakeRight));maxFast=std::max(maxFast,std::abs(fast.shakeRight));maxOff=std::max(maxOff,std::abs(off.shakeRight));
   if(i>0)maxJump=std::max(maxJump,std::abs(fast.shakeRight-last));last=fast.shakeRight;}
  CHECK(maxRest<1e-6f);CHECK(maxFast>.02f&&maxFast<=.081f);CHECK(maxOff<1e-6f);CHECK(maxJump<.03f);
 }
 puts("world camera position/orbit inertia, steering lens, settling, determinism/reset pass");
}
