#include "port/types.h"
#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "port/sys.h"
#include <cstring>
#include <cstdio>
BYTE g_playerCameraMode[8]={};int g_physicsTimeStep=65536;static Car car={};static bool enabled=true;
Car*Car_Get(int i){return i==0?&car:nullptr;}
extern "C" int Sys_GetOption(const char*,int){return enabled;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CameraRecord record={};record.index=0;record.car=0;g_playerCameraMode[0]=4;
 FixMatrix body={};body.up.y=65536;car.pWorld=&body;car.speed=65536;
 for(int i=0;i<100;i++){
  float a=i*.012f;body.right.x=std::lround(std::sin(a)*65536);body.right.z=std::lround(std::cos(a)*65536);
  car.velocity=body.right;CameraOperator_Update(&record,&body);
 }
 record.matrix.right.x=record.matrix.up.y=record.matrix.forward.z=65536;record.matrix.position={65536,131072,196608};auto base=record;
 CameraOperator_Apply(&record);CHECK(record.matrix.forward.x!=0);CHECK(!memcmp(&record.matrix.position,&base.matrix.position,sizeof(FixVector)));CHECK(record.matrix.up.y==65536);
 auto once=record;record=base;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&once,sizeof record)); // no time advance in matrix building
 enabled=false;record=base;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&base,sizeof record));CameraOperator_Update(&record,&body);
 enabled=true;record=base;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&base,sizeof record)); // disable reset
 g_playerCameraMode[0]=3;record=base;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&base,sizeof record));
 puts("camera operator runtime: actual update/apply, custom-only, position/roll retained, repeated draws and disable reset pass");return 0;
}
