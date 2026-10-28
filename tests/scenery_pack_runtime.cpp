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
#include "SceneryPackFormat.h"
#include <fstream>
#include <sstream>
#include "Sector.h"
#include "MeshVertex.h"
#include "port/sys.h"
#include "render/gfx_internal.h"
static bool legacy=false;static std::string base; static unsigned draws; static GfxMatrix world;static GfxTexture *bound;static std::vector<GfxVertex> captured;
D3DTextureManager* CGraphics::m_pTextureManager=nullptr;
unsigned int CGraphics::m_textureCount=0;
struct SysFile { FILE *file; };
extern "C" SysFile *Sys_OpenFile(const char *p,int){FILE *f=fopen(p,"rb");
 if(f&&legacy&&std::string(p).find("tree.obj")!=std::string::npos){
  FILE *copy=tmpfile();char line[256];fgets(line,sizeof line,f);int ch;while((ch=fgetc(f))!=EOF)fputc(ch,copy);fclose(f);rewind(copy);f=copy;
 }return f?new SysFile{f}:nullptr;}
extern "C" void Sys_CloseFile(SysFile *f){fclose(f->file);delete f;}
extern "C" DWORD Sys_GetFileSize(SysFile *f){fseek(f->file,0,SEEK_END);long n=ftell(f->file);rewind(f->file);return n;}
extern "C" DWORD Sys_ReadFile(SysFile *f,void *b,DWORD n){return fread(b,1,n,f->file);}
extern "C" int Sys_GetOption(const char*,int f){return f;}
extern "C" const char *Sys_GetOptionText(const char*,const char*){return base.c_str();}
extern "C" void Sys_Log(const char*,...){}
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
int main(int argc,char **argv){if(argc<2)return 0;base=argv[1];legacy=argc>2;CHECK(Gfx_SetVideoMode(640,480,FALSE));
 D3DTextureManager manager={};CGraphics::m_pTextureManager=&manager;CGraphics::m_textureCount=1;
 Texture source={};source.width=source.height=256;strcpy(source.name,"C:\\textures\\TRE_MD1.TGA");GenericFile archive={};source.pArchive=&archive;manager.textureBuffer[0]=&source;
 MeshTriangle triangle={};MeshVertexF verts[3]={};verts[1].y=2;verts[2].x=1;verts[0].textureCoordinates[0][1]=1;verts[2].textureCoordinates[0][0]=1;verts[2].textureCoordinates[0][1]=1;Mesh mesh={};mesh.flags=2;mesh.pTriangles=&triangle;mesh.triangleCount=1;mesh.pVertexData=(DWORD*)verts;mesh.field_0x10=3;
 StageObject object={};object.pMesh=&mesh;object.matrix.right.x=object.matrix.up.y=object.matrix.forward.z=65536;object.matrix.position.x=3*65536;
 GfxMatrix billboard={};billboard._11=billboard._22=billboard._33=billboard._44=1;billboard._41=99;
 CHECK(!SceneryPack_Draw(&object,(float*)&billboard));SceneryPack_RegisterArchive(&archive,"DATA/FRAHIT.BFL");
 if(legacy){CHECK(!SceneryPack_Draw(&object,(float*)&billboard));strcpy(source.name,"spc_mn9a.dds");CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(draws==1);SceneryPack_Clear();Gfx_Shutdown();puts("headerless legacy tree rejected; spectator preserved: pass");return 0;}
 CHECK(SceneryPack_Draw(&object,(float*)&billboard));Gfx_Present(FALSE);CHECK(draws==2);CHECK(world._41==3&&world._11==1);CHECK(mesh.flags==2&&object.pMesh==&mesh);
 source.width=128;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));source.width=256;
 CHECK(!captured.empty());CHECK(captured[0].x==.5f&&captured[0].y==0);
 Mesh cropped=mesh;MeshVertexF cropverts[3];memcpy(cropverts,verts,sizeof verts);
 for(auto &v:cropverts)v.textureCoordinates[0][0]=v.textureCoordinates[0][0]*.5f;
 cropped.pVertexData=(DWORD*)cropverts;object.pMesh=&cropped;CHECK(!SceneryPack_Draw(&object,(float*)&billboard));object.pMesh=&mesh;
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
