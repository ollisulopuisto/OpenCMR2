#include "port/types.h"
#include "Menu.h"
#include "Frontend.h"
#include "Graphics.h"
#include "Font.h"
#include "Sprite.h"
#include "SettingsPage.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
extern void FrontendMenu_BuildAdvancedGraphicsOptions();
extern void FrontendMenu_DrawAdvancedGraphicsOptions(Menu*);
extern Menu g_menu0x0081cd78;
extern "C" void __wrap__Z21FrontendDraw_PlayTimev(){}
extern "C" int __wrap__Z21FrontendDraw_MenuPathP4MenuiiciPPci(Menu*,int,int,char,int,char**,int){return 0;}
extern "C" void __wrap__Z21FrontendDraw_CarouselP4MenucPc(Menu*,char,char*){}
static FILE *out;static int fail=0;static int height=480;
extern "C" void __real__Z13Font_DrawTexthPcssPij(BYTE,char*,short,short,int*,unsigned);
extern "C" void __wrap__Z13Font_DrawTexthPcssPij(BYTE slot,char *text,short x,short y,int *c,unsigned f){
 if(y<80*height/480 || y>375*height/480)++fail;
 if(out)fprintf(out,"%d\t%d\t%s\n",x,y,text);
 __real__Z13Font_DrawTexthPcssPij(slot,text,x,y,c,f);
}
int main(int argc,char **argv){
 Graphics gfx={};g_pGraphics=&gfx;gfx.resX=640;gfx.resY=480;
 Texture arrow={};arrow.width=8;arrow.height=8;
 CFrontend::m_pAr640ATexture=CFrontend::m_pAr640DTexture=&arrow;
 char *texts[512];for(auto &p:texts)p=(char*)"Test";
 texts[0x1b8]=(char*)"Effect Detail";texts[0x1b7]=(char*)"Reflections";texts[0x1b6]=(char*)"High Textures";
 texts[0x1b9]=(char*)"High";texts[0x1ba]=(char*)"Low";texts[0x1bb]=(char*)"Off";
 texts[0x1b3]=(char*)"On";texts[0x1b2]=(char*)"Off";texts[0x133]=(char*)"Off";texts[0x134]=(char*)"On";texts[0x67]=(char*)"OK";
 CFrontend::m_textStrings=texts;CFrontend::m_textCount=512;CFrontend::m_textFirstId=0;
 FontHeader h={};h.glyphCount=1;h.lineHeight=16;h.spaceWidth=4;h.charSpacing=1;
 FontGlyph glyph={};glyph.width=7;glyph.height=16;glyph.field_0xa[0]=8;
 short map[256];for(auto &v:map)v=0;
 FontSlot fonts[4]={};for(auto &f:fonts){f.loaded=1;f.pHeader=&h;f.pGlyphs=&glyph;f.pCharMap=map;}
 g_fonts=fonts;g_fontCount=4;
 FrontendMenu_BuildAdvancedGraphicsOptions();Menu *m=&g_menu0x0081cd78;
 // Test shared keyboard/controller directional input path, all rows and wrap.
 m->pfnCallback1=nullptr;Menu_SetInputStateFlag(1);
 for(int i=0;i<m->itemCount;++i){if(m->cursor!=i)++fail;Menu_Update(m,8);Menu_Update(m,0);}
 if(m->cursor!=0)++fail;Menu_Update(m,4);Menu_Update(m,0);if(m->cursor!=m->itemCount-1)++fail;
 MenuItem *d=Menu_GetItem(m,0x17);m->cursor=(char)(d-m->items);d->max=40;
 Menu_Update(m,2);Menu_Update(m,0);if(d->max!=41)++fail;Menu_Update(m,1);Menu_Update(m,0);if(d->max!=40)++fail;
 for(int page=0;page<3;++page){m->cursor=page*7;g_spriteCount1=0;
   if(argc>1){char name[256];snprintf(name,sizeof(name),"%s-%d.tsv",argv[1],page);out=fopen(name,"w");}
   FrontendMenu_DrawAdvancedGraphicsOptions(m);if(out){fclose(out);out=nullptr;}
   for(unsigned i=0;i<g_spriteCount1;++i){auto r=g_spriteLayer1[i].dst;if(r.y<80||r.y+r.h>=384||r.x+r.w>640)++fail;}
   if(g_spriteCount1>900)++fail;
 }
 // Same bounded layout at common render heights; no display-model assumption.
 for(int res: {720,1080,1664}){
   height=res;gfx.resY=res;gfx.resX=res*4/3;
   for(int row=0;row<m->itemCount;++row){m->cursor=row;g_spriteCount1=0;FrontendMenu_DrawAdvancedGraphicsOptions(m);
     for(unsigned i=0;i<g_spriteCount1;++i){auto r=g_spriteLayer1[i].dst;if(r.y<80*res/480 || r.y+r.h>=384*res/480)++fail;}}
 }
 if(SettingsPage_ClampPercent(-1)!=0||SettingsPage_ClampPercent(300)!=100)++fail;
 printf("advanced menu: %s, %d rows, three bounded pages, navigation and dirt increments\n",fail?"FAIL":"pass",m->itemCount);return fail?1:0;
}
