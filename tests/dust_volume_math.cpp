#include "render/dust_volume_math.h"
#include <cstdio>
#include <cmath>
int main(){GfxMatrix m={};float inv[16];if(DustVolume_Inverse(m,inv))return 1;
 m._11=2;m._22=3;m._33=4;m._44=1;m._41=10;m._42=20;m._43=-7;
 if(!DustVolume_Inverse(m,inv))return 1;
 const float *a=&m._11;for(int r=0;r<4;++r)for(int c=0;c<4;++c){float v=0;for(int k=0;k<4;++k)v+=a[r*4+k]*inv[k*4+c];if(std::abs(v-(r==c?1.f:0.f))>1e-5)return 1;}
 puts("dust inverse: row-vector translation/scale, singular fallback pass");return 0;}
