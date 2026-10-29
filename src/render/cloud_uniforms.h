#pragma once
#include "dust_volume_math.h"
#include <cstring>
struct CloudUniforms {float inverse[16],camera[4],layer[4],sun[4],target[4];};
inline bool CloudUniforms_Build(const GfxCommand &c,const GfxDrawState &s,int w,int h,CloudUniforms &u){
 for(float f:c.clouds)if(!std::isfinite(f))return false;
 if(!c.raceWorld || c.clouds[0]<=0 || s.projection._34==0)return false;
 int sw=s.screenW?s.screenW:w,sh=s.screenH?s.screenH:h;
 if(s.viewport.x||s.viewport.y||s.viewport.width!=(DWORD)sw||s.viewport.height!=(DWORD)sh)return false;
 GfxMatrix vp;for(int r=0;r<4;r++)for(int col=0;col<4;col++){(&vp._11)[r*4+col]=0;for(int k=0;k<4;k++)(&vp._11)[r*4+col]+=(&s.view._11)[r*4+k]*(&s.projection._11)[k*4+col];}
 float inverseView[16];if(!DustVolume_Inverse(vp,u.inverse)||!DustVolume_Inverse(s.view,inverseView))return false;
 u.camera[0]=inverseView[12];u.camera[1]=inverseView[13];u.camera[2]=inverseView[14];u.camera[3]=c.clouds[5];
 u.layer[0]=c.clouds[1];u.layer[1]=c.clouds[1]+180.f;u.layer[2]=c.clouds[0];u.layer[3]=.025f;
 u.sun[0]=c.clouds[2];u.sun[1]=c.clouds[3];u.sun[2]=c.clouds[4];u.sun[3]=0;
 u.target[0]=1.f/w;u.target[1]=1.f/h;u.target[2]=u.target[3]=0;return true;
}
