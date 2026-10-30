#include "CameraOperator.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <functional>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 using namespace CameraOperator;State a,b;
 V3 car={0,0,0};a.Step(car,0,0,1.5,6.5,2,.04);
 a.Step(car, .6f,1,1.5,6.5,2,.04);
 CHECK(a.heading>0&&a.heading<.08f); // abrupt input does not snap onto an angular boundary
 for(int i=0;i<15;i++)a.Step(car,.6f,1,1.5,6.5,2,.04);
 CHECK(std::abs(Wrap(a.heading-.6f))<.035f);
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
 // Route preview may lift for visibility, but must not pull the camera sideways away from the car heading.
 V3 nodes[5]={{0,0,-10},{0,0,10},{10,0,20},{25,0,25},{40,0,25}};
 auto preview=PreviewRoad({0,0,0},1,5,false,[&](int i){return nodes[i];});CHECK(preview.valid&&preview.bend>.3f);
 State rightBend,leftBend;auto mirrored=preview;mirrored.bend=-preview.bend;mirrored.near.x=-preview.near.x;mirrored.far.x=-preview.far.x;
 for(int i=0;i<120;i++){rightBend.Step({0,0,0},-.5f,0,1.5,6.5,2,.04,&preview);leftBend.Step({0,0,0},.5f,0,1.5,6.5,2,.04,&mirrored);}
 CHECK(std::abs(Wrap(rightBend.heading+.5f))<.035f);CHECK(std::abs(Wrap(leftBend.heading-.5f))<.035f);
 CHECK(rightBend.position.x>2.f);CHECK(leftBend.position.x<-2.f); // behind the body, opposite the old outside-bend orbit
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
 {
  State base,close,far;close.distanceScale=.5f;far.distanceScale=1.5f;
  for(int i=0;i<400;i++){base.Step({0,0,0},0,0,0,6.5,2,.04);close.Step({0,0,0},0,0,0,6.5,2,.04);far.Step({0,0,0},0,0,0,6.5,2,.04);}
  CHECK(close.usedDistance<base.usedDistance&&far.usedDistance>base.usedDistance&&close.usedDistance>=9.f&&far.usedDistance<=25.f);
 }
 {  // Moving straight at steady speed: stay at the selected offset, not the old lagging tether.
  auto settle=[](float scale){State s;s.distanceScale=scale;V3 car={0,0,0};for(int i=0;i<1200;i++){car.z+=28.f/60;s.Step(car,0,0,73.f,6.5f,1.5f,1.f/60);}V3 a=s.position-car;return std::sqrt(a.x*a.x+a.z*a.z);};
  float d100=settle(1.f),d60=settle(.6f),d50=settle(.5f);
  CHECK(d100>9.6f&&d100<9.9f); // horizontal trail, independent of the corrected lower height
  CHECK(d60>8.99f&&d50>8.99f&&d50<=d60);
 }
 { // Editable parameter endpoints remain finite and respect the height floor.
  for(float gain:{.25f,2.f})for(float height:{.5f,1.5f})for(float drop:{0.f,1.f}){
   State s;s.lateralGain=gain;s.springGain=gain;s.heightScale=height;s.speedDropAmount=drop;V3 car;
   for(int i=0;i<600;i++){float yaw=std::sin(i*.01f)*.4f;car=car+V3{std::sin(yaw),0,std::cos(yaw)}*(20.f/60);
    s.Step(car,yaw,0,1.5f,6.5f,1.5f,1.f/60);CHECK(s.ready);CHECK(std::isfinite(s.position.x));CHECK(s.position.y>2.1f);CHECK((s.position-car).Length()<40.f);
   }
  }
 }
 { // Lateral position floats independently, while speed drop recovers smoothly.
  for(float dt:{.04f,1.f/60,1.f/120}){
   State s;s.distanceScale=.5f;s.lensSpeedGain=0;s.shakeAmount=0;V3 car;float yaw=0,maxSide=0,maxStep=0,lastY=0,highY=0;
   for(int i=0;i<int(10/dt);i++){
    float t=i*dt,rate=t>2&&t<3?.8f:t>3&&t<4?-.8f:0,speed=t<1?0:t<2?(t-1)*28:t<5?28:t<6?(6-t)*28:0;
    car=car+V3{std::sin(yaw),0,std::cos(yaw)}*(speed*dt);yaw+=rate*dt;
    s.Step(car,yaw,0,speed/28*1.5f,6.5f,1.5f,dt);
    V3 d=car-s.position;float side=d.x*std::cos(yaw)-d.z*std::sin(yaw);maxSide=Max(maxSide,std::abs(side));
    if(i)maxStep=Max(maxStep,std::abs(s.position.y-lastY));lastY=s.position.y;if(t>4.9f&&t<5.1f)highY=s.position.y;
    CHECK(s.position.y>2.19f);CHECK(std::isfinite(s.FramedForward().x));
   }
   CHECK(maxSide>2.f&&maxSide<6.f);CHECK(highY<3.f);CHECK(s.position.y>3.8f);CHECK(maxStep<dt*3.f);
  }
 }
 { // Gameplay steering envelope at 25/60/120 Hz, hard braking and rapid turns.
  for(float dt:{.04f,1.f/60,1.f/120}){
   State follow;follow.distanceScale=.6f;V3 car;float yaw=0,maxFall=0,maxCatch=0;
   for(int i=0;i<int(7/dt);i++){
    float t=i*dt;
    float v=t<1?0:t<2?(t-1)*28:t<4?28:0;
    // Continuous turns, including a reversal; not a teleport reset.
    yaw+=((t>2&&t<3)?.8f:(t>3&&t<4)?-1.2f:0.f)*dt;
    V3 forward={std::sin(yaw),0,std::cos(yaw)};
    car=car+forward*(v*dt);
    follow.Step(car,yaw,0,1.5f,6.5f,1.5f,dt);
    V3 behind=car-follow.position;
    float along=behind.x*forward.x+behind.z*forward.z;
    CHECK(along>follow.usedDistance*.54f); // never goes past the car, including emergency stop
    float orbit=std::atan2(behind.x,behind.z);
    CHECK(std::abs(Wrap(orbit-yaw))<1.0f); // revised float may orbit; never lose the behind-car hemisphere
    auto view=follow.FramedForward();
    CHECK(std::abs(Wrap(std::atan2(view.x,view.z)-orbit))<=.011f); // aim keeps car readable while position floats
    CHECK(std::abs(follow.foreLag)<=3.f);
    if(t>1&&t<2)maxFall=Max(maxFall,follow.foreLag);
    if(t>4&&t<4.6f)maxCatch=Max(maxCatch,-follow.foreLag);
   }
   CHECK(maxFall>1.f);CHECK(maxCatch>1.f); // actual fall-back and late braking, not rigid attachment
   CHECK(std::abs(follow.foreLag)<.02f); // settles rather than retaining speed-dependent lag
  }
 }
 { // Abrupt turn/reversal, wrap and braking: inspect world-space displacement
   // derivatives, rather than only the relative lag scalar.
  for(float dt:{.04f,1.f/60,1.f/120}){
   State s;V3 car;V3 previous,prevVelocity;float worstJump=0,worstAcceleration=0;
   for(int i=0;i<int(6/dt);i++){
    float t=i*dt,heading=t<2?3.12f:t<3?-3.12f:t<4?-2.7f:3.12f;
    float v=t<1?0:t<4?12:0;
    car=car+V3{0,0,-v*dt};
    s.Step(car,heading,0,1.5f,6.5f,1.5f,dt);
    if(i>2){V3 velocity=(s.position-previous)*(1/dt);worstJump=Max(worstJump,(s.position-previous).Length());
     worstAcceleration=Max(worstAcceleration,(velocity-prevVelocity).Length()/dt);prevVelocity=velocity;}
    previous=s.position;
   }
   fprintf(stderr,"dt %.5f max displacement %.3f world acceleration %.1f\n",dt,worstJump,worstAcceleration);
   CHECK(worstJump < dt*(12.f+26.f*2.f)); // translating car + farther drone turning at bounded orbit speed
   // Averaged world-velocity differences cannot exceed the integrated
   // acceleration contract, aside from 5% finite-difference/float tolerance.
   // This catches projection snaps; it does not establish a comfortable feel.
   CHECK(worstAcceleration<=FollowAccelerationLimit*1.05f);
  }
 }
 { // Heading wrap and mirrored left/right turns have the same bounded response.
  State left,right;V3 car;
  for(int i=0;i<300;i++){
   float yaw=3.0f+i*.012f;
   left.Step(car,Wrap(yaw),0,1.5f,6.5f,1.5f,.04f);
   right.Step(car,Wrap(-yaw),0,1.5f,6.5f,1.5f,.04f);
   CHECK(std::abs(Wrap(left.heading-yaw))<=.0351f);
   CHECK(std::abs(Wrap(right.heading+yaw))<=.0351f);
   CHECK(std::abs(left.position.x+right.position.x)<.001f);
   CHECK(std::abs(left.position.z-right.position.z)<.001f);
  }
 }
 { // Read the road ahead, including close distance, bends and terrain.
  for(float sc:{.5f,.6f,1.f,1.5f})for(bool bank:{false,true}){
   State s;s.distanceScale=sc;s.lensSpeedGain=0;V3 car;RoadPreview r;
   r.valid=true;r.bend=.8f;r.near={0,0,16};r.far={0,0,40};
   for(int i=0;i<600;i++){
    if(s.ready)s.position.y-=s.terrainLift;
    s.Step(car,0,0,1.5f,6.5f,1.5f,.04f,&r);
    float trailBefore=s.position.z;
    if(bank)s.ConstrainTerrain(car,.04f,[](V3 p,float&y){y=p.z<-1?4.f:0;return true;});
    CHECK(s.position.z==trailBefore); // terrain must never collapse trailing distance
    auto f=s.FramedForward();CHECK(f.z>.90f); // pitch remains within about26 degrees of horizontal
   }
  }
 }
 { // Discontinuous corridor samples must not teleport the drone upwards.
  State s;V3 car;float previous=0,worstRise=0;
  for(int i=0;i<200;i++){
   if(s.ready)s.position.y-=s.terrainLift;
   s.Step(car,0,0,1.5f,6.5f,1.5f,.04f);
   s.ConstrainTerrain(car,.04f,[&](V3,float&y){y=(i%25<4)?5.f:-1.5f;return true;});
   if(i>0)worstRise=Max(worstRise,s.position.y-previous);previous=s.position.y;
  }
  CHECK(worstRise<=.13f);
 }
 {  // camera escape: wild ground samples, tunnel roofs, variable dt, teleports and pauses must never send the camera far from the car
  typedef std::function<bool(V3,float&)> G;
  auto worst=[](G g,float scale,float dtBase,bool jitter,bool jumps){
   State s;s.distanceScale=scale;V3 car={0,0,0};float worstSep=0;unsigned seed=1;
   for(int i=0;i<1500;i++){
    seed=seed*1664525u+1013904223u;float dt=dtBase;if(jitter)dt=dtBase*(.2f+(seed>>16&255)/64.f); // .2x to 4.2x
    if(jumps&&i%400==399)car.z+=300; // respawn / restart
    if(jumps&&i%500==250)dt=0;       // paused step
    car.z+=28.f*std::max(dt,0.f);
    if(s.ready)s.position.y-=s.terrainLift;
    s.Step(car,0,0,1.5f,6.5f,1.5f,dt,nullptr);
    s.ConstrainTerrain(car,dt,[&](V3 p,float&y){return g(p,y);});
    if(s.ready)worstSep=std::max(worstSep,(s.position-car).Length());
   }
   return worstSep;
  };
  for(float sc:{1.f,.5f})for(bool j:{false,true}){
   CHECK(worst([](V3,float&y){y=-1.5f;return true;},sc,.04f,j,j)<40.f);
   CHECK(worst([](V3 p,float&y){y=(p.z>100&&p.z<100.5f)?400.f:-1.5f;return true;},sc,.04f,j,false)<40.f);   // one wild sample
   CHECK(worst([](V3 p,float&y){y=(p.y>2)?8.f:-1.5f;return true;},sc,.04f,j,false)<40.f);                    // tunnel roof
   CHECK(worst([](V3 p,float&y){y=(((int)(p.z*3))%7==0)?5000.f:-1.5f;return true;},sc,.04f,j,false)<40.f);    // many wild samples
   CHECK(worst([](V3 p,float&y){y=std::nanf("");return true;},sc,.04f,j,false)<40.f);                         // non-finite samples
  }
 }
 {  // early-return paths: a zero, negative, oversized or NaN time step after a teleport must not leave a stale far camera
  for(float badDt:{0.f,-.04f,.5f,(float)NAN}){
   State s;V3 car={0,0,0};for(int i=0;i<200;i++){car.z+=.5f;s.Step(car,0,0,1.5f,6.5f,1.5f,.04f);}
   CHECK(s.ready);car.z+=500;s.Step(car,0,0,1.5f,6.5f,1.5f,badDt);s.ConstrainTerrain(car,badDt,[](V3,float&y){y=-1.5f;return true;});
   CHECK(!s.ready||(s.position-car).Length()<=60.f);
   car.z+=.5f;s.Step(car,0,0,1.5f,6.5f,1.5f,.04f);CHECK((s.position-car).Length()<20.f);
  }
  State s;V3 car={0,0,0};for(int i=0;i<200;i++)s.Step(car,0,0,1.5f,6.5f,1.5f,.04f);
  s.position.y=1e30f;CHECK(s.Guard(car)&&!s.ready); // far/huge
  State t;for(int i=0;i<10;i++)t.Step(car,0,0,1.5f,6.5f,1.5f,.04f);t.position.x=NAN;CHECK(t.Guard(car)&&!t.ready);
 }
 {  // a bank that really needs more than the cap: the camera is held at the cap and may clip a very tall bank, but stays near the car
  State s;V3 car={0,0,0};float worstUp=0;
  for(int i=0;i<600;i++){car.z+=1.1f;if(s.ready)s.position.y-=s.terrainLift;s.Step(car,0,0,1.5f,6.5f,1.5f,.04f);
   s.ConstrainTerrain(car,.04f,[](V3,float&y){y=12.f;return true;});if(s.ready)worstUp=std::max(worstUp,s.position.y);}
  CHECK(worstUp<=11+4+8+1);
 }
 puts("gameplay drone heading bounds, fore/aft inertia, terrain, lens, settling and reset pass");
}
