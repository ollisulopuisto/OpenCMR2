#pragma once
#include <cmath>
// Optional visual roll only. No body roll, car input or camera position changes.
namespace CornerCameraRoll {
inline float Wrap(float x){return std::atan2(std::sin(x),std::cos(x));}
inline float Clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
struct State {
 bool ready=false;float lastHeading=0,angle=0,rate=0;
 void Reset(){*this=State{};}
 void Step(float heading,float speed,float dt,int amount){
  if(!std::isfinite(heading)||!std::isfinite(speed)||!std::isfinite(dt)||dt<=0||dt>.2f||amount<=0){Reset();return;}
  if(!ready){ready=true;lastHeading=heading;return;}
  float delta=Wrap(heading-lastHeading);lastHeading=heading;
  if(std::abs(delta)>.8f){Reset();return;}
  float limit=.06981317f*Clamp(amount/100.f,0.f,1.f); // up to four degrees, opt-in
  float target=Clamp(delta/dt*.06f,-limit,limit)*Clamp(std::abs(speed)/1.5f,0.f,1.f);
  int steps=int(std::ceil(dt/.005f));float h=dt/steps;
  for(int i=0;i<steps;i++){
   rate+=(64.f*(target-angle)-16.f*rate)*h;
   angle+=rate*h;
  }
 }
};
}
struct CameraRecord;struct FixMatrix;
void CornerCameraRoll_Reset(unsigned index);
void CornerCameraRoll_Update(CameraRecord*,const FixMatrix*);
void CornerCameraRoll_Apply(CameraRecord*);
