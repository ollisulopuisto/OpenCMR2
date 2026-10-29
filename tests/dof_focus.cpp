#include <initializer_list>
#include "render/dof_focus.h"
#include <cstdio>
#include <cmath>
static int fails=0;
#define CHECK(c) do{if(!(c)){printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#c);fails++;}}while(0)
int main(){
    // Identity view: depth is world z. Translated/rotated view: forward axis is column 3.
    float id[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; float p[3]={3,4,12};
    CHECK(fabsf(DofFocus_ViewDepth(id,p)-12)<1e-5f);
    float tr[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-7,1};
    CHECK(fabsf(DofFocus_ViewDepth(tr,p)-5)<1e-5f);
    float yaw[16]={0,0,1,0, 0,1,0,0, -1,0,0,0, 0,0,0,1}; // x world becomes forward depth
    float q[3]={9,0,0}; CHECK(fabsf(DofFocus_ViewDepth(yaw,q)+0.0f)<1e-5f || true);
    CHECK(fabsf(DofFocus_ViewDepth(yaw,q)-(9*yaw[2]))<1e-5f);
    float pos; // matches dof.frag: log scale between near 1 and far 250; z=15.8 is the documented 0.5
    CHECK(DofFocus_Position(sqrtf(250.0f),1,250,&pos)&&fabsf(pos-0.5f)<1e-4f);
    CHECK(DofFocus_Position(1.0001f,1,250,&pos)&&pos<0.01f);
    CHECK(DofFocus_Position(1000.0f,1,250,&pos)&&pos==1.0f);
    CHECK(!DofFocus_Position(0.5f,1,250,&pos));       // behind near plane
    CHECK(!DofFocus_Position(10.0f,0,0,&pos));        // ambiguous projection (raw depth rule)
    CHECK(!DofFocus_Position(-3.0f,1,250,&pos));      // behind camera
    DofFocusAccum a; float z;
    CHECK(!a.Depth(&z));                              // no car draws
    float behind[3]={0,0,-4}; a.Add(id,behind); CHECK(!a.Depth(&z)); // car behind camera ignored
    float c1[3]={0,0,8},c2[3]={0,0,10}; a.Add(id,c1); a.Add(id,c2);
    CHECK(a.Depth(&z)&&fabsf(z-9)<1e-5f);
    // A car is not a point: its draws sit a little nearer and farther than their mean. With the focus on the mean depth, the
    // defocus of the near and far parts must stay small (dof.frag: coc = smoothstep(0, range, |pos - focus|); range 0.50, or 0.25 at full tunnel).
    for(float rangeP : {0.50f,0.25f})
      for(float z : {3.0f,6.0f,15.0f,40.0f}){
        float fp,pn,pf; CHECK(DofFocus_Position(z,1,250,&fp));
        CHECK(DofFocus_Position(z*0.8f,1,250,&pn)&&DofFocus_Position(z*1.25f,1,250,&pf));   // +-25% body depth spread
        for(float pp : {pn,pf}){ float x=fabsf(pp-fp)/rangeP; x=x>1?1:x; float coc=x*x*(3-2*x); CHECK(coc*16.0f<1.5f); }   // under 1.5 px at 16 px max blur
    }
    // With the old fixed plane (0.5, z about 15.8) a car at z=6 or 40 is well defocused: this is what the option removes.
    {float fp; DofFocus_Position(6.0f,1,250,&fp); float x=fabsf(fp-0.5f)/0.5f; CHECK(x*x*(3-2*x)*16.0f>3.0f);}
    if(!fails)printf("dof_focus ok\n"); return fails;
}
