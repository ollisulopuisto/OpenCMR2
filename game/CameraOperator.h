#ifndef OPENCMR2_CAMERA_OPERATOR_H
#define OPENCMR2_CAMERA_OPERATOR_H
#include <cmath>

namespace CameraOperator {
inline const char* Label(bool enabled){return enabled?"drone: on":"drone: off";}
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
 if(!gotNear)return out;if(!gotFar)out.far=start;
 V3 ahead=out.far-out.near;ahead.y=0;if(ahead.Length()<1.f)return out;
 ahead=Unit(ahead);out.bend=Clamp(out.tangent.z*ahead.x-out.tangent.x*ahead.z,-1.f,1.f);
 out.valid=true;return out;
}
struct State{
 bool ready=false;V3 position,velocity,aim,aimVelocity,lastCar,roadAhead;bool hasRoad=false;
 float terrainLift=0,curveLift=0,pullback=1.f; // pullback: smoothed speed-dependent distance scale
 bool pullbackReady=false;
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
  distance=Clamp(distance*1.5f,9.f,14.f);height=Clamp(height*3.f,7.f,10.f);
  // Speed-dependent distance: pull back at low speed to show scenery, fly closer at
  // speed. Smoothed (about 1.3 s) so it never pumps; steering/physics are untouched.
  float speedUnit=Clamp(speed/1.5f,0.f,1.f);
  float wantedPull=1.3f-.5f*speedUnit;
  if(!pullbackReady){pullback=wantedPull;pullbackReady=true;}
  pullback+=(wantedPull-pullback)*(1-std::exp(-dt/1.3f));
  distance=Clamp(distance*pullback,7.f,17.f)*distanceScale; // distanceScale: user knob, 1 = default
  distance=Clamp(distance,4.f,26.f);usedDistance=distance;height=Clamp(height*(1.f+.2f*(pullback-1.f)),6.f,11.f);height*=.5f+.5f*Clamp(distanceScale,0.f,1.f);
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
  height+=curveLift*Clamp(distanceScale,0.f,1.f); // the curve lift follows the distance knob, so a closer drone stays closer in bends
  float orbitHeading=roadValid?std::atan2(road->tangent.x,road->tangent.z):carHeading;
  float delta=ready?Wrap(carHeading-lastHeading):0;
  if(ready&&((car-lastCar).Length()>25.f||std::abs(delta)>.8f)){Reset();delta=0;}
  if(!ready){ready=true;heading=orbitHeading;lastHeading=carHeading;lastCar=car;position=car+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};aim=car+V3{0,1,0};velocity={};}
  hasRoad=roadValid;if(roadValid)roadAhead=road->far;
  float measured=delta/dt;lastHeading=carHeading;
  turn+=(Clamp(std::abs(measured)*1.8f+std::abs(steering)*.8f,0.f,1.f)-turn)*(1-std::exp(-dt/.3f));
  // Independent trailing orbit heading: car can rotate without rotating camera.
  float accel=Clamp(3.f*Wrap(orbitHeading-heading)-3.f*headingRate,-.7f,.7f);
  headingRate=Clamp(headingRate+accel*dt,-.65f,.65f);heading=Wrap(heading+headingRate*dt);
  V3 desired=car+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};
  if(roadValid){
   // Outside the upcoming bend opens the view. Right-hand bend -> left of
   // route, even while the car is sliding with the opposite body heading.
   V3 right={road->tangent.z,0,-road->tangent.x};
   desired=desired+right*(-Clamp(road->bend*9.f,-6.f,6.f)*Clamp(distanceScale,0.f,1.f));
  }
  // Second-order world-space chase. Never re-add car position at draw time.
  V3 acceleration=(desired-position)*4.f-velocity*4.f;
  float magnitude=acceleration.Length();if(magnitude>35.f)acceleration=acceleration*(35.f/magnitude);
  velocity=velocity+acceleration*dt;position=position+velocity*dt;
  // Keep framing safe on very long high-speed transients, but do not rigidly
  // rotate the position offset with the car. This is a tether, not attachment.
  V3 away=position-car;float separation=away.Length();
  // The tether is what holds the camera near the car at speed (the spring chase lags a moving car). It stays 1.3 x the intended offset at the
  // default knob (100) and tightens to 1.0 x at 50, so a lower drone_distance really brings the camera closer at speed.
  float tether=std::sqrt(distance*distance+height*height)*(1.f+.3f*Clamp((distanceScale-.5f)/.5f,0.f,1.f));
  if(separation>tether){position=car+away*(tether/separation);}
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
 // Look direction keeps the player in the lower third while allowing the
 // independent aim to pan horizontally. Projection uses original 2.080*fov
 // times .625 base zoom. Custom FOV/tilt may intentionally alter this.
 V3 FramedForward()const{
  V3 f=Unit(aim-position),toCar=lastCar+V3{0,1.f,0}-position;
  float horizontal=std::sqrt(toCar.x*toCar.x+toCar.z*toCar.z);
  float carElevation=std::atan2(toCar.y,horizontal);
  float lookElevation=std::asin(Clamp(f.y,-1.f,1.f));
  float target=carElevation+std::atan(.30f/(1.3f*zoom));
  lookElevation=Clamp(lookElevation,target-.015f,target+.015f);
  float carYaw=std::atan2(toCar.x,toCar.z),aimYaw=std::atan2(f.x,f.z);
  float yaw=carYaw+Clamp(Wrap(aimYaw-carYaw),-.16f,.16f);
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
  V3 original=position,base=position;float required=0;
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
  // A tall bank should pull the drone in, not send it to TV-helicopter height.
  // Test bounded alternatives along the existing inertial direction. This is
  // a safety correction only, not a new car-attached nominal camera path.
  if(required>3.f){
   float cost=required;
   for(int alternative=1;alternative<=3;alternative++){
    float fraction=1.f-.2f*alternative;
    V3 candidate=car+(original-car)*fraction;candidate.y=original.y;
    if((candidate-car).Length()<5.f)continue;
    float lift=liftFor(candidate),score=lift+(candidate-original).Length()*.35f;
    if(score<cost){cost=score;base=candidate;required=lift;}
   }
  }
  required=Clamp(required,0.f,8.f);
  position=base;
  // Rise before collision if possible; never ease through known terrain.
  // Fall slowly after a crest so camera doesn't track every car bump.
  terrainLift=required>terrainLift?required:Max(required,terrainLift-dt*2.f);
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
