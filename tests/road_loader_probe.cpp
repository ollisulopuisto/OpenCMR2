// Source-only private artifact probe. Actual native relocation/parts/bounds/free;
// GPU upload and archive texture loading are intentionally stubbed.
// Pass an independently hash-verified original or derived C3D path.
#include "port/types.h"
#include "Sector.h"
#include "Mesh.h"
#define private public
#include "Graphics.h"
#undef private
#include "FileBuffer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
extern void *Sector_RelocateStageMeshFile(BYTE*,SceneNode*,GenericFile*);
extern BYTE Sector_FreeMeshPartLists();
extern void Sector_ComputeBounds();
extern Sector *g_sectors[];extern int g_sectorCount,g_sectorHalfSize;
static unsigned uploads,textures,vertices,triangles;static bool valid=true;
extern "C" void __wrap__Z30Graphics_LoadTextureRecordListPhPcjiP11GenericFile(BYTE*,char*,unsigned count,int,GenericFile*){textures=count;}
extern "C" void __wrap__Z19Mesh_UploadVerticesP4Mesh(Mesh *m){
 uploads++;vertices+=m->field_0x10;triangles+=m->triangleCount;
 if(m->field_0x10<=0||m->field_0x10>=2000||m->triangleCount<=0)valid=false;
 for(int i=0;i<m->triangleCount;i++)for(int k=0;k<3;k++)if(m->pTriangles[i].vertexIndex[k]>=m->field_0x10)valid=false;
}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 std::ifstream f(argv[1],std::ios::binary);std::vector<BYTE> raw((std::istreambuf_iterator<char>(f)),{});if(raw.empty())return 3;
 for(int pass=0;pass<3;pass++){
  BYTE *buffer=(BYTE*)CFileBuffer::AllocateLockedBuffer(raw.size());memcpy(buffer,raw.data(),raw.size());uploads=textures=vertices=triangles=0;
  CGraphics::m_textureCount=7;Sector_RelocateStageMeshFile(buffer,nullptr,nullptr);
  if(g_sectorCount!=800||uploads!=174||textures!=56||!valid)return 4;
  unsigned partIndices=0;for(int i=0;i<g_sectorCount;i++)if(auto m=g_sectors[i]->pMesh){
   if(m->partCount>50)return 5;
   for(int t=0;t<m->triangleCount;t++)for(int k=0;k<10;k++)if(m->pTriangles[t].textures[k]!=-1&&m->pTriangles[t].textures[k]<7)return 10;
   for(int p=0;p<m->partCount;p++){
    auto part=m->pParts[p];if(!part||part->maxIndex>=m->field_0x10||part->minIndex<0)return 6;
    for(int k=0;k<part->indexCount;k++)if(part->pData[k]+part->minIndex>part->maxIndex)return 7;
    partIndices+=part->indexCount;
   }
  }
  g_sectorHalfSize=0x100000;Sector_ComputeBounds();
  if(partIndices!=triangles*3)return 8;
  printf("pass%d: meshes=%u vertices=%u triangles=%u parts_indices=%u textures=%u native_relocation_parts_bounds=pass upload_and_texture_loading=stubbed\n",pass,uploads,vertices,triangles,partIndices,textures);
  Sector_FreeMeshPartLists();CFileBuffer::FreeGenericFileBuffer(buffer);if(g_sectorCount!=0)return 9;
 }
}
