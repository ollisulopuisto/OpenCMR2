#include <SDL3/SDL_timer.h>
#include "port/types.h"
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#define private public
#include "Graphics.h"
#undef private
#include "SceneryPack.h"
#include "ScenerySHA256.h"
#include <filesystem>
#include <chrono>
#include "SceneryPackFormat.h"
#include <fstream>
#include <sstream>
#include "Sector.h"
#include "MeshVertex.h"
#include "port/sys.h"
#include "render/gfx_internal.h"
static bool separate=false,legacy=false,windEnabled=true;static std::string base; static unsigned draws; static GfxMatrix world;static GfxTexture *bound;static std::vector<GfxVertex> captured;
D3DTextureManager* CGraphics::m_pTextureManager=nullptr;
unsigned int CGraphics::m_textureCount=0;
struct SysFile { FILE *file; };
extern "C" SysFile *Sys_OpenFile(const char *p,int){FILE *f=fopen(p,"rb");
 if(f&&legacy&&std::string(p).find("tree.obj")!=std::string::npos){
  FILE *copy=tmpfile();char line[256];fgets(line,sizeof line,f);int ch;while((ch=fgetc(f))!=EOF)fputc(ch,copy);fclose(f);rewind(copy);f=copy;
 }return f?new SysFile{f}:nullptr;}
