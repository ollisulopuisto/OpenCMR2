#ifndef OPENCMR2_SCENERY_SOURCE_FIT_H
#define OPENCMR2_SCENERY_SOURCE_FIT_H
#include "MeshVertex.h"
#include <cmath>
namespace ScenerySourceFit {
struct Fit {
 float origin[3],du[3],dv[3],normal[3],height;
 float u0=1e9f,u1=-1e9f,v0=1e9f,v1=-1e9f;
 void Map(float u,float v,float depth,float *out)const{
  for(int k=0;k<3;k++)out[k]=origin[k]+du[k]*u+dv[k]*v+normal[k]*height*depth;
 }
};
inline bool Build(const MeshVertexF *verts,int count,Fit &out){
 if(!verts||count<3||count>64)return false;
 const auto &a=verts[0];int b=-1,c=-1;float determinant=0;
 for(int i=1;i<count&&c<0;i++)for(int j=i+1;j<count;j++){
  float bu=verts[i].textureCoordinates[0][0]-a.textureCoordinates[0][0];
  float bv=verts[i].textureCoordinates[0][1]-a.textureCoordinates[0][1];
  float cu=verts[j].textureCoordinates[0][0]-a.textureCoordinates[0][0];
  float cv=verts[j].textureCoordinates[0][1]-a.textureCoordinates[0][1];
  determinant=bu*cv-bv*cu;if(std::isfinite(determinant)&&std::abs(determinant)>1e-6f){b=i;c=j;break;}
 }
 if(c<0)return false;Fit f;
 float bu=verts[b].textureCoordinates[0][0]-a.textureCoordinates[0][0],bv=verts[b].textureCoordinates[0][1]-a.textureCoordinates[0][1];
 float cu=verts[c].textureCoordinates[0][0]-a.textureCoordinates[0][0],cv=verts[c].textureCoordinates[0][1]-a.textureCoordinates[0][1];
 float av[]={a.x,a.y,a.z},bb[]={verts[b].x,verts[b].y,verts[b].z},cc[]={verts[c].x,verts[c].y,verts[c].z};
 for(int k=0;k<3;k++){
  f.du[k]=((bb[k]-av[k])*cv-(cc[k]-av[k])*bv)/determinant;
  f.dv[k]=((cc[k]-av[k])*bu-(bb[k]-av[k])*cu)/determinant;
  f.origin[k]=av[k]-f.du[k]*a.textureCoordinates[0][0]-f.dv[k]*a.textureCoordinates[0][1];
 }
 f.normal[0]=f.du[1]*f.dv[2]-f.du[2]*f.dv[1];f.normal[1]=f.du[2]*f.dv[0]-f.du[0]*f.dv[2];f.normal[2]=f.du[0]*f.dv[1]-f.du[1]*f.dv[0];
 float n=0,h=0;for(int k=0;k<3;k++){n+=f.normal[k]*f.normal[k];h+=f.dv[k]*f.dv[k];}
 n=std::sqrt(n);f.height=std::sqrt(h);if(!std::isfinite(n)||n<1e-5f||!std::isfinite(f.height)||f.height<1e-5f)return false;
 for(int k=0;k<3;k++)f.normal[k]/=n;
 for(int i=0;i<count;i++){
  float u=verts[i].textureCoordinates[0][0],v=verts[i].textureCoordinates[0][1],p[3];f.Map(u,v,0,p);
  float xyz[]={verts[i].x,verts[i].y,verts[i].z};
  if(!std::isfinite(u)||!std::isfinite(v))return false;
  for(int k=0;k<3;k++)if(!std::isfinite(p[k])||std::abs(p[k]-xyz[k])>f.height*.001f)return false;
  f.u0=(f.u0<u?f.u0:u);f.u1=(f.u1>u?f.u1:u);f.v0=(f.v0<v?f.v0:v);f.v1=(f.v1>v?f.v1:v);
 }
 // Cropped atlas cards need authored per-crop geometry. Do not distort an
 // entire tree into that subrectangle or reuse a neighbour's silhouette.
 if(f.u0>.02f||f.u1<.98f||f.v0>.02f||f.v1<.98f)return false;
 out=f;return true;
}
}
#endif
