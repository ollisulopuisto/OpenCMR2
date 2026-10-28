#include "port/types.h"
#include "DustPlume.h"
#include "Graphics.h"
#include "port/gfx.h"
#include <cstdio>
extern int g_billboardsEnabled,g_billboardsRequested;
extern void Particle_DrawAll(SceneNode*,BYTE);
extern void Particle_ClearActiveFlags();
extern Particle *g_particles;extern ParticleType *g_particleTypes;
extern int g_particleCount,g_particleTypeCount,g_nextParticle;
static int volumeCalls;static bool available=true;
extern "C" BOOL __wrap_Gfx_DrawDustVolume(const GfxDustLobe *lobes,int count){volumeCalls++;return available&&count>0&&lobes[0].centreRadius[0]==10;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){Particle pool[400]={};ParticleType types[33]={};g_particles=pool;g_particleTypes=types;g_particleCount=400;g_particleTypeCount=33;g_nextParticle=0;
 auto &type=types[DUST_PLUME_TYPE];type.flags=1;type.lifetime=DUST_PLUME_LIFE<<16;type.texture=reinterpret_cast<Texture*>(1);
 FixVector point={10<<16,0,20<<16},velocity={0,0,0};BYTE colour[]={120,90,50,255};
 DustPlume_Emit(&point,&velocity,colour,0,1);DustPlume_AdvanceWorld(65536);
 for(auto &p:pool)if(p.active){p.size=65536;p.type0x53=80;p.texture=type.texture;}
 g_billboardsEnabled=0;g_billboardsRequested=0;
 Particle_DrawAll(nullptr,0);CHECK(volumeCalls==1&&g_billboardsRequested==0);
 // An original dry wheel sheet is replaced only when explicitly tagged.
 Particle extra={};extra.active=1;extra.field0x55=1;extra.pType=&types[0x12];
 extra.texture=type.texture;extra.size=65536;extra.type0x53=80;extra.dryWheelDust=true;
 pool[399]=extra;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==2&&g_billboardsRequested==0);
 pool[399].dryWheelDust=false;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==3&&g_billboardsRequested==1);
 pool[399].active=0;g_billboardsRequested=0;
 available=false;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==4&&g_billboardsRequested==1);
 Particle_ClearActiveFlags();available=true;Particle_DrawAll(nullptr,0);CHECK(volumeCalls==4);
 // Recycling an explicitly tagged particle through the original API clears the tag.
 types[0x12].flags=1;types[0x12].lifetime=65536;g_nextParticle=0;
 Particle_SpawnTagged(0x12,&point,&velocity,0,nullptr,colour,0,nullptr,1,true);
 CHECK(pool[0].dryWheelDust);pool[0].active=0;g_nextParticle=0;
 Particle_Spawn(0x12,&point,&velocity,0,nullptr,colour,0,nullptr,1);
 CHECK(!pool[0].dryWheelDust);
 g_particles=nullptr;g_particleTypes=nullptr;g_particleCount=g_particleTypeCount=0;puts("volume replaces sheets; unsupported fallback and stage clear: pass");}
