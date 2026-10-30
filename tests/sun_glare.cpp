#include "render/sun_glare.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){SunGlareState a,b;float first=a.Step(.5,.5,true,100,.016f);CHECK(first>0&&first<.03f);for(int i=0;i<200;i++)a.Step(.5,.5,true,100,.016f);CHECK(a.response>.98f);float before=a.response;a.Step(.5,.5,false,100,.016f);CHECK(a.response<before&&a.response>.9f);CHECK(a.Step(.5,.5,true,0,.016f)==0&&a.response==0);for(int i=0;i<200;i++)b.Step(2,2,false,100,.016f);CHECK(b.response==0);SunGlareState at25,at120;for(int i=0;i<25;i++)at25.Step(.5,.5,true,100,1.f/25);for(int i=0;i<120;i++)at120.Step(.5,.5,true,100,1.f/120);CHECK(std::abs(at25.response-at120.response)<.0001f);
 puts("sun glare eye adaptation/bypass/bounds pass");}
