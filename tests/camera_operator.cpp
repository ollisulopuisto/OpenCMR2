#include "CameraOperator.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CameraOperator::State a,b;float previous=0,lastRate=0;
 for(int i=0;i<300;i++){
  float angle=a.Step(i*.012f,i*.012f,.8f,.04f);b.Step(i*.012f,i*.012f,.8f,.04f);
  CHECK(angle==b.angle&&std::abs(angle)<=.140001f);CHECK(std::abs(angle-previous)<=.008401f);
  CHECK(std::abs(a.rate-lastRate)<=.028001f);previous=angle;lastRate=a.rate;
 }CHECK(a.angle>.05f); // anticipatory lead, not merely trailing spring
 auto before=a.angle;CHECK(a.angle==before); // draws do not step
 for(int i=0;i<300;i++)a.Step(3.588f,3.588f,0,.04f);
 CHECK(std::abs(a.rate)<.001f);CHECK(std::abs(a.angle)<.02f);
 a.Step(6,6,1,.04f);CHECK(!a.ready&&a.angle==0);a.Step(0,0,1,0);CHECK(!a.ready);
 a.Step(0,0,1,.04f);a.Step(NAN,0,1,.04f);CHECK(!a.ready);a.Reset();CHECK(a.angle==0&&!a.ready);
 CameraOperator::State wrap;wrap.Step(3.13f,3.13f,1,.04f);wrap.Step(-3.13f,-3.13f,1,.04f);CHECK(wrap.ready&&std::abs(wrap.angle)<.01f);
 puts("camera operator: deterministic anticipation, deadband, slew/acceleration bounds, settling and reset pass");return 0;
}
