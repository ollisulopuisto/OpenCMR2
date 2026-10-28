#include "port/types.h"
#include "WorldDust.h"
#include <cstdio>
#include <cstring>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 if(argc==3&&!std::strcmp(argv[1],"--fixture")){
  WorldDust::Pool pool;
  for(int t=0;t<200;t++){float z=t*.25f;pool.Emit(8*std::sin(z*.06f),0,z,.66,.46,.22);pool.Step(.04f);}
  GfxDustLobe lobes[32];int n=pool.Sample(lobes,32);FILE *f=std::fopen(argv[2],"w");if(!f)return 1;
  std::fputs("[",f);for(int i=0;i<n;i++){if(i)std::fputs(",",f);std::fputs("[",f);
   for(int k=0;k<4;k++)std::fprintf(f,"%s%g",k?",":"",lobes[i].centreRadius[k]);
   for(int k=0;k<4;k++)std::fprintf(f,",%g",lobes[i].colourDensity[k]);std::fputs("]",f);}
  std::fputs("]",f);std::fclose(f);return 0;
 }
WorldDust::Pool pool;pool.Emit(10,2,20,.6,.4,.2);GfxDustLobe before[32]={},after[32]={};
 CHECK(pool.Sample(before,32)==1);for(int i=0;i<150;i++)pool.Step(.04f);
 CHECK(pool.Sample(after,32)==1);CHECK(after[0].centreRadius[0]==10&&after[0].centreRadius[2]==20);
 CHECK(after[0].centreRadius[3]>=6.5);CHECK(after[0].centreRadius[1]-after[0].centreRadius[3]<2);
 GfxDustLobe copy[32]={};pool.Sample(copy,32);CHECK(!memcmp(copy,after,sizeof after));
 // Sampling for any camera/reflection cannot advance or rebase the world.
 for(int i=0;i<20;i++){pool.Sample(copy,32);CHECK(!memcmp(copy,after,sizeof after));}
 for(int i=0;i<160;i++)pool.Step(.04f);CHECK(pool.Sample(after,32)==0);
 for(int i=0;i<100;i++)pool.Emit(i*10,0,0,.6,.4,.2);CHECK(pool.Sample(after,32)==32);
 pool.Clear();CHECK(pool.Sample(after,32)==0);puts("fixed terrain anchor, large lingering pool, read-only sampling, expiry and clear: pass");}
