#include "platform/enhancements.h"
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
 std::string args[]={"test","--trees","crossed","--spectators","original","--driver-names","real"};
 char *argv[7];for(int i=0;i<7;i++)argv[i]=args[i].data();
 for(int i=1;i<7;i++)CHECK(e.Consume(i,7,argv,error));
 e.Apply();CHECK(error.empty()&&e.NeedsPack());CHECK(e.overrides["video.tree_overrides"]==1&&e.overrides["video.spectator_overrides"]==0&&e.overrides["video.real_driver_names"]==1);
 args[2]="bogus";for(int i=0;i<7;i++)argv[i]=args[i].data();int i=1;CHECK(e.Consume(i,7,argv,error)&&!error.empty());
 std::string effectArgs[]={"test","--effect","dust_volume=off"};char *ef[]={effectArgs[0].data(),effectArgs[1].data(),effectArgs[2].data()};
 error.clear();i=1;CHECK(e.Consume(i,3,ef,error)&&error.empty()&&e.overrides["video.dust_volume"]==0);
 effectArgs[2]="unknown=on";ef[2]=effectArgs[2].data();i=1;CHECK(e.Consume(i,3,ef,error)&&!error.empty());
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
 fs::remove_all(root);puts("CLI parsing, transient settings, profile/save preservation: pass");return 0;
}
