// OpenCMR2: the cached tree wind (WindVertex/WindFrame/ApplyWind) must give the same result as the original per-vertex Offset() and RustleUV().
#include "SceneryMotion.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
static int failures;
static float rnd(){return rand()/(float)RAND_MAX;}
int main(void){
    float worstX=0,worstUV=0;
    for(int n=0;n<200000;n++){
        float bottom=rnd()*5-2,height=.5f+rnd()*30,y=bottom+(rnd()*1.4f-.2f)*height,u=rnd()*1.2f-.1f,v=rnd()*1.2f-.1f;
        uint64_t ticks=(uint64_t)(rnd()*200000.f)*65536ull/7;uint32_t phase=(uint32_t)rand()*2654435761u;float strength=n%7==0?0.f:rnd()*2;
        float du0,dv0;SceneryMotion::RustleUV(u,v,height>0?(y-bottom)/height:0,ticks,phase,strength,&du0,&dv0);
        float dx0=SceneryMotion::Offset(y,bottom,height,ticks,phase,true);
        auto wv=SceneryMotion::MakeWindVertex(u,v,y,bottom,height);auto wf=SceneryMotion::MakeWindFrame(height,ticks,phase,strength);
        float dx,du,dv;SceneryMotion::ApplyWind(wv,wf,&dx,&du,&dv);
        worstX=std::fmax(worstX,std::fabs(dx-dx0)/(height*.045f));worstUV=std::fmax(worstUV,std::fmax(std::fabs(du-du0),std::fabs(dv-dv0)));
        if(!std::isfinite(dx)||!std::isfinite(du)||!std::isfinite(dv)){printf("FAIL nonfinite\n");failures++;break;}
    }
    printf("worst x error %.3g of crown sway, worst uv error %.3g (uv amplitude is up to 0.008)\n",worstX,worstUV);
    if(worstX>1e-4f){printf("FAIL: x differs\n");failures++;}
    if(worstUV>2e-5f){printf("FAIL: uv differs\n");failures++;}
    // edge cases: invalid extent and non finite input move nothing, like the originals
    auto wv=SceneryMotion::MakeWindVertex(.5f,.5f,10,0,0);auto wf=SceneryMotion::MakeWindFrame(0,0,1,1);float dx,du,dv;SceneryMotion::ApplyWind(wv,wf,&dx,&du,&dv);
    if(dx!=0||du!=0||dv!=0){printf("FAIL: zero height must not move\n");failures++;}
    wv=SceneryMotion::MakeWindVertex(NAN,.5f,5,0,10);wf=SceneryMotion::MakeWindFrame(10,100,1,1);SceneryMotion::ApplyWind(wv,wf,&dx,&du,&dv);
    if(du!=0||dv!=0||!std::isfinite(dx)){printf("FAIL: nan u must not move the texture\n");failures++;}
    puts(failures?"FAILED":"ok");return failures?1:0;
}
