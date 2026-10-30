// Manual owned-source execution probe. No proprietary bytes in this file.
#include "port/types.h"
#include "CurvedRoad.h"
#include "Game.h"
#include "GenericFileLoader.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstring>
static std::string sources,pack;static std::vector<std::vector<char>>files;
static void(*release)()=nullptr;static bool exact=false;static bool mismatch=false;static bool callbackFail=false;
extern "C" const char*__wrap_Sys_GetRoadDirectory(){return pack.c_str();}
extern "C" void*__wrap__ZN18CGenericFileLoader8FindFileEP11GenericFilePcPhPjh(GenericFile*,char*name,BYTE*,DWORD*n,BYTE){
 std::ifstream f(sources+"/"+name,std::ios::binary);files.emplace_back(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());auto &b=files.back();if(b.empty())return nullptr;if(mismatch)b[0]^=1;*n=b.size();return b.data();
}
extern "C" int __wrap__ZN5CGame16RegisterCallbackEPvS0_(void*p,void*){if(callbackFail)return -1;release=(void(*)())p;return 1;}
extern "C" void __wrap__Z24Track_ResearchExactEdgesb(bool enabled){exact=enabled;}
int main(int argc,char**argv){
 if(argc!=3&&argc!=4)return 2;sources=argv[1];pack=argv[2];const char*stage=argc==4?argv[3]:"fra01";files.reserve(20);GenericFile a={};
 CurvedRoad_Begin(&a,"fin10");if(CurvedRoad_Render()||CurvedRoad_Contact()||exact)return 3;
 mismatch=true;CurvedRoad_Begin(&a,stage);if(CurvedRoad_Render()||CurvedRoad_Contact()||exact)return 4;
 mismatch=false;callbackFail=true;CurvedRoad_Begin(&a,stage);if(CurvedRoad_Render()||CurvedRoad_Contact()||exact)return 11;callbackFail=false;CurvedRoad_Begin(&a,stage);if(!CurvedRoad_Render()||!release)return 5;
 if(!strcmp(stage,"fin01")){if(CurvedRoad_Contact()||exact)return 12;}else if(!CurvedRoad_Contact()||!exact)return 13;
 // A missed release cannot reuse a prior pair for a different stage.
 CurvedRoad_Begin(&a,"fin10");if(CurvedRoad_Render()||CurvedRoad_Contact())return 6;
 release();if(CurvedRoad_Render()||CurvedRoad_Contact()||exact)return 7;
 pack+="/absent";CurvedRoad_Begin(&a,stage);if(CurvedRoad_Render()||CurvedRoad_Contact()||exact)return 8;
 puts("road runtime: exact-source pair, unsupported/wrong/missing originals fallback, stale-stage guard and release pass");
}
