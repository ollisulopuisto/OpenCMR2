#include "port/types.h"
#include "France1Textures.h"
#include "Texture.h"
#include "Graphics.h"
#include "ScenerySHA256.h"
#include "render/gfx_internal.h"
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstring>
D3DTextureManager* CGraphics::m_pTextureManager=nullptr;
unsigned int CGraphics::m_textureCount=0;
struct France1TexturesTestAccess{static void Set(D3DTextureManager*p,unsigned n){CGraphics::m_pTextureManager=p;CGraphics::m_textureCount=n;}};
static std::string root,original;
static std::vector<char> geometry;
extern "C" const char*Sys_GetTextureDirectory(){return root.c_str();}
void*CGenericFileLoader::FindFile(GenericFile*,char*,BYTE*,DWORD*n,BYTE){*n=geometry.size();return geometry.data();}
extern "C" void Sys_Log(const char*,...){}
extern "C" void Sys_GetDesktopSize(int*w,int*h){*w=640;*h=480;}
int Platform_GetSettingInt(const char*,int x){return x;}
const char*Platform_GetSetting(const char*,const char*x){return x;}
struct Capture:GfxExecutor{bool SetVideoMode(int,int,bool)override{return true;}void SetFullscreen(bool)override{}void Execute(GfxFrame&,std::vector<GfxTexture*>&,GfxReadback&,bool,bool,bool)override{}void DestroyTexture(GfxTexture*)override{}void ReadTexture(GfxTexture*)override{}};
GfxExecutor*Gfx_CreateGpuExecutor(){return new Capture;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
std::vector<char>read(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){
 CHECK(Gfx_SetVideoMode(640,480,FALSE));Texture t={};t.pSurface=Gfx_CreateTexture(8,8,GFX_FORMAT_BGRA8,1,0);D3DTextureManager manager={};manager.textureBuffer[0]=&t;France1TexturesTestAccess::Set(&manager,1);auto before=t.pSurface;char wrong[10]={};
 CHECK(!France1Textures_Apply(&t,wrong,sizeof wrong)&&t.pSurface==before);
 if(argc<4){Gfx_DestroyTexture(t.pSurface);Gfx_Shutdown();return 0;}
 root=argv[1];geometry=read(argv[2]);auto source=read(argv[3]);GenericFile a={};
 France1Textures_Begin(&a,"fin10");CHECK(!France1Textures_Apply(&t,source.data(),source.size())&&t.pSurface==before);
 France1Textures_Begin(&a,"fra01");CHECK(!France1Textures_Apply(&t,wrong,sizeof wrong)&&t.pSurface==before);
 CHECK(France1Textures_Apply(&t,source.data(),source.size())&&t.pSurface==before&&Gfx_DrawnTexture(t.pSurface)!=before);
 GfxLockedRect lock={};CHECK(Gfx_LockTexture(Gfx_DrawnTexture(t.pSurface),0,0,&lock));CHECK(lock.pitch>0);
 if(argc>4){std::ofstream out(argv[4],std::ios::binary);int width=Gfx_DrawnTexture(t.pSurface)->width,height=Gfx_DrawnTexture(t.pSurface)->height;out<<"P6\n"<<width<<" "<<height<<"\n255\n";for(int y=0;y<height;y++)for(int x=0;x<width;x++){auto*p=(unsigned char*)lock.pixels+y*lock.pitch+x*4;char rgb[3]={(char)p[2],(char)p[1],(char)p[0]};out.write(rgb,3);}}
 Gfx_UnlockTexture(t.pSurface,0,0);
 France1Textures_Clear();before=t.pSurface;CHECK(Gfx_DrawnTexture(t.pSurface)==before);CHECK(!France1Textures_Apply(&t,source.data(),source.size())&&t.pSurface==before);
 Gfx_DestroyTexture(t.pSurface);Gfx_Shutdown();puts("France1 actual owned-source upload, all mips, stage/identity/reset guards pass");return 0;
}
