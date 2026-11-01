#include "port/types.h"
#include "CameraRecord.h"
#include "port/sys.h"
#include <cstdio>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
extern CameraRecord g_viewRecords[4];
extern BYTE g_activeViewRecord[4],g_playerCameraMode[8],g_droneVariant[8];
bool View_IsDroneCamera(int);
int View_SpeedFovZoom(BYTE,int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("drone-active-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini") << "[video]\n";
 Sys_SetOption("video.camera_operator",1);Sys_SetOption("video.speed_fov",100);
 for(int player=0;player<2;player++)for(int slot=0;slot<2;slot++){
  for(int r=0;r<8;r++)g_playerCameraMode[r]=g_droneVariant[r]=0;
  g_activeViewRecord[player]=slot;int r=player*2+slot;
  g_viewRecords[r].type=5;g_playerCameraMode[r]=4;g_droneVariant[r]=1;
  CHECK(View_IsDroneCamera(player));CHECK(View_SpeedFovZoom(player,65536)==65536);
  g_droneVariant[r]=0;CHECK(!View_IsDroneCamera(player));
  g_droneVariant[r]=1;g_viewRecords[r].type=4;CHECK(!View_IsDroneCamera(player));
 }
 CHECK(!View_IsDroneCamera(-1)&&!View_IsDroneCamera(2));
 std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("active player camera slot drone identity and speed-FOV bypass pass");
}
