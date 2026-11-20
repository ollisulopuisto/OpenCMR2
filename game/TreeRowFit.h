#ifndef OPENCMR2_TREE_ROW_FIT_H
#define OPENCMR2_TREE_ROW_FIT_H
#include "ScenerySourceFit.h"
#include "BrushRelief.h"
#include <algorithm>
namespace TreeRowFit {
inline bool AwaySafe(const float *base,const float *moved,const float *route,int count){
 auto distance=[&](const float*p){float best=1e30f;for(int i=0;i<count-1;i++){float ax=route[i*2],az=route[i*2+1],dx=route[i*2+2]-ax,dz=route[i*2+3]-az,l=dx*dx+dz*dz;float t=l>1e-8f?((p[0]-ax)*dx+(p[2]-az)*dz)/l:0;t=std::max(0.f,std::min(1.f,t));float x=p[0]-ax-t*dx,z=p[2]-az-t*dz;best=std::min(best,x*x+z*z);}return best;};
 if(!route||!base||!moved||count<2||count>100000)return false;
 for(int k=0;k<3;k++)if(!std::isfinite(base[k])||!std::isfinite(moved[k]))return false;
 return distance(moved)+1e-4f>=distance(base);
}
struct Frame {
 ScenerySourceFit::Fit fit;float away[3]={},front=-.5f,scale=1,width=1,center=0,anchor=.5f;bool perTree=false;
 bool SetTree(float lo,float hi){
  if(!std::isfinite(lo)||!std::isfinite(hi)||lo<-.5f||hi>.5f||hi-lo<1e-5f)return false;
  center=(lo+hi)*.5f;scale=std::min(fit.height,.98f*width/(hi-lo));
  float radius=(hi-lo)*.5f*scale/width;anchor=std::max(radius,std::min(1-radius,.5f+center));perTree=true;return true;
 }
 void Base(float x,float *out)const {
  fit.Map(perTree?anchor:.5f+x*scale/width,1,0,out);
  if(perTree){out[0]+=(x-center)*scale*fit.du[0]/width;out[2]+=(x-center)*scale*fit.du[2]/width;}
 }
 bool Map(float x,float y,float z,float *out)const {
  if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||std::abs(x)>.5f||y<0||y>1.001f||std::abs(z)>.5f)return false;
  Base(x,out);
  for(int k=0;k<3;k++){out[k]+=-fit.dv[k]/fit.height*y*scale+away[k]*scale*(z-front);if(!std::isfinite(out[k]))return false;}
  return true;
 }
};
inline bool Build(const MeshVertexF *c,const float *route,int count,Frame &out){
 Frame f;if(!c||!route||count<2||count>100000||!ScenerySourceFit::Build(c,4,f.fit))return false;
 for(int k=0;k<2*count;k++)if(!std::isfinite(route[k]))return false;
 // Current fixed cards have vertical stems. Do not tilt trunks to fit an unverified card.
 if(std::abs(f.fit.dv[0])+std::abs(f.fit.dv[2])>f.fit.height*.001f||f.fit.dv[1]>=0)return false;
 for(int i=0;i<4;i++)if(c[i].textureCoordinates[0][0]!=(i%2)||c[i].textureCoordinates[0][1]!=(i/2))return false;
 float a[]={c[0].x,c[0].y,c[0].z},b[]={c[1].x,c[1].y,c[1].z};
 if(!BrushRelief::AwayFromRoad(a,b,f.fit.normal,route,count,60.f,f.away))return false;
 f.width=std::hypot(f.fit.du[0],f.fit.du[2]);if(!std::isfinite(f.width)||f.width<1e-5f)return false;f.scale=std::min(f.width,f.fit.height);
 out=f;return true;
}
}
#endif
