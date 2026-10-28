#include "port/types.h"
#include "GraphicsOptions.h"
#include "Sector.h"
#include "Mesh.h"
#include <cstdio>
int main(){
 int fail=0;StageObject objects[600]={};AnimObject_ClearTable();
 if(AnimObject_Update(nullptr)!=0 || g_animStateCount!=0)++fail;
 for(auto &object:objects)if(AnimObject_Update(&object)!=0)++fail;
 if(g_animStateCount!=600 || !g_animStates)++fail;
 for(int i=0;i<600;++i){auto &s=g_animStates[i];if(s.pObject!=&objects[i] || !s.initialized || s.frameCount!=1 || s.baseTexture!=-1)++fail;}
 Mesh mesh={};MeshPart part={};MeshPart *parts[]={&part};
 part.texture=37;mesh.partCount=1;mesh.pParts[0]=&part;
 StageObject textured={};textured.pMesh=&mesh;
 if(AnimObject_Update(&textured)!=0 || g_animStates[g_animStateCount-1].baseTexture!=37)++fail;
 for(int i=0;i<20;++i)for(auto &object:objects)if(AnimObject_Update(&object)!=0)++fail;
 AnimObject_ClearTable();if(g_animStates || g_animStateCount)++fail;
 if(AnimObject_Update(&objects[0])!=0 || g_animStateCount!=1)++fail;
 int t=0;if(AnimFrame_Advance(0,0,&t)!=0 || AnimFrame_Advance(0,4,nullptr)!=0)++fail;
 AnimObject_ClearTable();printf("animated lifecycle: %s, cold allocation, growth, clear/reload, safe static fallback\n",fail?"FAIL":"pass");return fail?1:0;
}
