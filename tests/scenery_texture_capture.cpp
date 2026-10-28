// Exercise actual archive texture-load callsite, not a manufactured pArchive.
#include "port/types.h"
#include "Texture.h"
#include "Graphics.h"
#include "ScenerySHA256.h"
#include <cstdio>
#include <cstring>
static Texture texture;static unsigned char raw[]={1,2,3,4,5};static bool captured;
extern "C" void *__wrap__ZN18CGenericFileLoader17FindFileInArchiveEP11GenericFilePcPj(GenericFile*,char*,DWORD *size){*size=sizeof raw;return raw;}
extern "C" Texture *__wrap__ZN9CGraphics26CreateTextureFromImageDataEPcPvj(char *name,void*,unsigned){snprintf(texture.name,sizeof texture.name,"%s",name);return &texture;}
extern "C" void __wrap__Z26SceneryPack_RegisterSourceP7TexturePKvj(Texture *t,const void *data,unsigned size){captured=t==&texture&&size==sizeof raw&&!memcmp(data,raw,size);}
extern "C" Texture *__wrap__ZN9CGraphics21LoadDDSThenTGATextureEPcj(char*,unsigned){return nullptr;}
int main(){GenericFile archive={};archive.didFileLoad=TRUE;char name[]="Textures\\spc_mn9a.dds";bool loaded=false;DWORD size=0;auto t=CTexture::FindLoadTexture(&archive,name,&loaded,&size,false,0);if(!t||!captured||!loaded||size!=sizeof raw||t->pArchive!=&archive)return 1;puts("actual archive texture load captures original bytes and size for identity routing: pass");return 0;}
