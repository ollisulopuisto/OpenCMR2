#include "render/gfx_internal.h"
#include <cstdio>
#include <cstring>
extern "C" int Sys_GetOption(const char *,int v){return v;}
int Platform_GetSettingInt(const char*,int v){return v;}
const char *Platform_GetSetting(const char*,const char*v){return v;}
extern "C" void Sys_Log(const char*,...){}
extern "C" void Sys_GetDesktopSize(int*w,int*h){*w=640;*h=480;}
extern "C" unsigned Sys_GetTicks(){return 0;}
static bool supported=true;static int seen=0;
struct Executor:GfxExecutor{
 bool SetVideoMode(int,int,bool)override{return true;}void SetFullscreen(bool)override{}
 bool SupportsDustVolume()override{return supported;}
 void DestroyTexture(GfxTexture*)override{}void ReadTexture(GfxTexture*)override{}
 void Execute(GfxFrame&f,std::vector<GfxTexture*>&,GfxReadback&,bool,bool,bool)override{
  for(auto &c:f.commands)if(c.type==GFX_CMD_DUST_VOLUME){seen++;if(c.dustCount!=32||f.dustLobes[c.dustOffset].centreRadius[0]!=7)seen=-100;}
 }
};
GfxExecutor *Gfx_CreateGpuExecutor(){return new Executor;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){unsetenv("OPENCMR2_HEADLESS");CHECK(Gfx_SetVideoMode(640,480,FALSE));
 GfxMatrix id={};id._11=id._22=id._33=id._44=1;Gfx_SetTransform(GFX_TRANSFORM_VIEW,&id);
 GfxDustLobe lobes[40]={};lobes[0].centreRadius[0]=7;
 CHECK(!Gfx_DrawDustVolume(lobes,40)); // orthographic
 GfxMatrix perspective=id;perspective._34=1;perspective._44=0;perspective._43=-.1;
 Gfx_SetTransform(GFX_TRANSFORM_PROJECTION,&perspective);CHECK(Gfx_DrawDustVolume(lobes,40));
 lobes[0].centreRadius[0]=99; // command owns a copy
 supported=false;CHECK(!Gfx_DrawDustVolume(lobes,40));supported=true;
 GfxViewport split={0,0,320,480,0,1};Gfx_SetViewport(&split);CHECK(!Gfx_DrawDustVolume(lobes,40));
 split.width=640;Gfx_SetViewport(&split);GfxMatrix zero={};Gfx_SetTransform(GFX_TRANSFORM_VIEW,&zero);CHECK(!Gfx_DrawDustVolume(lobes,40));
 Gfx_Present(FALSE);CHECK(seen==1);Gfx_Shutdown();puts("dust recorder: count32, immutable snapshot, unsupported/split/ortho/singular rejection: pass");}
