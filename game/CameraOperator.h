#ifndef OPENCMR2_CAMERA_OPERATOR_H
#define OPENCMR2_CAMERA_OPERATOR_H
#include <cmath>

namespace CameraOperator {
inline const char* Label(bool enabled){return enabled?"drone: on":"drone: off";}
// These are gameplay tuning values, not measured drone physics. Native feel
// remains an acceptance gate. Exposed here so tests check the actual contract.
constexpr float FollowAccelerationLimit=800.f;
inline float Max(float a,float b){return a>b?a:b;}
inline float Clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
inline float Wrap(float x){return std::atan2(std::sin(x),std::cos(x));}
struct V3 {
 float x=0,y=0,z=0;
 V3 operator+(V3 b)const{return {x+b.x,y+b.y,z+b.z};}
 V3 operator-(V3 b)const{return {x-b.x,y-b.y,z-b.z};}
 V3 operator*(float a)const{return {x*a,y*a,z*a};}
 float Length()const{return std::sqrt(x*x+y*y+z*z);}
};
inline V3 Unit(V3 v){float n=v.Length();return n>.0001f?v*(1/n):V3{0,0,1};}
struct RoadPreview {bool valid=false;V3 tangent,near,far;float bend=0;};
// Pure bounded route preview. Node positions are stage data, not body heading.
// `next` is the race-progress segment end, so no global nearest-road search
// can jump to a different leg of a hairpin or a parallel road.
template<class Point> RoadPreview PreviewRoad(V3 car,int next,int count,bool loop,Point point,float lookahead=1.f){
 lookahead=Clamp(lookahead,1.f,2.f); // scales the near/far sample distances; 1 = the previous 12 and 30 world units
 RoadPreview out;if(count<2||next<0||next>=count)return out;
 V3 previous=point(next>0?next-1:loop?count-1:0),end=point(next);
 V3 segment=end-previous;segment.y=0;float length=segment.Length();
 if(length<.1f||!std::isfinite(length)||((car-previous).Length()>80.f&&(car-end).Length()>80.f))return out;
 out.tangent=Unit(segment);V3 from=car-previous;
 float t=Clamp((from.x*segment.x+from.z*segment.z)/(length*length),0.f,1.f);
 V3 start=previous+segment*t;float along=0;bool gotNear=false,gotFar=false;
 for(int i=0;i<32;i++){
  int node=next+i;if(loop)node%=count;else if(node>=count)break;
  V3 target=point(node),delta=target-start;delta.y=0;float len=delta.Length();
  if(len>.01f){
   if(!gotNear&&along+len>=12*lookahead){out.near=start+delta*((12*lookahead-along)/len);gotNear=true;}
   if(!gotFar&&along+len>=30*lookahead){out.far=start+delta*((30*lookahead-along)/len);gotFar=true;break;}
   along+=len;start=target;
  }
 }
 if(!gotNear)return out;
 if(!gotFar)out.far=start;
 V3 ahead=out.far-out.near;ahead.y=0;if(ahead.Length()<1.f)return out;
 ahead=Unit(ahead);out.bend=Clamp(out.tangent.z*ahead.x-out.tangent.x*ahead.z,-1.f,1.f);
 out.valid=true;return out;
}
struct State{
 bool ready=false;V3 position,velocity,aim,aimVelocity,lastCar,roadAhead;bool hasRoad=false;
 float speedDrop=0,speedDropAmount=1.f,heightScale=1.f,lateralGain=1.f,springGain=1.f;
 float terrainLift=0,curveLift=0,pullback=1.f; // pullback: smoothed speed-dependent distance scale
 bool pullbackReady=false;
 float foreLag=0; // diagnostic longitudinal lag, not a clamped displacement
 float bendSmooth=0;V3 nearSmooth;bool previewReady=false; // low-pass of the route preview so node crossings do not step the framing
 float heading=0,headingRate=0,lastHeading=0,turn=0,zoom=1.7f,zoomRate=0,pitch=0,lensSpeedGain=.3f,shakeAmount=1.f,shakePhase=0,shakeRight=0,shakeUp=0,distanceScale=1.f,usedDistance=0; // lensSpeedGain: zoom change at full speed (+ = tighter lens, - = wider lens)
 void Reset(){*this=State{};}
 // A camera far from the car, or not finite, is a failure. Start over behind the car on the next valid step. Runs on every update path,
 // including the early returns for a zero or oversized time step, so a stale state cannot survive them.
 bool Guard(V3 car){
  if(!ready)return false;
  if(std::isfinite(car.x)&&std::isfinite(car.y)&&std::isfinite(car.z)&&std::isfinite(position.x)&&std::isfinite(position.y)&&std::isfinite(position.z)&&(position-car).Length()<=60.f)return false;
  Reset();return true;
 }
 void Step(V3 car,float carHeading,float steering,float speed,float distance,float height,float dt,const RoadPreview*road=nullptr){
  Guard(car);
  if(!std::isfinite(steering)||!std::isfinite(speed)||!std::isfinite(distance)||!std::isfinite(height)||!std::isfinite(carHeading)||!std::isfinite(dt)||dt<=0||dt>.2f||!std::isfinite(car.x)||!std::isfinite(car.y)||!std::isfinite(car.z)){Reset();return;}
  distance=Clamp(distance*1.5f,9.f,14.f);
  // Speed-dependent distance: pull back at low speed to show scenery, fly closer at
  // speed. Smoothed (about 1.3 s) so it never pumps; steering/physics are untouched.
  float speedUnit=Clamp(speed/1.5f,0.f,1.f);
  float wantedPull=1.3f-.5f*speedUnit;
  if(!pullbackReady){pullback=wantedPull;pullbackReady=true;}
  pullback+=(wantedPull-pullback)*(1-std::exp(-dt/1.3f));
  // Drone stand-off is distinct from the original 5-6-unit chase camera.
  // The 50..150 knob scales stand-off within 9..25 units, never collapsing to 4.
  distance=Clamp(distance*pullback*(.75f+.5f*distanceScale),9.f,25.f);usedDistance=distance;
  bool roadValid=road&&road->valid;
  RoadPreview filtered;
  if(roadValid){
   // The preview is piecewise-linear between route nodes, so its bend and near point step each time a node is crossed. Low-pass them (about 0.3 s)
   // so the camera glides into a bend instead of correcting in steps. Visual only.
   if(!previewReady){bendSmooth=road->bend;nearSmooth=road->near-car;previewReady=true;}
   float k=1-std::exp(-dt/.3f);bendSmooth+=(road->bend-bendSmooth)*k;nearSmooth=nearSmooth+((road->near-car)-nearSmooth)*k; // filtered relative to the car, so a fast car does not drag the point behind it
   filtered=*road;filtered.bend=bendSmooth;filtered.near=car+nearSmooth;road=&filtered;
  }else previewReady=false;
  // Actual upcoming route geometry, not steering or car yaw, raises the
  // operator before a corner. Descend more slowly when preview straightens.
  float bend=roadValid?std::abs(road->bend):0.f;
  float wantedLift=Clamp((bend-.06f)/.64f,0.f,1.f)*4.f;
  float liftTime=wantedLift>curveLift?.55f:1.7f;
  curveLift+=(wantedLift-curveLift)*(1-std::exp(-dt/liftTime));
  // Keep the drone low enough to look down the road at every distance knob.
  // Previously a 4-unit trailing distance retained 5-7 units of height before
  // terrain lift, then lower-third framing forced a steep overhead aim.
  height=Clamp(distance*.45f+curveLift*.25f,3.8f,distance*.58f);
  // Actual car speed, independent of SpeedLook (disabled for drone).
  // Descend gradually at speed and rise back more slowly. No projection/FOV change.
  float wantedDrop=Clamp(speedDropAmount,0.f,2.f)*speedUnit*1.8f;
  speedDrop+=(wantedDrop-speedDrop)*(1-std::exp(-dt/(wantedDrop>speedDrop?1.1f:1.6f)));
  height=Max(2.2f,height*Clamp(heightScale,.5f,1.5f)-speedDrop);
  float orbitHeading=carHeading; // gameplay: the driver needs a view from behind, even on a different route tangent
  float delta=ready?Wrap(carHeading-lastHeading):0;
  if(ready&&((car-lastCar).Length()>25.f||std::abs(delta)>.8f)){Reset();delta=0;}
  if(!ready){ready=true;heading=orbitHeading;lastHeading=carHeading;lastCar=car;foreLag=0;position=car+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};aim=car+V3{0,1,0};velocity={};}
  hasRoad=roadValid;if(roadValid)roadAhead=road->far;
  float measured=delta/dt;lastHeading=carHeading;
  turn+=(Clamp(std::abs(measured)*1.8f+std::abs(steering)*.8f,0.f,1.f)-turn)*(1-std::exp(-dt/.3f));
  // Fast damped heading follow, not a multi-second sideways orbit.
  // Substeps keep the critically damped follower stable at low simulation
  // rates. No angular clamp: an abrupt input may briefly exceed the normal
  // couple-degree lag instead of teleporting the camera onto its boundary.
  int steps=int(std::ceil(dt/.005f));float h=dt/steps;
  for(int i=0;i<steps;i++){
   float angularAcceleration=Clamp(6400.f*Wrap(orbitHeading-heading)-160.f*headingRate,-60.f,60.f);
   headingRate+=angularAcceleration*h;
   heading=Wrap(heading+headingRate*h);
  }
  V3 forward={std::sin(carHeading),0,std::cos(carHeading)};
  V3 movement=(car-lastCar)*(1.f/dt);
  float forwardSpeed=movement.x*forward.x+movement.z*forward.z;
  // Fast world-space spring. Position and velocity are integrated, not
  // reconstructed from a clamped relative offset. Input jumps change bounded
  // acceleration rather than instantly moving the camera sideways/forward.
  for(int i=0;i<steps;i++){
   V3 anchor=lastCar+(car-lastCar)*(float(i)/steps);
   V3 desired=anchor+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};
   V3 desiredVelocity=movement+V3{-std::cos(heading)*distance*headingRate,0,std::sin(heading)*distance*headingRate};
   V3 error=desired-position,relativeVelocity=velocity-desiredVelocity;
   V3 longitudinal={std::sin(heading),0,std::cos(heading)};
   float e=error.x*longitudinal.x+error.z*longitudinal.z;
   float v=relativeVelocity.x*longitudinal.x+relativeVelocity.z*longitudinal.z;
   // Independent longitudinal spring: about1-2units of transient lag under
   // hard acceleration/braking, then settles. Lateral position floats separately.
   V3 lateral={std::cos(heading),0,-std::sin(heading)};
   float sideError=error.x*lateral.x+error.z*lateral.z;
   // Do not feed orbit velocity into sideways motion: that welded position to
   // every turn. Partial car translation feedforward leaves a visible float.
   float sideVelocity=(velocity.x-.2f*movement.x)*lateral.x+(velocity.z-.2f*movement.z)*lateral.z;
   V3 acceleration=lateral*(sideError*(36.f*lateralGain)-sideVelocity*(12.f*std::sqrt(lateralGain)))
       +longitudinal*(e*(16.f*springGain)-v*(8.f*std::sqrt(springGain)))+V3{0,error.y*100.f-relativeVelocity.y*20.f,0};
   float n=acceleration.Length();if(n>FollowAccelerationLimit)acceleration=acceleration*(FollowAccelerationLimit/n);
   velocity=velocity+acceleration*h;position=position+velocity*h;
  }
  V3 away=position-car;
  float along=away.x*forward.x+away.z*forward.z;
  foreLag=-along-distance;
  // Emergency safety boundary is behind the car. Ordinary braking never
  // reaches it; discontinuous teleports/inputs prioritize not overtaking.
  float minimum=distance*.55f;
  if(along>-minimum){position=position-forward*(along+minimum);float v=velocity.x*forward.x+velocity.z*forward.z;if(v>forwardSpeed)velocity=velocity-forward*(v-forwardSpeed);}
  V3 target=car+V3{std::sin(carHeading)*Clamp(speed*3.f,2.f,5.f),1.2f,std::cos(carHeading)*Clamp(speed*3.f,2.f,5.f)};
  if(roadValid){target=road->near;target.y=car.y+1.2f;}
  V3 aimAcceleration=(target-aim)*12.f-aimVelocity*5.5f;
  aimVelocity=aimVelocity+aimAcceleration*dt;aim=aim+aimVelocity*dt;
  // Steering can widen before the body turns. Lens is actual projection zoom,
  // independent of viewpoint distance. Never random or an FOV pulse per frame.
  float speedRatio=Clamp(speed/1.5f,0.f,1.f);
  // Speed-linked handheld shake: small, smooth (two slow sines per axis, about 3-6 Hz), grows
  // with speed squared so it is nearly absent when slow. Position offset only, applied after
  // framing, never touches the car or the aim. shakeAmount 0 = off (1 = about 8 cm at full speed).
  shakePhase+=dt;{float amp=shakeAmount*.08f*speedRatio*speedRatio;
  shakeRight=amp*(.6f*std::sin(shakePhase*17.3f)+.4f*std::sin(shakePhase*29.1f+1.7f));
  shakeUp=amp*.8f*(.6f*std::sin(shakePhase*21.7f+.5f)+.4f*std::sin(shakePhase*37.9f));}
  float wanted=1.55f+lensSpeedGain*speedRatio-.4f*turn;
  zoomRate+=(8.f*(wanted-zoom)-5.f*zoomRate)*dt;
  zoom=Clamp(zoom+zoomRate*dt,1.15f,1.9f);lastCar=car;
 }
 // Look direction keeps the player in the lower third, with tightly bounded
 // horizontal framing so route preview cannot hide the steering direction. Projection uses original 2.080*fov
 // times .625 base zoom. Custom FOV/tilt may intentionally alter this.
 V3 FramedForward()const{
  V3 f=Unit(aim-position),toCar=lastCar+V3{0,1.f,0}-position;
  float horizontal=std::sqrt(toCar.x*toCar.x+toCar.z*toCar.z);
  float carElevation=std::atan2(toCar.y,horizontal);
  float lookElevation=std::asin(Clamp(f.y,-1.f,1.f));
  float target=carElevation+std::atan(.30f/(1.3f*zoom));
  lookElevation=Clamp(lookElevation,target-.015f,target+.015f);
  float carYaw=std::atan2(toCar.x,toCar.z),aimYaw=std::atan2(f.x,f.z);
  float yaw=carYaw+Clamp(Wrap(aimYaw-carYaw),-.01f,.01f);

  V3 flat={std::sin(yaw),0,std::cos(yaw)};
  return {flat.x*std::cos(lookElevation),std::sin(lookElevation),flat.z*std::cos(lookElevation)};
 }
 // Ground-height constrained sight corridor, not generic object collision.
 // Query samples current/near-future camera footpoints and car->camera rays.
 template<class Query> void ConstrainTerrain(V3 car,float dt,Query rawQuery){
  // Ground samples far from the car's own level (another surface level, a bad triangle) are ignored, and the lift is capped: it is
  // divided by a ray fraction as small as 1/24, so one wild sample used to move the camera by up to 24x its error, without limit.
  Guard(car);
  const float carY=car.y;
  auto ground=[&](V3 point,float&y){float v=0;if(!rawQuery(point,v)||!std::isfinite(v)||v>carY+15.f||v<carY-60.f)return false;y=v;return true;};
  if(!ready||dt<=0||dt>.2f)return;
  V3 base=position;float required=0;
  auto liftFor=[&](V3 camera){
   float lift=0;
   auto corridor=[&](V3 target){
    float targetFloor;
    if(ground(target,targetFloor)&&std::isfinite(targetFloor))target.y=Max(target.y,targetFloor+1.8f);
    for(int i=1;i<=24;i++){
     float t=i/24.f;V3 point=target+(camera-target)*t;float y;
     if(ground(point,y)&&std::isfinite(y))lift=Max(lift,(y+1.2f-point.y)/t);
    }
   };
   corridor(car+V3{0,1.2f,0});corridor(aim);if(hasRoad)corridor(roadAhead);
   float floor;V3 predictive=camera+velocity*.35f;
   if(ground(predictive,floor)&&std::isfinite(floor))lift=Max(lift,floor+2.f-camera.y);
   return Max(0.f,lift);
  };
  required=liftFor(base);
  // Never pull horizontally toward the car to clear terrain: this collapsed
  // the trailing distance while retaining height and forced an overhead view.
  // Do not trade away the road view for unlimited corridor clearance. As with
  // bounded rise, a real bank can clip; this remains a ground heuristic.
  float trail=std::sqrt((base.x-car.x)*(base.x-car.x)+(base.z-car.z)*(base.z-car.z));
  float liftBudget=Max(0.f,trail*.65f-(base.y-car.y));
  required=Clamp(required,0.f,Max(0.f,liftBudget));
  position=base;
  // A discontinuous corridor/triangle sample must not teleport the camera
  // upwards. Rise at most 3 units/s; preserve the accepted slow 2 units/s
  // descent. This is a ground-height heuristic, not collision clearance:
  // a sudden real obstruction can clip while the bounded rise catches up.
  terrainLift=required>terrainLift?Clamp(required,terrainLift,terrainLift+dt*3.f):Max(required,terrainLift-dt*2.f);
  position.y+=terrainLift;
  if(terrainLift>0&&velocity.y<0)velocity.y=0;
  // Safety net: a camera this far from the car is a failure, not a framing. Start over behind the car next step.
  Guard(car);
 }

};
}
struct CameraRecord;struct FixMatrix;struct FixVector;
bool CameraOperator_Enabled(const CameraRecord*);
void CameraOperator_Reset(unsigned index);
void CameraOperator_Update(CameraRecord*,const FixMatrix*,const FixVector* offset=nullptr,short pitch=0x2d);
void CameraOperator_Apply(CameraRecord*);
#endif
