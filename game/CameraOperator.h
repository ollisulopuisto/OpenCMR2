#ifndef OPENCMR2_CAMERA_OPERATOR_H
#define OPENCMR2_CAMERA_OPERATOR_H
#include <cmath>
namespace CameraOperator {
inline const char* Label(bool enabled){return enabled?"human operator: on":"human operator: off";}
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
struct State{
 bool ready=false;V3 position,velocity,aim,aimVelocity,lastCar;
 float heading=0,headingRate=0,lastHeading=0,turn=0,zoom=1.7f,zoomRate=0,pitch=0;
 void Reset(){*this=State{};}
 void Step(V3 car,float carHeading,float steering,float speed,float distance,float height,float dt){
  if(!std::isfinite(steering)||!std::isfinite(speed)||!std::isfinite(distance)||!std::isfinite(height)||!std::isfinite(carHeading)||!std::isfinite(dt)||dt<=0||dt>.2f||!std::isfinite(car.x)||!std::isfinite(car.y)||!std::isfinite(car.z)){Reset();return;}
  distance=Clamp(distance*2.8f,16.f,32.f);height=Clamp(height*2.f,5.f,10.f);
  float delta=ready?Wrap(carHeading-lastHeading):0;
  if(ready&&((car-lastCar).Length()>25.f||std::abs(delta)>.8f)){Reset();delta=0;}
  if(!ready){ready=true;heading=lastHeading=carHeading;lastCar=car;position=car+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};aim=car+V3{0,1,0};velocity={};}
  float measured=delta/dt;lastHeading=carHeading;
  turn+=(Clamp(std::abs(measured)*1.8f+std::abs(steering)*.8f,0.f,1.f)-turn)*(1-std::exp(-dt/.3f));
  // Independent trailing orbit heading: car can rotate without rotating camera.
  float accel=Clamp(3.f*Wrap(carHeading-heading)-3.f*headingRate,-.7f,.7f);
  headingRate=Clamp(headingRate+accel*dt,-.65f,.65f);heading=Wrap(heading+headingRate*dt);
  V3 desired=car+V3{-std::sin(heading)*distance,height,-std::cos(heading)*distance};
  // Second-order world-space chase. Never re-add car position at draw time.
  V3 acceleration=(desired-position)*4.f-velocity*4.f;
  float magnitude=acceleration.Length();if(magnitude>35.f)acceleration=acceleration*(35.f/magnitude);
  velocity=velocity+acceleration*dt;position=position+velocity*dt;
  // Keep framing safe on very long high-speed transients, but do not rigidly
  // rotate the position offset with the car. This is a tether, not attachment.
  V3 away=position-car;float separation=away.Length();
  if(separation>distance*2.5f){position=car+away*(distance*2.5f/separation);}
  V3 target=car+V3{0,1,0};
  V3 aimAcceleration=(target-aim)*12.f-aimVelocity*5.5f;
  aimVelocity=aimVelocity+aimAcceleration*dt;aim=aim+aimVelocity*dt;
  // Steering can widen before the body turns. Lens is actual projection zoom,
  // independent of viewpoint distance. Never random or an FOV pulse per frame.
  float speedRatio=Clamp(speed/1.5f,0.f,1.f);
  float wanted=1.55f+.3f*speedRatio-.4f*turn;
  zoomRate+=(8.f*(wanted-zoom)-5.f*zoomRate)*dt;
  zoom=Clamp(zoom+zoomRate*dt,1.15f,1.9f);lastCar=car;
 }
};
}
struct CameraRecord;struct FixMatrix;struct FixVector;
bool CameraOperator_Enabled(const CameraRecord*);
void CameraOperator_Reset(unsigned index);
void CameraOperator_Update(CameraRecord*,const FixMatrix*,const FixVector* offset=nullptr,short pitch=0x2d);
void CameraOperator_Apply(CameraRecord*);
#endif
