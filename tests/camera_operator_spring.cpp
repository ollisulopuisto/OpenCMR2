#include "port/types.h"
#include "CameraOperator.h"
#include "CameraRecord.h"
#include "Car.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <chrono>
extern void Scene_InitFixedMathTables();
extern void ChaseCamera_Update(CameraRecord*,FixMatrix*,int);
extern BYTE g_playerCameraMode[8];
extern FixVector g_chaseCameraOffsets[6],g_chaseCameraViewOffsets[6];
extern int g_chaseCameraLag[4],g_physicsTimeStep;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("cmr-operator-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini") << "[video]\n";
 Scene_InitFixedMathTables();
 Car car={};g_cars[0]=&car;car.surfaceNoise=65536;car.speed=0;
 FixMatrix body={};body.right.z=body.up.y=65536;body.forward.x=-65536;car.pWorld=&body;
 CameraRecord record={};record.index=record.view=record.car=0;
 g_playerCameraMode[0]=4;g_physicsTimeStep=65536;g_chaseCameraLag[0]=0xe0000;
 Sys_SetOption("video.camera_effects",0);Sys_SetOption("video.camera_operator",1);
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);
 CHECK(g_chaseCameraViewOffsets[2].x==0&&g_chaseCameraViewOffsets[2].z==65536);
 CHECK(g_chaseCameraLag[0]==0xe0000); // never mutate saved spring
 Sys_SetOption("video.camera_operator",0);
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);
 CHECK(g_chaseCameraViewOffsets[2].x!=0); // original lag is restored immediately
 CHECK(g_chaseCameraLag[0]==0xe0000);
 // Other camera modes keep original behavior even if the option is enabled.
 Sys_SetOption("video.camera_operator",1);g_playerCameraMode[0]=0;
 g_chaseCameraOffsets[2]={0,65536,0};g_chaseCameraViewOffsets[2]={65536,0,0};
 ChaseCamera_Update(&record,&body,0);CHECK(g_chaseCameraViewOffsets[2].x!=0);
 std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("actual ChaseCamera_Update custom-only spring bypass/restoration pass; native feel unverified");
 return 0;
}
