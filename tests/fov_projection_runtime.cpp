#include "port/types.h"
#include "Graphics.h"
#include "CameraRecord.h"
#include "port/sys.h"
#include "platform/testing.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cmath>
extern D3DTextureManager g_textureManager;
extern CameraRecord g_viewRecords[4];extern BYTE g_activeViewRecord[4],g_playerCameraMode[8],g_droneVariant[8];
extern int g_viewZooms[2],g_viewNearPlanes[2],g_viewFarPlanes[2];
void View_SetPlayerProjection(unsigned int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
static DWORD ticks(){return 1000;}static double precise(){return 1000;}
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/"opencmr-fov-proj";
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini")<<"[video]\n";
 PlatformTestClock clock={ticks,nullptr,0,precise};Platform_SetTestClock(&clock);
 Graphics graphics={};graphics.resX=1920;graphics.resY=1080;g_pGraphics=&graphics;
 auto &tm=g_textureManager;CGraphics::m_projectionScale=1;
 g_viewZooms[0]=65536;g_viewNearPlanes[0]=65536;g_viewFarPlanes[0]=1000*65536;
 g_activeViewRecord[0]=0;g_viewRecords[0].type=4;Sys_SetOption("video.speed_effects",1);Sys_SetOption("video.speed_fov_hold",1);
 Sys_SetOption("video.speed_fov",0);View_SetPlayerProjection(0);float base=CGraphics::m_projection22;CHECK(base>0);
 Sys_SetOption("video.speed_fov",100);View_SetPlayerProjection(0);CHECK(std::abs(CGraphics::m_projection22/base-.5f)<.001f);CHECK(tm.fixedProjection[5]<base*65536*.501f);
 g_viewRecords[0].type=3;Sys_SetOption("video.speed_fov_cockpit",25);View_SetPlayerProjection(0);CHECK(std::abs(CGraphics::m_projection22/base-.8f)<.001f);
 Sys_SetOption("video.camera_operator",1);g_viewRecords[0].type=5;g_playerCameraMode[0]=4;g_droneVariant[0]=1;View_SetPlayerProjection(0);CHECK(std::abs(CGraphics::m_projection22/base-1.f)<.001f);
 Platform_SetTestClock(nullptr);g_pGraphics=nullptr;std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("actual View_SetPlayerProjection -> renderer matrix: chase100 halves scale, cockpit25 scales0.8, drone unchanged");
}
