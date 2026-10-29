#include "WheelParticleEmission.h"
#include <cstdio>
#include <climits>
int main(){
    int fail=0;
    auto check=[&](bool v,const char *s){if(!v){puts(s);++fail;}};
    check(WheelParticle_Count(0,true,true,0)==0,"stationary");
    check(WheelParticle_Count(20<<16,false,true,0)==0,"airborne");
    check(WheelParticle_Count(20<<16,true,false,0)==0,"dry hard surface");
    check(WheelParticle_Count(10<<16,true,true,0)==0,"threshold");
    int low=0,high=0;for(unsigned r=0;r<12;++r){low+=WheelParticle_Count(12<<16,true,true,r);high+=WheelParticle_Count(20<<16,true,true,r);}
    check(low==3,"nonzero lower band25percent");check(high==4,"upper band one third");
    check(WheelParticle_Count(-(20<<16),true,true,0)==1,"reverse");
    check(WheelParticle_Count(INT_MIN,true,true,0)==1,"negative overflow safe");
    int speed[4]={20<<16,20<<16,0,0};
    for(int i=0;i<4;++i)check(WheelParticle_Count(speed[i],true,true,0)==(i<2?1:0),"front spin independent of stationary rear");
    speed[0]=speed[1]=0;speed[2]=speed[3]=20<<16;
    for(int i=0;i<4;++i)check(WheelParticle_Count(speed[i],true,true,0)==(i>=2?1:0),"stationary front no stale rear count");
    // Optional stronger debris: level 0 is the original, same gates, bounded.
    for(unsigned r=0;r<50;++r){check(WheelParticle_BoostedCount(20<<16,true,true,r,0)==WheelParticle_Count(20<<16,true,true,r),"level 0 unchanged");}
    check(WheelParticle_BoostedCount(20<<16,false,true,5,300)==0,"airborne stays 0");
    check(WheelParticle_BoostedCount(20<<16,true,false,5,300)==0,"hard surface stays 0");
    check(WheelParticle_BoostedCount(5<<16,true,true,5,300)==WheelParticle_Count(5<<16,true,true,5),"slow wheel unchanged");
    check(WheelParticle_BoostedCount(20<<16,true,true,0,300)==3,"level 300 gives 3 at low remainder");
    check(WheelParticle_BoostedCount(20<<16,true,true,0xffff00,999)==4,"capped at 4");
    {int sum=0;for(unsigned r=0;r<1000;++r)sum+=WheelParticle_BoostedCount(20<<16,true,true,r<<8,150);check(sum>1300&&sum<1700,"level 150 averages about 1.5");}
    puts(fail?"FAIL":"wheel particle emission pass");return fail?1:0;
}
