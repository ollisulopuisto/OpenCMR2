#ifndef OPENCMR2_SCENERY_MIP_H
#define OPENCMR2_SCENERY_MIP_H
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
namespace SceneryMip {
// BGRA, straight alpha. Average premultiplied color to keep hidden RGB out.
// Keep the nearest attainable source >=128 coverage in each smaller level.
inline std::vector<uint8_t> Down(const std::vector<uint8_t>&src,unsigned w,unsigned h,double coverage){
 unsigned nw=std::max(1u,w/2),nh=std::max(1u,h/2);std::vector<uint8_t> dst(nw*nh*4);unsigned hist[256]={};
 for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++){
  uint64_t a=0,c[3]={};unsigned count=0;
  // Integer partition includes the final row/column for odd dimensions.
  for(unsigned sy=y*h/nh;sy<(y+1)*h/nh;sy++)for(unsigned sx=x*w/nw;sx<(x+1)*w/nw;sx++){
   auto p=&src[(sy*w+sx)*4];a+=p[3];for(int k=0;k<3;k++)c[k]+=unsigned(p[k])*p[3];++count;
  }
  auto p=&dst[(y*nw+x)*4];p[3]=(a+count/2)/count;for(int k=0;k<3;k++)p[k]=a?(c[k]+a/2)/a:0;hist[p[3]]++;
 }
 unsigned target=unsigned(std::lround(coverage*nw*nh));double best=1,bestChange=0;unsigned have=0;
 for(int a=128;a<256;a++)have+=hist[a];unsigned bestError=have>target?have-target:target-have;
 for(int threshold=1;threshold<256;threshold++)if(hist[threshold]){
  double scale=127.5/threshold;unsigned covered=0;for(int a=1;a<256;a++)if(a*scale>=127.5)covered+=hist[a];
  unsigned error=covered>target?covered-target:target-covered;double change=std::abs(scale-1);
  if(error<bestError||(error==bestError&&change<bestChange)){best=scale;bestError=error;bestChange=change;}
 }
 for(size_t i=3;i<dst.size();i+=4)dst[i]=std::min(255,int(dst[i]*best+.5));return dst;
}
}
#endif