extern "C" BOOL Sys_HasSceneryDirectory(){return separate;}
extern "C" SysFile *Sys_OpenSceneryFile(const char*p){return Sys_OpenFile((base+"/"+p).c_str(),SYS_FILE_READ);}
extern "C" void Sys_CloseFile(SysFile *f){fclose(f->file);delete f;}
extern "C" DWORD Sys_GetFileSize(SysFile *f){fseek(f->file,0,SEEK_END);long n=ftell(f->file);rewind(f->file);return n;}
extern "C" DWORD Sys_ReadFile(SysFile *f,void *b,DWORD n){return fread(b,1,n,f->file);}
extern "C" int Sys_GetOption(const char *key,int f){return !strcmp(key,"video.tree_wind")?int(windEnabled):f;}
extern "C" const char *Sys_GetOptionText(const char*,const char*){return base.c_str();}
static std::string logs;
extern "C" void Sys_Log(const char*fmt,...){char b[2048];va_list ap;va_start(ap,fmt);vsnprintf(b,sizeof b,fmt,ap);va_end(ap);logs+=std::string(b)+"\n";}
extern "C" void Sys_GetDesktopSize(int *w,int *h){*w=640;*h=480;}
int Platform_GetSettingInt(const char*,int x){return x;}
const char *Platform_GetSetting(const char*,const char *x){return x;}
struct Capture: GfxExecutor {
 bool SetVideoMode(int,int,bool)override{return true;} void SetFullscreen(bool)override{}
 void Execute(GfxFrame &frame,std::vector<GfxTexture*>&,GfxReadback&,bool,bool,bool)override{
 for(auto &c:frame.commands)if(c.type==GFX_CMD_DRAW){draws++;auto &s=frame.states[c.state];world=s.world;bound=s.stages[0].texture;const GfxVertex *v=(const GfxVertex*)(frame.vertices.data()+c.firstVertexByte);captured.assign(v,v+c.vertexCount);}}
 void DestroyTexture(GfxTexture*)override{} void ReadTexture(GfxTexture*)override{}
};
GfxExecutor *Gfx_CreateGpuExecutor(){return new Capture;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){if(argc<2)return 0;base=argv[1];legacy=argc>2&&!strcmp(argv[2],"legacy");separate=argc>2&&!strcmp(argv[2],"separate");CHECK(Gfx_SetVideoMode(640,480,FALSE));
 bool identityMode=argc>2&&!strcmp(argv[2],"identity");
 std::filesystem::path temporary;
 if(identityMode){
   CHECK(SceneryIdentity::SHA256("",0)=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
   CHECK(SceneryIdentity::SHA256("abc",3)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
   CHECK(SceneryIdentity::SHA256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",56)=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
   std::string multi(1000,'a');CHECK(SceneryIdentity::SHA256(multi.data(),multi.size())=="41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
   temporary=std::filesystem::temp_directory_path()/("cmr-identity-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
   std::filesystem::copy(base,temporary,std::filesystem::copy_options::recursive);base=temporary.string();
   std::ofstream wind(temporary/"tree-wind-identities.tsv");wind<<"frahit.bfl/tre_unknown.dds\t3\t"<<SceneryIdentity::SHA256("ghi",3)<<"\n";wind.close();
   std::ofstream ids(temporary/"source-identities.tsv");ids<<"frahit.bfl/tre_md1.dds\t3\t"<<SceneryIdentity::SHA256("abc",3)<<"\nfrahit.bfl/spc_mn9a.dds\t3\t"<<SceneryIdentity::SHA256("def",3)<<"\n";ids.close();
 }
 D3DTextureManager manager={};CGraphics::m_pTextureManager=&manager;CGraphics::m_textureCount=1;
 Texture source={};source.width=source.height=256;strcpy(source.name,"C:\\textures\\TRE_MD1.TGA");GenericFile archive={};source.pArchive=&archive;manager.textureBuffer[0]=&source;
 MeshTriangle triangle={};triangle.vertexIndex[1]=1;triangle.vertexIndex[2]=2;MeshVertexF verts[3]={};verts[1].y=2;verts[2].x=1;verts[0].textureCoordinates[0][1]=1;verts[2].textureCoordinates[0][0]=1;verts[2].textureCoordinates[0][1]=1;Mesh mesh={};mesh.flags=2;mesh.pTriangles=&triangle;mesh.triangleCount=1;mesh.pVertexData=(DWORD*)verts;mesh.field_0x10=3;
 StageObject object={};object.pMesh=&mesh;object.matrix.right.x=object.matrix.up.y=object.matrix.forward.z=65536;object.matrix.position.x=3*65536;
 GfxMatrix billboard={};billboard._11=billboard._22=billboard._33=billboard._44=1;billboard._41=99;
 CHECK(!SceneryPack_Draw(&object,(float*)&billboard));SceneryPack_RegisterArchive(&archive,"DATA/FRAHIT.BFL");
 if(identityMode){
   // Actual stage archive is not the recipe's source container.
   SceneryPack_RegisterArchive(&archive,"DATA/FRA01HI.BFL");
   CHECK(!SceneryPack_Draw(&object,(float*)&billboard));
   SceneryPack_RegisterSource(&source,"bad",3);CHECK(!SceneryPack_Draw(&object,(float*)&billboard));
   SceneryPack_RegisterSource(&source,"abc",3);CHECK(SceneryPack_Draw(&object,(float*)&billboard));
   // Loose texture, no runtime archive; identity still reaches the animation.
   source.pArchive=nullptr;strcpy(source.name,"spc_mn9a.dds");SceneryPack_RegisterSource(&source,"def",3);
   CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);auto firstFrame=bound;
   for(int i=0;i<5;i++)SceneryPack_Advance(65536);
   CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(bound!=firstFrame);
   unsigned long long tick=0;unsigned frame=0,changes=0;CHECK(SceneryPack_GetAnimationStatus(&tick,&frame,&changes));CHECK(tick==5&&changes>0);
   for(int i=0;i<25;i++)SceneryPack_Advance(65536);
   CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(SceneryPack_GetAnimationStatus(&tick,&frame,&changes)&&tick==30&&changes>1);
   CHECK(logs.find("bound-frame-changes=")!=std::string::npos);
   strcpy(source.name,"tre_unknown.dds");source.pSurface=bound;SceneryPack_RegisterSource(&source,"ghi",3);
   CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);auto beforeWind=captured;auto originalX=verts[1].x;
   for(int i=0;i<20;i++)SceneryPack_Advance(65536);
   CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);
   CHECK(captured[0].x==beforeWind[0].x&&captured[2].x==beforeWind[2].x); // fixed base
   CHECK(captured[1].x!=beforeWind[1].x&&verts[1].x==originalX); // moving top, immutable source
   CHECK(logs.find("tree wind active")!=std::string::npos);
   if(argc>3){std::ofstream evidence(argv[3]);for(auto &v:beforeWind)evidence<<"before "<<v.x<<" "<<v.y<<" "<<v.z<<"\n";for(auto &v:captured)evidence<<"after "<<v.x<<" "<<v.y<<" "<<v.z<<"\n";}

   windEnabled=false;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));windEnabled=true;
   SceneryPack_RegisterSource(&source,"bad",3);CHECK(!SceneryPack_Draw(&object,(float*)&billboard));
   unsigned td=0,wd=0,og=0;SceneryPack_GetTreeStatus(&td,&wd,&og);CHECK(wd==2&&td==1);
   strcpy(source.name,"spc_mn9a.dds");
   SceneryPack_RegisterSource(&source,"xyz",3);CHECK(!SceneryPack_Draw(&object,(float*)&billboard));
   CHECK(logs.find("spectator sidecar active")!=std::string::npos);
   SceneryPack_Clear();SceneryPack_RegisterArchive(&archive,"DATA/FRAHIT.BFL");source.pArchive=&archive;
   CHECK(!SceneryPack_Draw(&object,(float*)&billboard)); // new pack must not bypass missing identity
   Gfx_Shutdown();std::filesystem::remove_all(temporary);puts("exact-byte scenery routing: stage archives, loose textures, animation frames, modified bytes and reset pass");return 0;
 }

 if(legacy){CHECK(!SceneryPack_Draw(&object,(float*)&billboard));
 CHECK(logs.find("frahit.bfl/tre_md1")!=std::string::npos&&logs.find("source-canvas-1 header")!=std::string::npos);
 auto logged=logs;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));CHECK(logs==logged);strcpy(source.name,"spc_mn9a.dds");CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(draws==1);SceneryPack_Clear();Gfx_Shutdown();puts("headerless legacy tree rejected; spectator preserved: pass");return 0;}
 CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(draws==2);CHECK(world._41==3&&world._11==1);CHECK(mesh.flags==2&&object.pMesh==&mesh);
 source.width=128;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));CHECK(logs.find("dimensions differ")!=std::string::npos);source.width=256;
 CHECK(!captured.empty());CHECK(captured[0].x==.5f&&captured[0].y==0);
 Mesh cropped=mesh;MeshVertexF cropverts[3];memcpy(cropverts,verts,sizeof verts);
 for(auto &v:cropverts)v.textureCoordinates[0][0]=v.textureCoordinates[0][0]*.5f;
 cropped.pVertexData=(DWORD*)cropverts;object.pMesh=&cropped;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));object.pMesh=&mesh;CHECK(logs.find("cropped, nonplanar or invalid")!=std::string::npos);
 strcpy(source.name,"spc_mn9a.dds");draws=0;CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(draws==1&&world._41==99);auto first=bound;
 for(int i=0;i<10;i++)CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(bound==first);
 for(int i=0;i<5;i++)SceneryPack_Advance(65536);CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(bound!=first);
 std::ifstream manifest(base+"/manifest.tsv");std::string line;unsigned entries=0;
 while(std::getline(manifest,line)){SceneryFormat::Entry e;if(!SceneryFormat::ParseEntry(line,e))continue;
  SceneryPack_RegisterArchive(&archive,e.archive.c_str());strcpy(source.name,(e.source+".dds").c_str());
  CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);entries++;}
 printf("loaded/drawn%u actualmanifestentries\n",entries);
 SceneryPack_Clear();CHECK(!SceneryPack_Draw(&object,(float*)&billboard));Gfx_Shutdown();puts("actual pack runtime draw, matrix, frames, clear: pass");
}
