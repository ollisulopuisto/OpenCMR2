#include "port/types.h"
#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "RallyRoute.h"
#include "port/sys.h"
#include <cstring>
#include <cstdio>
BYTE g_playerCameraMode[8]={};BYTE g_droneVariant[8]={1,1,1,1,1,1,1,1};int g_physicsTimeStep=65536;static Car car={};static bool enabled=true;
RallyRouteNode*g_routeNodes=nullptr;int g_routeNodeCount=0;
int RallyData_GetActiveCarRaceRecordField0(Car*){return 1;}
int RallyData_GetRouteAdditionalState(){return 0;}
bool Track_CameraGroundHeight(FixVector*,int*){return false;}
Car*Car_Get(int i){return i==0?&car:nullptr;}
extern "C" int Sys_GetOption(const char*,int){return enabled;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CameraRecord record={};record.index=0;record.car=0;g_playerCameraMode[0]=4;
 FixMatrix body={};body.up.y=body.right.z=65536;car.pWorld=&body;car.speed=98304;
 CameraOperator_Update(&record,&body);CameraOperator_Apply(&record);
 body.right.x=32768;body.right.z=56756;for(int i=0;i<20;i++)CameraOperator_Update(&record,&body);CameraOperator_Apply(&record);
 {float yaw=std::atan2(record.matrix.forward.x/65536.f,record.matrix.forward.z/65536.f);CHECK(std::abs(CameraOperator::Wrap(yaw-.5235988f))<.036f);}
 CHECK(record.zoom>0xa000);
 auto once=record;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&once,sizeof record));
 float straight=record.zoom;car.steeringInput=65536;
 for(int i=0;i<80;i++)CameraOperator_Update(&record,&body);
 CameraOperator_Apply(&record);CHECK(record.zoom<straight);
 // Saved tilt offset affects orientation, not world position/lens settings.
 auto level=record;CameraOperator_Update(&record,&body,nullptr,0x5a);CameraOperator_Apply(&record);
 CHECK(record.matrix.forward.y!=level.matrix.forward.y);
 CHECK(std::abs(record.matrix.right.x/65536.f*record.matrix.forward.x/65536.f+record.matrix.right.z/65536.f*record.matrix.forward.z/65536.f)<.001f);
 // Actual runtime wiring reads route progress, not just the pure math fixture.
 RallyRouteNode nodes[5]={};const int coords[5][2]={{0,-10},{0,10},{10,20},{25,25},{40,25}};
 for(int i=0;i<5;i++){nodes[i].position.x=coords[i][0]*65536;nodes[i].position.z=coords[i][1]*65536;}
 g_routeNodes=nodes;g_routeNodeCount=5;CameraOperator_Reset(0);body.right.x=-32768;body.right.z=56756;
 for(int i=0;i<150;i++)CameraOperator_Update(&record,&body);
 CameraOperator_Apply(&record);
 {float orbit=std::atan2(-record.matrix.position.x/65536.f,-record.matrix.position.z/65536.f);
  CHECK(std::abs(CameraOperator::Wrap(orbit+.5235988f))<.036f);} // body-heading follow wins over conflicting route tangent
 g_routeNodes=nullptr;g_routeNodeCount=0;
 enabled=false;auto off=record;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record));CameraOperator_Update(&record,&body);
 enabled=true;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record)); // not ready until next simulation step
 g_droneVariant[0]=0;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record)); // classic chase variant: untouched
 g_droneVariant[0]=1;g_playerCameraMode[0]=3;CameraOperator_Apply(&record);CHECK(!memcmp(&record,&off,sizeof record));
 puts("actual operator world position/zoom apply, turn inertia, repeated draws and off reset pass");
}
