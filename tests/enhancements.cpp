#include "platform/enhancements.h"
#include "Input.h"
#include "GameInfo.h"
#define private public
#include "InstallInfo.h"
#undef private
#include "platform/platform.h"
#include "port/sys.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
namespace fs=std::filesystem;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
void write(fs::path p,std::string s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
std::string read(fs::path p){std::ifstream f(p);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(){
 EnhancementOptions e;std::string error;
 {EnhancementOptions road;std::string a[]={"test","--roads","curved"};char*av[]={a[0].data(),a[1].data(),a[2].data()};int k=1;CHECK(road.Consume(k,3,av,error)&&error.empty()&&road.roads=="curved"&&!road.NeedsPack());road.Apply();CHECK(road.overrides.empty());a[2]="original";av[2]=a[2].data();k=1;CHECK(road.Consume(k,3,av,error)&&error.empty()&&road.roads=="original");a[2]="bad";av[2]=a[2].data();k=1;CHECK(road.Consume(k,3,av,error)&&!error.empty());error.clear();}

 std::string args[]={"test","--trees","crossed","--spectators","original","--driver-names","real"};
 char *argv[7];for(int i=0;i<7;i++)argv[i]=args[i].data();
 for(int i=1;i<7;i++)CHECK(e.Consume(i,7,argv,error));
 e.Apply();CHECK(error.empty()&&e.NeedsPack());CHECK(e.overrides["video.tree_overrides"]==1&&e.overrides["video.spectator_overrides"]==0&&e.overrides["video.real_driver_names"]==1);
 args[2]="bogus";for(int i=0;i<7;i++)argv[i]=args[i].data();int i=1;CHECK(e.Consume(i,7,argv,error)&&!error.empty());
 std::string effectArgs[]={"test","--effect","dust_volume=off"};char *ef[]={effectArgs[0].data(),effectArgs[1].data(),effectArgs[2].data()};
 error.clear();i=1;CHECK(e.Consume(i,3,ef,error)&&error.empty()&&e.overrides["video.dust_volume"]==0);
 effectArgs[2]="unknown=on";ef[2]=effectArgs[2].data();i=1;CHECK(e.Consume(i,3,ef,error)&&!error.empty());
 std::string windArgs[]={"test","--tree-wind","off"};char *wv[]={windArgs[0].data(),windArgs[1].data(),windArgs[2].data()};
 error.clear();i=1;CHECK(e.Consume(i,3,wv,error)&&error.empty());e.Apply();CHECK(e.overrides["video.tree_wind"]==0);
 auto root=fs::temp_directory_path()/(std::string("cmr-enhancements-")+std::to_string(SDL_GetPerformanceCounter()));
 auto game=root/"owned game",user=root/"profile";
 write(game/"Game/Bigfiles/common.bfl","base");write(game/"FrontEnd/common.bfl","front");write(game/"Sounds/surface.bfl","sound");
 write(game/"CountrySpecific/Europe/englishtext.bfl","text");write(game/"SetupRep/640/common.bfl","setup");
 write(user/"opencmr2.ini","[video]\ndriver_name_overrides=1\nscenery_overrides=1\ntree_overrides=1\n");
 write(user/"pps/valued.pps","driver progress");write(user/"gamesave/race.rcs","race save");
 std::string ini=read(user/"opencmr2.ini");
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);
 std::string actual[]={"test","--data",game.string(),"--user-dir",user.string(),"--trees","original","--spectators","original","--driver-names","original"};
 char *av[11];for(int j=0;j<11;j++)av[j]=actual[j].data();
 CHECK(Platform_Init(11,av));CHECK(Sys_GetOption("video.tree_overrides",1)==0&&Sys_GetOption("video.spectator_overrides",1)==0&&Sys_GetOption("video.driver_name_overrides",1)==0);
 CHECK(std::string(Platform_GetUserDir())==user.string()+"/");Platform_Shutdown();
 CHECK(read(user/"pps/valued.pps")=="driver progress"&&read(user/"gamesave/race.rcs")=="race save"&&read(user/"opencmr2.ini")==ini);
 // Starting without flags resets transient overrides, not the persisted profile.
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);
 CHECK(Platform_Init(5,av));CHECK(Sys_GetOption("video.tree_overrides",1)==1);Platform_Shutdown();
 // Per-launch keys reach real gameplay polling, not persisted records/menu keys.
 write(user/"Configuration/Controller.rcf","sentinel saved controls");
 std::string bound[]={"test","--data",game.string(),"--user-dir",user.string(),"--bind-key","left=A","--bind-key","right=D"};
 char *bv[9];for(int j=0;j<9;j++)bv[j]=bound[j].data();SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(9,bv));
 auto &device=CInput::m_availableDevices[0];device.field_0x0=1;device.keyboard.field_0x468=0xcb;device.keyboard.field_0x469=0xcd;
 CGameInfo::m_unk0x0059f8d0=0;memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0x1e]=0x80;CInput::ReadKeyboardDevice(&device);CHECK(device.field_0x4==1&&device.keyboard.field_0x468==0xcb);
 memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0xcb]=0x80;CInput::ReadKeyboardDevice(&device);CHECK(device.field_0x4==0);
 CGameInfo::m_unk0x0059f8d0=1;CInput::ReadKeyboardDevice(&device);CHECK(device.field_0x4==1);
 CHECK(read(user/"Configuration/Controller.rcf")=="sentinel saved controls");
 // Explicit in-game save serializes only actual control edits, never CLI keys.
 CInstallInfo::m_isHDInstall=TRUE;strcpy(CInstallInfo::m_hdPath,".");CInput::m_unk0x005168f4[0]=0;CInput::m_unk0x005168f4[1]=1;
 memset(CInput::m_controllerInfo,0,sizeof CInput::m_controllerInfo);CInput::m_controllerInfo[0].field_0x13e[0]=0x2c;
 strcpy(CInput::m_controllerInfo[0].keyNames[0],"Z");CInput::SaveControllerInfo();auto saved=read(user/"Configuration/Controller.rcf");CHECK(saved.size()==0x11a4);
 CInput::m_controllerInfo[0].field_0x13e[0]=0;CInput::LoadControllerInfo();CHECK(CInput::m_controllerInfo[0].field_0x13e[0]==0x2c);CHECK(device.keyboard.field_0x468==0x2c);
 Platform_Shutdown();CHECK(read(user/"Configuration/Controller.rcf")==saved);
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(5,bv));CGameInfo::m_unk0x0059f8d0=0;memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0x2c]=0x80;CInput::ReadKeyboardDevice(&device);CHECK(device.field_0x4==1);Platform_Shutdown();
 CHECK(read(user/"Configuration/Controller.rcf")==saved&&read(user/"opencmr2.ini")==ini);
 fs::remove_all(root);puts("CLI parsing, transient settings, profile/save preservation: pass");return 0;
}
