#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/Graphics.h"
#include "../game/WindshieldCrackPane.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>

static SDL_Window *testWindow;
extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
int Sys_GetOption(const char *, int fallback);   // port/sys.h shape (unused: the extern "C" stub above wins for C callers)
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
const char *Platform_GetSetting(const char *key, const char *fallback) {
    return !strcmp(key, "video.graphics_api") ? "opengl" : fallback;
}
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }
bool Gfx_GpuIsAvailable(void) { return false; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }
void TouchOverlay_RecordFrame(int, int, bool) {}   // harness: no touch overlay

extern "C" void Input_SetTouchPicture(int,int,BOOL) {}
static void require(bool ok,const char*why){if(!ok){fprintf(stderr,"FAIL %s\n",why);exit(1);}}
static const int W=96,H=64;
static MeshVertexF source[4];static MeshTriangle triangles[2];static Mesh mesh;
static GfxTexture*crack;static GfxMatrix identity={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static void render(DWORD*pixels,int selector,bool poison,bool fog=false){
 Gfx_SetTransform(GFX_TRANSFORM_WORLD,&identity);Gfx_SetTransform(GFX_TRANSFORM_VIEW,&identity);Gfx_SetTransform(GFX_TRANSFORM_PROJECTION,&identity);
 Gfx_SetRaceWorld(fog);Gfx_SetPostFog(fog,100,0);Gfx_Clear(GFX_CLEAR_TARGET|GFX_CLEAR_ZBUFFER,0xff183028,1);
 Gfx_SetStageTexCoordIndex(0,poison?(GFX_TCI_CAMERASPACEREFLECTIONVECTOR|1):0);Gfx_SetStageTextureTransform(0,poison?GFX_TTFF_COUNT3:GFX_TTFF_DISABLE);
 Gfx_SetAlphaTest(poison);Gfx_SetAlphaRef(254);Gfx_SetFog(poison);Gfx_SetBodyDirt(poison?1:0,1,0,0);Gfx_SetDistanceFade(poison?1:0,poison?2:0);
 for(auto&t:triangles)t.field_0x2c=selector;
 int n=WindshieldCrackPane::Draw(&mesh,0x100,identity,[](int){return crack;});require(n==(selector?2:0),"live exterior pane count");
 if(fog)Gfx_EndWorldEffects();
 // Harness only: invalidate the cached read, wait for THIS submission, then
 // collect. The game intentionally keeps asynchronous historical readback.
 s_readback.ready=false;
 static DWORD dummy[W*H];Gfx_ReadBackBuffer(0,0,W,H,dummy);Gfx_Present(FALSE);
 gl.Finish();
 require(Gfx_ReadBackBuffer(0,0,W,H,pixels),"fresh completed readback");
}
static void dump(const char*prefix,const char*tag,const DWORD*p){if(!prefix)return;FILE*f=fopen((std::string(prefix)+"-"+tag+".ppm").c_str(),"wb");fprintf(f,"P6\n%d %d\n255\n",W,H);for(int i=0;i<W*H;i++){fputc(p[i]>>16&255,f);fputc(p[i]>>8&255,f);fputc(p[i]&255,f);}fclose(f);}
int main(int argc,char**argv){
 const char*headless=SDL_getenv("OPENCMR2_HEADLESS");
 if(headless&&*headless!='0'){fprintf(stderr,"GL pixel test cannot use OPENCMR2_HEADLESS; unset it to run the real GL executor.\n");return 2;}
 require(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());Gfx_PrepareGlWindow();testWindow=SDL_CreateWindow("windshield crack material regression",W,H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);require(testWindow,SDL_GetError());require(Gfx_SetVideoMode(W,H,FALSE),"video");
 GfxViewport viewport={0,0,W,H,0,1};Gfx_SetViewport(&viewport);
 const float x[]={-.8f,.8f,-.8f,.8f},y[]={-.8f,-.8f,.8f,.8f};for(int i=0;i<4;i++){source[i].x=x[i];source[i].y=y[i];source[i].z=.5f;source[i].nz=-1;source[i].textureCoordinates[0][0]=i%2;source[i].textureCoordinates[0][1]=i/2;}
 triangles[0].vertexIndex[0]=0;triangles[0].vertexIndex[1]=1;triangles[0].vertexIndex[2]=2;triangles[1].vertexIndex[0]=2;triangles[1].vertexIndex[1]=1;triangles[1].vertexIndex[2]=3;for(auto&t:triangles){t.flags=0x400;t.textures[3]=3;t.textures[5]=5;}
 mesh.pVertexData=(DWORD*)source;mesh.field_0x10=4;mesh.pTriangles=triangles;mesh.triangleCount=2;
 crack=Gfx_CreateTexture(8,8,GFX_FORMAT_BGRA8,1,0);GfxLockedRect lock;require(Gfx_LockTexture(crack,0,0,&lock),"texture lock");for(int y=0;y<8;y++)for(int x=0;x<8;x++)((DWORD*)((char*)lock.pixels+y*lock.pitch))[x]=(x==y||x==7-y)?0xb0ffffff:0x00183028;Gfx_UnlockTexture(crack,0,0);
 DWORD clean[W*H],poison[W*H],intact[W*H];render(clean,3,false);render(poison,3,true);render(intact,0,true);
 DWORD fogClean[W*H],fogPoison[W*H];render(fogClean,3,false,true);render(fogPoison,3,true,true);
 require(!memcmp(fogClean,fogPoison,sizeof fogClean),"post-fog compositing depends on incoming material state");
 const char*prefix=argc>1?argv[1]:nullptr;dump(prefix,"damaged-clean",clean);dump(prefix,"damaged-reflection-incoming",poison);dump(prefix,"intact",intact);
 require(!memcmp(clean,poison,sizeof clean),"incoming reflection/material state changes crack pixels");int changed=0;for(int i=0;i<W*H;i++)if(clean[i]!=intact[i])changed++;require(changed>200,"damaged material is actually visible");
 printf("synthetic actual-helper GL pixels: clean=poisoned, intact no pane, %d changed pixels; no native replay or Metal claim\n",changed);Gfx_Shutdown();SDL_DestroyWindow(testWindow);SDL_Quit();
}
