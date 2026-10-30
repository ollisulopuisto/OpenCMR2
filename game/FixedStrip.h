#ifndef OPENCMR2_FIXED_STRIP_H
#define OPENCMR2_FIXED_STRIP_H
#include "MeshVertex.h"
#include "ScenerySourceFit.h"
#include <vector>
// Visual-only tessellation. Source perimeter is exactly fixed; no contact,
// collision records, placement, bounding box or physics vertices are edited.
namespace FixedStrip {
struct Vertex { MeshVertexF source; float depth,weight;float lo[3],hi[3];unsigned short indices[3];float bary[3]; };
inline bool Build(const MeshVertexF *v,int count,const unsigned short *ix,int n,std::vector<Vertex>&out,bool sourceWarp=false){
 if(!v||!ix||count<4||count>2000||n<6||n>12000||n%6)return false;
 std::vector<Vertex> result;
 for(int p=0;p<n;p+=6){
  unsigned short ids[4];int used=0;
  for(int j=0;j<6;j++){if(ix[p+j]>=count)return false;bool found=false;for(int k=0;k<used;k++)if(ids[k]==ix[p+j])found=true;if(!found){if(used==4)return false;ids[used++]=ix[p+j];}}
  if(used!=4)return false;MeshVertexF q[4];for(int k=0;k<4;k++)q[k]=v[ids[k]];
  ScenerySourceFit::Fit fit;
  if(!ScenerySourceFit::Build(q,4,fit)){
   // Exact-source France quads are intentionally warped. Keep each original
   // triangle domain, never flatten/reproject its original diagonal or edges.
   if(!sourceWarp)return false;unsigned corners=0;float height2=0;
   for(int k=0;k<4;k++){
    float u=q[k].textureCoordinates[0][0],sv=q[k].textureCoordinates[0][1];
    if((u!=0&&u!=1)||(sv!=0&&sv!=1))return false;
    unsigned bit=1u<<(int(u)+2*int(sv));if(corners&bit)return false;corners|=bit;
    const float*pos=(const float*)&q[k];for(int c=0;c<6;c++)if(!std::isfinite(pos[c]))return false;
   }
   if(corners!=15)return false;float normal[3]={};
   for(int t=0;t<2;t++){
    const float*a=(const float*)&v[ix[p+t*3]],*b=(const float*)&v[ix[p+t*3+1]],*c=(const float*)&v[ix[p+t*3+2]];
    float ab[3],ac[3];for(int k=0;k<3;k++){ab[k]=b[k]-a[k];ac[k]=c[k]-a[k];}
    float nn[3]={ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]};
    float length=std::sqrt(nn[0]*nn[0]+nn[1]*nn[1]+nn[2]*nn[2]);if(!std::isfinite(length)||length<1e-5)return false;
    for(int k=0;k<3;k++)normal[k]+=nn[k]/length;
   }
   float length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);if(length<1)return false;
   for(int k=0;k<3;k++)fit.normal[k]=normal[k]/length;
   for(int a=0;a<4;a++)for(int b=a+1;b<4;b++)if(q[a].textureCoordinates[0][0]==q[b].textureCoordinates[0][0]){
    float h2=0;for(int k=0;k<3;k++){float d=((float*)&q[a])[k]-((float*)&q[b])[k];h2+=d*d;}height2=std::max(height2,h2);
   }
   fit.height=std::sqrt(height2);if(fit.height<1e-5)return false;
  }
  // Keep the exact two original diagonal domains and their winding/colours.
  const int steps=3;
  for(int t=0;t<2;t++){
   const auto&a=v[ix[p+t*3]],&b=v[ix[p+t*3+1]],&c=v[ix[p+t*3+2]];
   auto point=[&](int i,int j){float wb=float(i)/steps,wc=float(j)/steps,wa=1-wb-wc;Vertex o={};for(int k=0;k<3;k++)o.indices[k]=ix[p+t*3+k];o.bary[0]=wa;o.bary[1]=wb;o.bary[2]=wc;const float*aa=(const float*)&a,*bb=(const float*)&b,*cc=(const float*)&c;float*oo=(float*)&o.source;
    for(int k=0;k<6;k++)oo[k]=aa[k]*wa+bb[k]*wb+cc[k]*wc;
    for(int set=0;set<2;set++)for(int k=0;k<2;k++)o.source.textureCoordinates[set][k]=a.textureCoordinates[set][k]*wa+b.textureCoordinates[set][k]*wb+c.textureCoordinates[set][k]*wc;
    for(int colour=0;colour<2;colour++){const auto*ac=(const unsigned char*)&a.diffuseColour+colour*4;const auto*bc=(const unsigned char*)&b.diffuseColour+colour*4;const auto*ccol=(const unsigned char*)&c.diffuseColour+colour*4;auto*oc=(unsigned char*)&o.source.diffuseColour+colour*4;for(int k=0;k<4;k++)oc[k]=(unsigned char)(ac[k]*wa+bc[k]*wb+ccol[k]*wc+.5f);}
    float u=o.source.textureCoordinates[0][0],sv=o.source.textureCoordinates[0][1];float edge=std::sin(3.14159265f*u)*std::sin(3.14159265f*sv);if(u<=0||u>=1||sv<=0||sv>=1)edge=0;
    o.depth=edge*std::sin(u*15.7079633f)*std::min(.15f,fit.height*.015f);o.weight=edge*(1-sv)*(1-sv);
    for(int k=0;k<3;k++){
     o.lo[k]=o.hi[k]=((const float*)&q[0])[k];for(int b=1;b<4;b++){float x=((const float*)&q[b])[k];o.lo[k]=std::min(o.lo[k],x);o.hi[k]=std::max(o.hi[k],x);}
     oo[k]=std::max(o.lo[k],std::min(o.hi[k],oo[k]+fit.normal[k]*o.depth));
    }
    return o;
   };
   for(int i=0;i<steps;i++)for(int j=0;j<steps-i;j++){
    result.push_back(point(i,j));result.push_back(point(i+1,j));result.push_back(point(i,j+1));
    if(i+j<steps-1){result.push_back(point(i+1,j));result.push_back(point(i+1,j+1));result.push_back(point(i,j+1));}
   }
  }
 }
 out=std::move(result);return !out.empty();
}
}
#endif
