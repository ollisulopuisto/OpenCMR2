#include "port/types.h"
#include "TreeRowFit.h"
#include <cstdio>
#include <limits>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 MeshVertexF c[4]={};for(int i=0;i<4;i++){c[i].x=(i%2)*8;c[i].y=(i/2)?0:4;c[i].z=10;c[i].textureCoordinates[0][0]=i%2;c[i].textureCoordinates[0][1]=i/2;}
 float road[]={-20,0,20,0};TreeRowFit::Frame f;
 CHECK(TreeRowFit::Build(c,road,2,f));
 float p[3];CHECK(f.Map(0,0,0,p));CHECK(p[0]==4&&p[1]==0&&p[2]>=10);
 CHECK(f.Map(0,1,.5,p));CHECK(p[1]==4&&p[2]>10);
 CHECK(f.Map(0,1,-.5,p));CHECK(p[2]>=10); // offset entire row behind original plane
 CHECK(!f.Map(100,0,0,p));CHECK(!f.Map(0,-1,0,p));CHECK(!f.Map(0,0,std::numeric_limits<float>::infinity(),p));
 CHECK(!TreeRowFit::Build(c,nullptr,0,f));
 float middle[]={-20,10,20,10};CHECK(!TreeRowFit::Build(c,middle,2,f));
 c[3].z=11;CHECK(!TreeRowFit::Build(c,road,2,f));c[3].z=10;
 c[3].textureCoordinates[0][0]=.5;CHECK(!TreeRowFit::Build(c,road,2,f));
 MeshVertexF narrow[4];for(int i=0;i<4;i++){narrow[i]=c[i];narrow[i].x=(i%2)*2;narrow[i].textureCoordinates[0][0]=i%2;}
 CHECK(TreeRowFit::Build(narrow,road,2,f));CHECK(f.SetTree(-.2f,.2f));CHECK(f.scale==4);
 float left[3],right[3],top[3];CHECK(f.Map(-.2f,0,0,left));CHECK(f.Map(.2f,0,0,right));CHECK(f.Map(0,1,0,top));CHECK(std::abs(right[0]-left[0]-1.6f)<1e-5f);CHECK(top[1]==4);
 CHECK(!f.SetTree(.2f,-.2f));
 float p0[]={0,0,10},p1[]={0,0,11},p2[]={0,0,9};CHECK(TreeRowFit::AwaySafe(p0,p1,road,2));CHECK(!TreeRowFit::AwaySafe(p0,p2,road,2));
 puts("tree-row full-canvas fit, away-only depth, invalid/ambiguous input fallback pass");
}
