#ifndef OPENCMR2_CAMERA_OPERATOR_H
#define OPENCMR2_CAMERA_OPERATOR_H
#include <cmath>
namespace CameraOperator {
inline float Clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
inline float Wrap(float x){return std::atan2(std::sin(x),std::cos(x));}
struct State{
 bool ready=false;float heading=0,trend=0,angle=0,rate=0;
 void Reset(){*this=State{};}
 float Step(float currentHeading,float travelHeading,float speedRatio,float dt){
  if(!std::isfinite(currentHeading)||!std::isfinite(travelHeading)||!std::isfinite(speedRatio)||!std::isfinite(dt)||dt<=0||dt>.2f){Reset();return 0;}
  if(!ready){ready=true;heading=currentHeading;return 0;}
  float turn=Wrap(currentHeading-heading);heading=currentHeading;
  if(std::abs(turn)>.65f){Reset();return 0;} // cut, respawn, replay seek
  // Stable trend extrapolates a deliberate lead; no random jitter or whipping.
  float measured=Clamp(turn/dt,-1.2f,1.2f);
  trend+=(measured-trend)*(1-std::exp(-dt/.22f));
  float desired=Clamp(trend*.35f+Wrap(travelHeading-currentHeading)*.18f*Clamp(speedRatio,0,1),-.14f,.14f);
  if(std::abs(desired-angle)<.0087f)desired=angle; // half-degree framing hold
  float wantedRate=Clamp((desired-angle)/.28f,-.21f,.21f); //12deg/s
  rate+=Clamp(wantedRate-rate,-.7f*dt,.7f*dt); //40deg/s2
  float next=angle+rate*dt;
  if((desired-angle)*(desired-next)<0){next=desired;rate=0;}
  angle=Clamp(next,-.14f,.14f);return angle;
 }
};
}
struct CameraRecord;struct FixMatrix;
void CameraOperator_Reset(unsigned index);
void CameraOperator_Update(CameraRecord*,const FixMatrix*);
void CameraOperator_Apply(CameraRecord*);
#endif
