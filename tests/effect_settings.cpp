#include "port/types.h"
#include "render/effect_settings.h"
#include <map>
#include <string>
#include <cstdio>
static std::map<std::string,int> settings;static int blur,amount,bloom,grade,shadow,soft,particles,dof,rays;
extern "C" int Sys_GetOption(const char*k,int fallback){auto i=settings.find(k);return i==settings.end()?fallback:i->second;}
extern "C" void Gfx_SetMotionBlur(BOOL on,int mix){blur=on;amount=mix;}
extern "C" void Gfx_SetBloom(BOOL on,int){bloom=on;}
extern "C" void Gfx_SetParticles(int type,int){particles=type;}
extern "C" void Gfx_SetShadowQuality(int size,int softness){shadow=size;soft=softness;}
extern "C" void Gfx_SetShadows(int,int){}
extern "C" void Gfx_SetDepthOfField(BOOL on,int,int,int){dof=on;}
extern "C" void Gfx_SetGodRays(BOOL on,int){rays=on;}
extern "C" void Gfx_SetColorGrading(int temp,int,int sat,int contrast){grade=temp+sat+contrast-200;}
#define CHECK(x) do{if(!(x)){printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 EffectSettings::Apply();CHECK(blur&&bloom&&particles==1&&amount==40&&shadow==1024&&soft==25&&grade==0&&!dof&&!rays);CHECK(settings.empty());
 settings={{"video.motion_blur",0},{"video.bloom",0},{"video.color_grading",1},{"video.shadow_quality",2},{"video.shadow_softness",200},{"video.depth_of_field",1},{"video.god_rays",1},{"video.particles",2}};
 auto before=settings;EffectSettings::Apply();CHECK(!blur&&!bloom&&grade>0&&shadow==2048&&soft==100&&dof&&rays&&particles==2);CHECK(before==settings);
 settings["video.grade_amount"]=0;EffectSettings::Apply();CHECK(grade==0);puts("saved effect startup restore, nonneutral grade, bounds and no writes pass");
}
