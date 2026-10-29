#include "port/types.h"
#include "port/sys.h"
#include "platform/testing.h"
#include "StageBlock.h"
#include "StageWeather.h"
#include "GraphicsOptions.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <chrono>
extern void CarInterior_UpdateWiperSweep(int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("cmr-wiper-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini") << "[video]\n";
 ViewWeatherState weather[2]={};g_viewWeather=weather;weather[0].kind=1;weather[0].intensity=0xffff;
 short maximum=1000;g_carInteriorProfilePointers[0].firstWiperMaxAngle=&maximum;
 auto &w=g_carWiperStates[0];w={0,1,0,0};BYTE keys[256]={};Platform_SetTestKeyboard(keys);
 Sys_SetOption("video.windscreen_wiper_key",0x11);Sys_SetOption("video.windscreen_wipers",1);Sys_SetOption("video.windscreen_dirt",0);
 CarInterior_UpdateWiperSweep(0);CHECK(w.mode==3&&w.angle>0); // heavy rain real blade path
 keys[0x11]=0x80;WindscreenWiper_UpdateInput();CHECK(!Sys_GetOption("video.windscreen_wipers",1));
 CarInterior_UpdateWiperSweep(0);CHECK(w.mode==1);WindscreenWiper_UpdateInput();CHECK(!Sys_GetOption("video.windscreen_wipers",1)); // held
 for(int i=0;i<100;i++)CarInterior_UpdateWiperSweep(0);
 CHECK(w.mode==0&&w.angle==0); // rain cannot restart after parked
 keys[0x11]=0;WindscreenWiper_UpdateInput();keys[0x11]=0x80;WindscreenWiper_UpdateInput();CHECK(Sys_GetOption("video.windscreen_wipers",0));
 CarInterior_UpdateWiperSweep(0);CHECK(w.mode==3&&w.angle>0); // second edge resumes real blades
 keys[0x11]=0;WindscreenWiper_UpdateInput();Sys_SetOption("video.windscreen_wipers",0);for(int i=0;i<100;i++)CarInterior_UpdateWiperSweep(0);CHECK(w.mode==0&&w.angle==0); // menu option same control
 // Light rain retains original slow speed; both cockpit slots obey off.
 g_carInteriorProfilePointers[1].firstWiperMaxAngle=&maximum;
 weather[1].kind=1;weather[1].intensity=0x5000;g_carWiperStates[1]={0,1,0,0};
 Sys_SetOption("video.windscreen_wipers",1);CarInterior_UpdateWiperSweep(1);CHECK(g_carWiperStates[1].mode==2);
 Sys_SetOption("video.windscreen_wipers",0);for(int i=0;i<100;i++){CarInterior_UpdateWiperSweep(0);CarInterior_UpdateWiperSweep(1);}
 CHECK(g_carWiperStates[0].angle==0&&g_carWiperStates[1].angle==0&&g_carWiperStates[1].mode==0);
 // Saved remap controls the same cockpit path, not only the overlay.
 Sys_SetOption("video.windscreen_wiper_key",0x13);keys[0x11]=0;WindscreenWiper_UpdateInput();keys[0x11]=0x80;WindscreenWiper_UpdateInput();CHECK(!Sys_GetOption("video.windscreen_wipers",1));
 keys[0x13]=0x80;WindscreenWiper_UpdateInput();CHECK(Sys_GetOption("video.windscreen_wipers",0));CarInterior_UpdateWiperSweep(1);CHECK(g_carWiperStates[1].mode==2&&g_carWiperStates[1].angle>0);
 Platform_SetTestKeyboard(nullptr);std::filesystem::current_path(old);std::filesystem::remove_all(tmp);puts("actual rain/W held/release/off/park/on path passes independent of dirt");return 0;
}
