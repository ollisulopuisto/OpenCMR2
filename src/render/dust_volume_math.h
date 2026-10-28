#ifndef OPENCMR2_DUST_VOLUME_MATH_H
#define OPENCMR2_DUST_VOLUME_MATH_H
#include "port/gfx.h"
#include <cmath>
inline bool DustVolume_Inverse(const GfxMatrix &matrix,float *out){
 double a[4][8]={};const float *m=&matrix._11;
 for(int r=0;r<4;++r){for(int c=0;c<4;++c)a[r][c]=m[r*4+c];a[r][r+4]=1;}
 for(int c=0;c<4;++c){int best=c;for(int r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[best][c]))best=r;
  if(std::abs(a[best][c])<1e-12)return false;
  for(int j=0;j<8;++j){double t=a[c][j];a[c][j]=a[best][j];a[best][j]=t;}
  double d=a[c][c];for(int j=0;j<8;++j)a[c][j]/=d;
  for(int r=0;r<4;++r)if(r!=c){d=a[r][c];for(int j=0;j<8;++j)a[r][j]-=d*a[c][j];}}
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)out[r*4+c]=(float)a[r][c+4];return true;
}
#endif
