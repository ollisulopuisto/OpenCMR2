#include "port/types.h"
#include "ScenerySourceFit.h"
#include <cstdio>
#include <limits>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 MeshVertexF v[4]={};float xyz[4][2]={{-3,0},{3,0},{3,10},{-3,10}},uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
 for(int i=0;i<4;i++){v[i].x=xyz[i][0];v[i].y=xyz[i][1];v[i].textureCoordinates[0][0]=uv[i][0];v[i].textureCoordinates[0][1]=uv[i][1];}
 ScenerySourceFit::Fit f;CHECK(ScenerySourceFit::Build(v,4,f));float p[3];f.Map(.5,1,0,p);CHECK(p[0]==0&&p[1]==0);f.Map(1,0,0,p);CHECK(p[0]==3&&p[1]==10);
 for(auto &q:v)q.textureCoordinates[0][0]=1-q.textureCoordinates[0][0];CHECK(ScenerySourceFit::Build(v,4,f));f.Map(1,0,0,p);CHECK(p[0]==-3&&p[1]==10);
 for(auto &q:v)q.textureCoordinates[0][0]=.5f+.5f*q.textureCoordinates[0][0];CHECK(!ScenerySourceFit::Build(v,4,f));
 for(int i=0;i<4;i++)v[i].textureCoordinates[0][0]=uv[i][0];v[3].z=1;CHECK(!ScenerySourceFit::Build(v,4,f));v[3].z=0;
 v[2].textureCoordinates[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!ScenerySourceFit::Build(v,4,f));
 v[2].textureCoordinates[0][0]=uv[2][0];v[3].z=std::numeric_limits<float>::quiet_NaN();CHECK(!ScenerySourceFit::Build(v,4,f));v[3].z=0;
 v[2].textureCoordinates[0][0]=2;CHECK(!ScenerySourceFit::Build(v,4,f));
 puts("full/flipped source fit; cropped/nonplanar/nonfinite fallback: pass");
}
