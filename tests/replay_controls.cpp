#include "ReplayControls.h"
#include <cstring>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 ReplayControls c;CHECK(c.Scale()==1.0f);
 c.Slower();CHECK(c.Scale()==.5f);c.Slower();CHECK(c.Scale()==.25f);c.Slower();CHECK(c.Scale()==.25f);
 c.Faster();c.Faster();c.Faster();CHECK(c.Scale()==2.0f);for(int i=0;i<9;i++)c.Faster();CHECK(c.Scale()==8.0f);
 c.TogglePause();CHECK(c.Scale()==0.0f);c.TogglePause();CHECK(c.Scale()==8.0f);
 c.TogglePause();c.Slower();CHECK(!c.paused&&c.Scale()==4.0f); // changing speed while paused resumes
 c.Reset();CHECK(c.Scale()==1.0f&&!c.paused);
 char t[32];c.Label(t,sizeof t);CHECK(!strcmp(t,"speed 1x"));c.Slower();c.Label(t,sizeof t);CHECK(!strcmp(t,"speed 0.50x"));c.TogglePause();c.Label(t,sizeof t);CHECK(!strcmp(t,"paused"));
 // Clock: scale 1 is the real clock plus a constant; pause stops it; slow runs at the scale; never goes backwards; fast catches up.
 ReplayClock k;int real=1000;int start=k.Frame(real,1.f);CHECK(start==1000);
 int v=start;for(int i=0;i<60;i++){real+=17;int n=k.Frame(real,1.f);CHECK(n-v==17);v=n;}
 int paused=v;for(int i=0;i<60;i++){real+=17;CHECK(k.Frame(real,0.f)==paused);}
 int s0=k.Frame(real,.5f);for(int i=0;i<100;i++){real+=16;k.Frame(real,.5f);}int s1=k.Frame(real,.5f);CHECK(std::abs((s1-s0)-800)<=2);
 int f0=s1;for(int i=0;i<100;i++){real+=16;int n=k.Frame(real,4.f);CHECK(n>=f0);f0=n;}CHECK(std::abs((f0-s1)-6400)<=4);
 char tm[16];ReplayClock_FormatCentiseconds(11760,tm,sizeof tm);CHECK(!strcmp(tm,"1:57.60"));ReplayClock_FormatCentiseconds(-5,tm,sizeof tm);CHECK(!strcmp(tm,"0:00.00"));
 // Packet runs: byte 2 low 6 bits = steps in the run (0 counts as 1).
 unsigned char pk[16]={0,0,3,0, 0,0,0,0, 0,0,0x3f,0, 0,0,0x80|1,0};
 CHECK(ReplayPacketSteps(pk)==3&&ReplayPacketSteps(pk+4)==1&&ReplayPacketSteps(pk+8)==63&&ReplayPacketSteps(pk+12)==1);
 CHECK(ReplayStepsTotal(pk,4)==68);CHECK(ReplayStepsBefore(pk,2,0)==4);CHECK(ReplayStepsBefore(pk,2,5)==9);CHECK(ReplayStepsBefore(pk,0,0)==0);
 puts("replay controls ok");return 0;}
