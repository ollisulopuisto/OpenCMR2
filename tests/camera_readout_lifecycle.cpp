#include "port/types.h"
#include "Car.h"
#include "CameraRecord.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
#include <cmath>
extern Car *g_cars[40],*g_carBuffer;
extern int g_carCount;
extern CameraRecord g_viewStates[2];
void View_GetCameraCarOffset(unsigned,double*,double*,double*,double*);
int View_CameraMoveStep(int,int);
int View_SpeedFovZoom(BYTE,int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 double d,f,s,u;Car cars[2]={};
 auto zero=[&](unsigned view){d=f=s=u=99;View_GetCameraCarOffset(view,&d,&f,&s,&u);return d==0&&f==0&&s==0&&u==0;};
 // Freed-stage slots are intentionally poison. No read through them is allowed.
 g_carBuffer=nullptr;g_carCount=0;g_cars[0]=(Car*)1;g_cars[1]=(Car*)1;
 CHECK(zero(0)&&zero(1)&&zero(2)&&zero(256));
 Sys_SetOption("video.camera_move_chase",100);Sys_SetOption("video.speed_fov",100);
 CHECK(View_CameraMoveStep(5,0)==0);View_SpeedFovZoom(0,65536);
 // New allocation before spawn: matrix pointer can still contain old heap bits.
 g_carBuffer=cars;g_carCount=2;g_cars[0]=cars;g_cars[1]=cars+1;
 cars[0].pWorld=(FixMatrix*)0xbeffffffbf00001full;
 CHECK(zero(0));CHECK(View_CameraMoveStep(5,0)==0);
 // Slot pointing outside this generation is not a live car.
 g_cars[0]=(Car*)1;CHECK(zero(0));g_cars[0]=cars;
 for(unsigned v=0;v<2;v++){
  Car &c=cars[v];c.pWorld=&c.physicsMatrix;c.pWorld->right.x=65536;c.pWorld->up.y=65536;c.pWorld->forward.z=65536;
  g_viewStates[v].matrix.position={3*65536,4*65536,12*65536};
  View_GetCameraCarOffset(v,&d,&f,&s,&u);CHECK(d==13&&f==12&&s==3&&u==4);
 }
 // Stage exit with legacy dangling slots, then a smaller next-stage allocation.
 g_carBuffer=nullptr;g_carCount=0;CHECK(zero(0)&&zero(1));
 g_carBuffer=cars;g_carCount=1;CHECK(zero(1));
 puts("camera readout freed/reallocated/unspawned/split-view lifecycle guards pass");
}
