#include "CameraOperator.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CameraOperator::State a,b;float previous=0,lastRate=0;
 // Calm lead-in, late turn, then stable heading: delayed following and one
 // small overcorrection, not permanently anticipatory framing.
 bool lag=false,overshoot=false;float heading=0;
 for(int i=0;i<300;i++){
  if(i>=30&&i<60)heading+=.012f;
  float angle=a.Step(heading,heading,.8f,.04f);b.Step(heading,heading,.8f,.04f);
  CHECK(angle==b.angle&&std::abs(angle)<=.210001f);CHECK(std::abs(angle-previous)<=.018001f);
  CHECK(std::abs(a.rate-lastRate)<=.056001f || std::abs(angle)>.209f);previous=angle;lastRate=a.rate;
  if(i>30&&i<60&&angle<-.02f)lag=true;
  if(i>60&&angle>.001f)overshoot=true;
 }
 CHECK(lag&&overshoot);CHECK(a.angle==0&&a.rate==0);
 a.Step(2,2,1,.04f);CHECK(!a.ready&&a.angle==0);a.Step(0,0,1,0);CHECK(!a.ready);
 a.Step(0,0,1,.04f);a.Step(NAN,0,1,.04f);CHECK(!a.ready);a.Reset();CHECK(a.angle==0&&!a.ready);
 CameraOperator::State left,right;
 for(int i=0;i<50;i++){float h=(i>30?i-30:0)*.012f;left.Step(h,h,1,.04f);right.Step(-h,-h,1,.04f);}
 CHECK(std::abs(left.angle+right.angle)<.0001f&&left.angle<0&&right.angle>0);
 left.Reset();CHECK(left.angle==0&&right.angle>0); // independent views
 CameraOperator::State gentle;
 for(int i=0;i<200;i++)gentle.Step(i*.003f,i*.003f,1,.04f);
 CHECK(gentle.angle==0&&gentle.hold==0); // ordinary gentle following stays framed
 CameraOperator::State parked;
 for(int i=0;i<100;i++)parked.Step((i>30?i-30:0)*.012f,0,0,.04f);
 CHECK(parked.angle==0); // stationary body changes do not trigger missed turns
 CameraOperator::State wrap;wrap.Step(3.13f,3.13f,1,.04f);wrap.Step(-3.13f,-3.13f,1,.04f);CHECK(wrap.ready&&std::abs(wrap.angle)<.01f);
 puts("camera operator: deterministic late turn, overshoot, slew/acceleration bounds, settling and reset pass");return 0;
}
