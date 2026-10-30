#pragma once
#include <cmath>
#include <algorithm>
// Camera-only response. No FOV, controls or physics. Amount0 is exact bypass.
struct SunGlareState {
 float response=0;
 float Step(float x,float y,bool visible,int percent,float dt){
  percent=std::clamp(percent,0,100);dt=std::clamp(dt,0.f,.25f);
  if(!percent){response=0;return 0;}
  float radial=std::sqrt((x-.5f)*(x-.5f)+(y-.5f)*(y-.5f));
  float target=visible&&std::isfinite(radial)?std::clamp(1.f-radial/ .72f,0.f,1.f):0.f;
  response+=(target-response)*(1.f-std::exp(-dt/(target>response?.65f:1.1f)));
  return response*percent/100.f;
 }
};
