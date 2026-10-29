#ifndef OPENCMR2_CAMERA_OPERATOR_H
#define OPENCMR2_CAMERA_OPERATOR_H
#include <cmath>
namespace CameraOperator {
inline const char* Label(bool enabled){return enabled?"human operator: on":"human operator: off";}
inline float Clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
inline float Wrap(float x){return std::atan2(std::sin(x),std::cos(x));}
struct State{
 bool ready=false;float heading=0,angle=0,rate=0;
 float calm=0,cooldown=0,hold=0,heldHeading=0;
 void Reset(){*this=State{};}
 float Step(float currentHeading,float travelHeading,float speedRatio,float dt){
  if(!std::isfinite(currentHeading)||!std::isfinite(travelHeading)||!std::isfinite(speedRatio)||!std::isfinite(dt)||dt<=0||dt>.2f){Reset();return 0;}
  if(!ready){ready=true;heading=currentHeading;return 0;}
  float turn=Wrap(currentHeading-heading);float previous=heading;heading=currentHeading;
  if(std::abs(turn)>.65f){Reset();return 0;} // cut, respawn, replay seek
  float measured=turn/dt;cooldown=Clamp(cooldown-dt,0.f,10.f);
  // Most turns stay framed. A distinct turn after a quiet stretch can catch
  // the operator late. No random shake or continuously oscillating target.
  if(hold<=0 && cooldown<=0 && calm>.8f && std::abs(measured)>.25f && speedRatio>0){
   heldHeading=previous;hold=.65f;cooldown=4.f;
  }
  calm=std::abs(measured)<.12f?calm+dt:0.f;
  float desired=0;
  if(hold>0){hold=Clamp(hold-dt,0.f,1.f);desired=Clamp(Wrap(heldHeading-currentHeading),-.21f,.21f);}
  // Damped catch-up deliberately permits one small overshoot after release.
  // Bounded acceleration/rate prevents a delayed turn from becoming a whip.
  float acceleration=Clamp(20.f*(desired-angle)-6.f*rate,-1.4f,1.4f);
  rate=Clamp(rate+acceleration*dt,-.45f,.45f);
  angle+=rate*dt;
  if(angle<-.21f){angle=-.21f;rate=Clamp(rate,0.f,.45f);}
  if(angle>.21f){angle=.21f;rate=Clamp(rate,-.45f,0.f);}
  if(hold<=0 && std::abs(angle)<.0005f && std::abs(rate)<.002f){angle=0;rate=0;}
  return angle;
 }
};
}
struct CameraRecord;struct FixMatrix;
bool CameraOperator_Enabled(const CameraRecord*);
void CameraOperator_Reset(unsigned index);
void CameraOperator_Update(CameraRecord*,const FixMatrix*);
void CameraOperator_Apply(CameraRecord*);
#endif
