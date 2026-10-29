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
 FixMatrix body={};body.up.y=body.right.z=65536;car.pWorld=&body;car.speed=98304;
 CameraOperator_Update(&record,&body);CameraOperator_Apply(&record);auto first=record;
 body.right.x=32768;body.right.z=56756;CameraOperator_Update(&record,&body);CameraOperator_Apply(&record);
 CHECK(std::abs(record.matrix.position.x-first.matrix.position.x)<1000);CHECK(record.zoom>0xa000);
 auto once=record;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&once,sizeof record));
 float straight=record.zoom;car.steeringInput=65536;
 for(int i=0;i<80;i++)CameraOperator_Update(&record,&body);
 CameraOperator_Apply(&record);CHECK(record.zoom<straight);
 // Saved tilt offset affects orientation, not world position/lens settings.
 auto level=record;CameraOperator_Update(&record,&body,nullptr,0x5a);CameraOperator_Apply(&record);
 CHECK(record.matrix.forward.y!=level.matrix.forward.y);
 CHECK(std::abs(record.matrix.right.x/65536.f*record.matrix.forward.x/65536.f+record.matrix.right.z/65536.f*record.matrix.forward.z/65536.f)<.001f);
 enabled=false;auto off=record;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record));CameraOperator_Update(&record,&body);
 enabled=true;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record)); // not ready until next simulation step
 g_playerCameraMode[0]=3;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record));
 puts("actual operator world position/zoom apply, turn inertia, repeated draws and off reset pass");
}
