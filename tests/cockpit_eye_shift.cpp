#include "CockpitEyeShift.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);fails++;}}while(0)
int main(){int fails=0;
 CHECK(CockpitEyeShift_Amount(65536,100)==0&&CockpitEyeShift_Amount(131072,0)==0&&CockpitEyeShift_Amount(60000,100)==0);
 CHECK(CockpitEyeShift_Amount(131072,100)==kCockpitEyeShiftMax&&CockpitEyeShift_Amount(131072,-100)==-kCockpitEyeShiftMax);
 CHECK(CockpitEyeShift_Amount(131072,500)==kCockpitEyeShiftMax&&CockpitEyeShift_Amount(300000,100)==kCockpitEyeShiftMax);
 CHECK(CockpitEyeShift_Amount(98304,100)==kCockpitEyeShiftMax/2);
 if(!fails)puts("cockpit_eye_shift ok");return fails;}
