// Renders the real Graphics options page (FrontendMenu_BuildGraphicsOptions /
// FrontendMenu_DrawGraphicsOptions) and the real Atmosphere submenu
// (FrontendMenu_GetAtmosphereAmounts / FrontendMenu_DrawAdvancedGraphicsOptions)
// through the shared menu machine, dumping block-glyph pixel frames. Substitute
// glyphs: this proves layout, labels and wiring of the shared menu code, not
// native fonts or native pixels.
#include "port/types.h"
#include "Menu.h"
#include "Frontend.h"
#include "Graphics.h"
#include "Font.h"
#include "Sprite.h"
#include <cstdio>
#include <string>
#include <cstring>
extern void FrontendMenu_BuildGraphicsOptions();
extern void FrontendMenu_DrawGraphicsOptions(Menu*);
extern void FrontendMenu_DrawAdvancedGraphicsOptions(Menu*);
extern Menu g_menu0x008210f8;
extern Menu* FrontendMenu_GetAtmosphereAmounts(Menu*);
extern "C" void __wrap__Z21FrontendDraw_PlayTimev(){}
extern "C" int __wrap__Z21FrontendDraw_MenuPathP4MenuiiciPPci(Menu*,int,int,char,int,char**,int){return 0;}
extern "C" void __wrap__Z21FrontendDraw_CarouselP4MenucPc(Menu*,char,char*){}
static const int W=640,H=480;
static unsigned char fb[H][W][3];
struct DrawnLine{int font,x,y,white;char text[128];};
static DrawnLine drawn[512];static int drawnCount;
static int fail=0;
extern "C" void __real__Z13Font_DrawTexthPcssPij(BYTE,char*,short,short,int*,unsigned);
extern "C" unsigned char g_colourWhite0x00524968[4];
extern "C" void __wrap__Z13Font_DrawTexthPcssPij(BYTE slot,char *text,short x,short y,int *c,unsigned f){
 if(drawnCount<512){auto&l=drawn[drawnCount++];l.font=slot;l.x=x;l.y=y;l.white=c==(int*)g_colourWhite0x00524968;std::strncpy(l.text,text?text:"",127);}
 int cw=6,ch=slot==2?13:10;
 for(int i=0;text&&text[i];i++)for(int gy=0;gy<ch-1;gy++)for(int gx=0;gx<cw-1;gx++){
  int px=x+i*cw+gx,py=y+gy;if(px>=0&&px<W&&py>=0&&py<H){fb[py][px][0]=fb[py][px][1]=fb[py][px][2]=220;}
 }
 __real__Z13Font_DrawTexthPcssPij(slot,text,x,y,c,f);
}
static bool sawText(const char*t){for(int i=0;i<drawnCount;i++)if(std::strcmp(drawn[i].text,t)==0)return true;return false;}
// Amount rows draw as "label < value >": match the label prefix.
static bool sawPrefix(const char*t){size_t n=std::strlen(t);for(int i=0;i<drawnCount;i++)if(std::strncmp(drawn[i].text,t,n)==0)return true;return false;}
int main(int argc,char**argv){
 Graphics gfx={};g_pGraphics=&gfx;gfx.resX=W;gfx.resY=H;
 Texture arrow={};arrow.width=8;arrow.height=8;
 CFrontend::m_pAr640ATexture=CFrontend::m_pAr640DTexture=&arrow;
 char *texts[512];for(auto &p:texts)p=(char*)"Test";
 texts[0x8e]=(char*)"GRAPHICS";texts[0x1ac]=(char*)"Advanced";texts[0x67]=(char*)"OK";
 texts[0x1ce]=(char*)"Driver";texts[0x1cf]=(char*)"Fullscreen";texts[0x1d0]=(char*)"Device";
 texts[0x1d1]=(char*)"Resolution";texts[0x1d2]=(char*)"Filtering";texts[0x1d3]=(char*)"Frames";
 texts[0x133]=(char*)"Off";texts[0x134]=(char*)"On";
 texts[0x1d5]=(char*)"Med";texts[0x1d6]=(char*)"High";
 CFrontend::m_textStrings=texts;CFrontend::m_textCount=512;CFrontend::m_textFirstId=0;
 FontHeader h={};h.glyphCount=1;h.lineHeight=16;h.spaceWidth=4;h.charSpacing=1;
 FontGlyph glyph={};glyph.width=7;glyph.height=16;glyph.field_0xa[0]=8;
 short map[256];for(auto &v:map)v=0;
 FontSlot fonts[4]={};for(auto &f:fonts){f.loaded=1;f.pHeader=&h;f.pGlyphs=&glyph;f.pCharMap=map;}
 g_fonts=fonts;g_fontCount=4;
 FrontendMenu_BuildGraphicsOptions();
 Menu *m=&g_menu0x008210f8;
 // The Atmosphere row exists, carries its literal label and targets the submenu.
 MenuItem *atmoItem=NULL;
 for(int i=0;i<m->itemCount;i++)if(m->items[i].value==0x10)atmoItem=&m->items[i];
 if(!atmoItem||!atmoItem->text||std::strcmp(atmoItem->text,"Atmosphere...")){std::puts("FAIL: Atmosphere row");++fail;}
 // Page 1: Graphics options, cursor on the Atmosphere row.
 if(atmoItem)m->cursor=(char)(atmoItem-m->items);
 drawnCount=0;std::memset(fb,16,sizeof fb);g_spriteCount1=0;
 FrontendMenu_DrawGraphicsOptions(m);
 if(!sawText("Atmosphere...")){std::puts("FAIL: Atmosphere label not drawn on Graphics page");++fail;}
 if(!sawText("OK")){std::puts("FAIL: OK not drawn");++fail;}
 for(int i=0;i<drawnCount;i++)if(drawn[i].y<8||drawn[i].y>=H-8){std::puts("FAIL: text off-frame page1");++fail;break;}
 if(argc>1){FILE*f=std::fopen((std::string(argv[1])+"-graphics.ppm").c_str(),"wb");
  if(f){std::fprintf(f,"P6\n%d %d\n255\n",W,H);std::fwrite(fb,1,sizeof fb,f);std::fclose(f);}
  FILE*l=std::fopen((std::string(argv[1])+"-graphics.lines").c_str(),"w");
  if(l){for(int i=0;i<drawnCount;i++)std::fprintf(l,"%d %d %d %d %s\n",drawn[i].font,drawn[i].x,drawn[i].y,drawn[i].white,drawn[i].text);std::fclose(l);}}
 // Page 2: the opened Atmosphere submenu, cursor on the fog toggle.
 Menu *atmo=FrontendMenu_GetAtmosphereAmounts(m);
 atmo->pfnCallback0(atmo,0);   // EnterAtmosphere: read saved options
 atmo->cursor=0;
 drawnCount=0;std::memset(fb,16,sizeof fb);g_spriteCount1=0;
 FrontendMenu_DrawAdvancedGraphicsOptions(atmo);
 if(!sawPrefix("Fog (0/1)")){std::puts("FAIL: fog toggle row not drawn");++fail;}
 if(!sawPrefix("Fog through windscreen glass (0/1)"))std::puts("note: glass row not present on this branch (ships with the beams PR)");
 if(!sawPrefix("Fog light response % (head/rear lights)")){std::puts("FAIL: light response row not drawn");++fail;}
 for(int i=0;i<drawnCount;i++)if(drawn[i].y<8||drawn[i].y>=H-8){std::puts("FAIL: text off-frame page2");++fail;break;}
 int maxY=0;for(int i=0;i<drawnCount;i++)if(drawn[i].y>maxY)maxY=drawn[i].y;
 if(argc>1){FILE*f=std::fopen((std::string(argv[1])+"-atmosphere.ppm").c_str(),"wb");
  if(f){std::fprintf(f,"P6\n%d %d\n255\n",W,H);std::fwrite(fb,1,sizeof fb,f);std::fclose(f);}
  FILE*l=std::fopen((std::string(argv[1])+"-atmosphere.lines").c_str(),"w");
  if(l){for(int i=0;i<drawnCount;i++)std::fprintf(l,"%d %d %d %d %s\n",drawn[i].font,drawn[i].x,drawn[i].y,drawn[i].white,drawn[i].text);std::fclose(l);}}
 std::printf("graphics menu render: %s, page1 %d lines, page2 %d lines, deepest y %d\n",fail?"FAIL":"pass",0,0,maxY);
 return fail?1:0;
}
