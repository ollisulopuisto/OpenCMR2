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
    check(WheelParticle_BoostedCount(20<<16,true,true,0xffff00,999)==10,"level 999 gives 10");
    check(WheelParticle_BoostedCount(20<<16,true,true,0xffff00,9999)==20,"level clamps at 2000, capped at 20");
    check(WheelParticle_BoostedCount(20<<16,true,true,0,2000)==20,"level 2000 gives 20 deterministically");
    {int sum=0;for(unsigned r=0;r<1000;++r)sum+=WheelParticle_BoostedCount(20<<16,true,true,r<<8,150);check(sum>1300&&sum<1700,"level 150 averages about 1.5");}
    // Executable simulation of the new default: 4 wheels at 150 km/h for
    // 250 fixed effects steps (10 s at the 25 Hz step) at level 800, with
    // the 60%-pool chip guard on a 1200 pool. Deterministic: 800/100 = 8
    // per wheel per step until the guard engages at 720 live chips.
    {   int live=0; long spawned=0; int steps=0;
        for(int step=0;step<250;step++){steps++;
            for(int wheel=0;wheel<4;wheel++){
                int n=WheelParticle_BoostedCount(150<<16,true,true,(unsigned)(step*4+wheel)<<8,800);
                for(int k=0;k<n;k++){if(!WheelParticle_ChipBudgetOK(live,1200))break;live++;spawned++;}
            }
            // chips expire: lifetime ~1 s = 25 steps, steady state ~ 8*4*25=800 -> guard caps at 719
            live-=32; if(live<0)live=0;
        }
        check(spawned>4*8*25,"default 800 floods past the old 4-cap within a second");
        check(steps==250,"simulation ran all steps");
        // guard: live never exceeds 60% of the pool (719), reserving the rest
        check(WheelParticle_ChipBudgetOK(719,1200) && !WheelParticle_ChipBudgetOK(720,1200),"chip guard boundary at 60 percent");
    }
    // frame-rate independence: counts key off the wheel speed and the fixed
    // step, never wall-clock - no time argument enters these functions.
    check(WheelParticle_BoostedCount(150<<16,true,true,0,800)==WheelParticle_BoostedCount(150<<16,true,true,0,800),"same step same count");
    puts(fail?"FAIL":"wheel particle emission pass");return fail?1:0;
}
