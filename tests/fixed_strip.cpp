#include "port/types.h"
#include "FixedStrip.h"
#include <cstdio>
#include <cstring>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 MeshVertexF v[4]={};v[0].textureCoordinates[0][0]=1;v[0].textureCoordinates[0][1]=1;v[0].x=10;
 v[1].textureCoordinates[0][1]=1;v[2].textureCoordinates[0][0]=1;v[2].x=10;v[2].y=10;v[3].y=10;
 for(auto&q:v){q.diffuseColour=0xff334455;q.specularColour=0xff001122;q.nz=1;}
 auto original=v[0];unsigned short ix[]={0,1,2,3,2,1};std::vector<FixedStrip::Vertex>out;CHECK(FixedStrip::Build(v,4,ix,6,out));CHECK(out.size()==54);
 bool relief=false;for(auto&q:out){CHECK(std::isfinite(q.source.x)&&std::isfinite(q.source.y)&&std::isfinite(q.source.z));float u=q.source.textureCoordinates[0][0],vv=q.source.textureCoordinates[0][1];CHECK(u>=-1e-6&&u<=1.000001f&&vv>=-1e-6&&vv<=1.000001f);if(u<=0||u>=1||vv<=0||vv>=1)CHECK(q.depth==0&&q.weight==0);relief|=fabs(q.depth)>.001;CHECK(q.source.diffuseColour==0xff334455);CHECK(q.source.x>=0&&q.source.x<=10&&q.source.y>=0&&q.source.y<=10&&q.source.z==0);}
 CHECK(relief);for(auto&q:out){CHECK(q.indices[0]<4&&q.indices[1]<4&&q.indices[2]<4);CHECK(fabs(q.bary[0]+q.bary[1]+q.bary[2]-1)<1e-5);}
 CHECK(relief);CHECK(!memcmp(&v[0],&original,sizeof original));v[2].z=2;v[3].z=2;CHECK(FixedStrip::Build(v,4,ix,6,out));bool zrelief=false;for(auto&q:out){float base=0;for(int k=0;k<3;k++)base+=v[q.indices[k]].z*q.bary[k];zrelief|=fabs(q.source.z-base)>1e-4;CHECK(q.source.z>=q.lo[2]&&q.source.z<=q.hi[2]);}CHECK(zrelief);v[2].z=v[3].z=0;
 ix[0]=9;CHECK(!FixedStrip::Build(v,4,ix,6,out));ix[0]=0;v[3].z=1;CHECK(!FixedStrip::Build(v,4,ix,6,out));CHECK(FixedStrip::Build(v,4,ix,6,out,true));
 for(auto&q:out){float u=q.source.textureCoordinates[0][0],sv=q.source.textureCoordinates[0][1];if(u<=0||u>=1||sv<=0||sv>=1){float xyz[3]={};for(int b=0;b<3;b++)for(int c=0;c<3;c++)xyz[c]+=((float*)&v[q.indices[b]])[c]*q.bary[b];CHECK(fabs(q.source.x-xyz[0])<1e-5&&fabs(q.source.y-xyz[1])<1e-5&&fabs(q.source.z-xyz[2])<1e-5);}}
 v[3].textureCoordinates[0][0]=.5f;CHECK(!FixedStrip::Build(v,4,ix,6,out,true));puts("fixed strip full-canvas fit, immutable source, finite relief, perimeter and malformed guards pass");
}
