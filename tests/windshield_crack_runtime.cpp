#include "port/types.h"
#include "WindshieldCrackPane.h"
#include "render/gfx_internal.h"
#include <cstdio>
#include <cstring>
#include <limits>
extern "C" int Sys_GetOption(const char*,int f){return f;}
extern "C" void Sys_Log(const char*,...){}
extern "C" void Sys_GetDesktopSize(int*w,int*h){*w=96;*h=64;}
extern "C" double Sys_GetTicksPrecise(){return 0;}
int Platform_GetSettingInt(const char*,int f){return f;}
const char*Platform_GetSetting(const char*,const char*f){return f;}
void TouchOverlay_RecordFrame(int,int,bool){}
extern "C" void Input_SetTouchPicture(int,int,int){}
static std::vector<GfxDrawState>states;static std::vector<GfxVertex>vertices;
struct Capture:GfxExecutor{
 bool SetVideoMode(int,int,bool)override{return true;}void SetFullscreen(bool)override{}
 void DestroyTexture(GfxTexture*)override{}void ReadTexture(GfxTexture*)override{}
 void Execute(GfxFrame&f,std::vector<GfxTexture*>&,GfxReadback&,bool,bool,bool)override{for(auto&c:f.commands)if(c.type==GFX_CMD_DRAW){states.push_back(f.states[c.state]);const auto*v=(const GfxVertex*)(f.vertices.data()+c.firstVertexByte);vertices.insert(vertices.end(),v,v+c.vertexCount);}}
};
bool Gfx_GpuIsAvailable(){return true;}
GfxExecutor*Gfx_CreateGpuExecutor(){return new Capture;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CHECK(Gfx_SetVideoMode(96,64,FALSE));GfxTexture*damaged=Gfx_CreateTexture(2,2,GFX_FORMAT_BGRA8,1,0),*broken=Gfx_CreateTexture(2,2,GFX_FORMAT_BGRA8,1,0);
 MeshVertexF v[3]={};v[1].x=1;v[2].y=1;for(auto&q:v){q.z=1;q.nz=-1;q.textureCoordinates[0][0]=.25f;q.textureCoordinates[0][1]=.75f;}
 MeshTriangle tri={};tri.flags=0x400;tri.vertexIndex[1]=1;tri.vertexIndex[2]=2;tri.textures[3]=3;tri.textures[5]=5;
 Mesh mesh={};mesh.pVertexData=(DWORD*)v;mesh.field_0x10=3;mesh.pTriangles=&tri;mesh.triangleCount=1;
 const MeshTriangle original=tri;MeshVertexF saved[3];memcpy(saved,v,sizeof v);
 GfxMatrix world={};world._11=world._22=world._33=world._44=1;world._41=2;
 auto lookup=[&](int t){return t==3?damaged:t==5?broken:nullptr;};
 // Replay rewind/camera switch: old cached state may still say broken, but the
 // exterior selector is now intact. Production helper must submit no pane.
 CHECK(WindshieldCrackPane::Draw(&mesh,0x100,world,lookup)==0);Gfx_Present(FALSE);CHECK(states.empty());
 tri.field_0x2c=3;
 // Deliberately poison every inherited state the old overlay left uncontrolled.
 Gfx_SetStageTexCoordIndex(0,GFX_TCI_CAMERASPACEREFLECTIONVECTOR|1);Gfx_SetStageTextureTransform(0,GFX_TTFF_COUNT3);
 Gfx_SetStageAddress(0,GFX_ADDRESS_WRAP);Gfx_SetAlphaTest(TRUE);Gfx_SetAlphaRef(254);Gfx_SetFog(TRUE);Gfx_SetBodyDirt(1,1,0,0);Gfx_SetDistanceFade(1,2);
 CHECK(WindshieldCrackPane::Draw(&mesh,0x100,world,lookup)==1);
 GfxVertex probe[3]={};Gfx_DrawPrimitive(GFX_TRIANGLELIST,probe,3);Gfx_Present(FALSE);CHECK(states.size()==2);
 CHECK(vertices.size()==6&&vertices[0].u0==.25f&&vertices[0].v0==.75f&&vertices[1].x==1);
 const auto&s=states[0];CHECK(s.stages[0].texture==damaged&&s.stages[0].texCoordIndex==0&&s.stages[0].textureTransform==GFX_TTFF_DISABLE&&s.stages[0].address==GFX_ADDRESS_CLAMP);
 CHECK(!s.alphaTest&&!s.fog&&!s.depthWrite&&s.depthTest&&s.alphaBlend&&s.world._41==2&&s.bodyDirt[0]==0&&s.fadeEnd==0);
 CHECK(states[1].stages[0].texCoordIndex==(GFX_TCI_CAMERASPACEREFLECTIONVECTOR|1)&&states[1].alphaTest&&states[1].fog&&states[1].bodyDirt[0]==1); // restored
 states.clear();tri.field_0x2c=5;CHECK(WindshieldCrackPane::Draw(&mesh,0x100,world,lookup)==1);Gfx_Present(FALSE);CHECK(states[0].stages[0].texture==broken);
 states.clear();tri.field_0x2c=0;CHECK(!WindshieldCrackPane::Draw(&mesh,0x100,world,lookup));tri.field_0x2c=10;CHECK(!WindshieldCrackPane::Draw(&mesh,0x100,world,lookup));
 tri.field_0x2c=3;tri.flags=0x20;CHECK(!WindshieldCrackPane::Draw(&mesh,0x100,world,lookup));tri.flags=0x8500;CHECK(WindshieldCrackPane::Draw(&mesh,0x100,world,lookup)==1); // outer fallback
 tri.vertexIndex[2]=99;CHECK(!WindshieldCrackPane::Draw(&mesh,0x100,world,lookup));tri=original;tri.field_0x2c=3;
 v[0].x=std::numeric_limits<float>::infinity();CHECK(!WindshieldCrackPane::Draw(&mesh,0x100,world,lookup));memcpy(v,saved,sizeof v);tri=original;
 CHECK(!memcmp(v,saved,sizeof v));CHECK(!memcmp(&tri,&original,sizeof tri));
 // Side windows are separate groups, not exclusive states of one pane.
 MeshTriangle sides[2]={};for(auto&t:sides){t.vertexIndex[1]=1;t.vertexIndex[2]=2;t.textures[3]=3;t.textures[5]=5;}
 sides[0].flags=0x40;sides[0].field_0x2c=3;sides[1].flags=0x200;sides[1].field_0x2c=5;mesh.pTriangles=sides;mesh.triangleCount=2;
 states.clear();Gfx_Present(FALSE);states.clear();
 CHECK(WindshieldCrackPane::Draw(&mesh,0x40,world,lookup)==1);
 CHECK(WindshieldCrackPane::Draw(&mesh,0x80,world,lookup)==1);Gfx_Present(FALSE);
 CHECK(states.size()==2&&states[0].stages[0].texture==damaged&&states[1].stages[0].texture==broken);
 world._41=std::numeric_limits<float>::quiet_NaN();CHECK(!WindshieldCrackPane::Draw(&mesh,0x40,world,lookup));
 Gfx_Shutdown();puts("live exterior damage selectors, UV/material isolation and state restore pass; capture only, not Metal pixels");
}
