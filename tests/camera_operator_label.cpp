#include "port/types.h"
#include "Font.h"
#include "Sprite.h"
#include "CameraOperator.h"
#include <cstdio>
#include <cstring>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char**argv){
 // A deliberately lowercase-only fixture reproduces the real draw function's
 // missing-glyph behavior. This is NOT the original/native font artwork.
 FontHeader header={};header.glyphCount=1;header.lineHeight=18;header.spaceWidth=5;header.charSpacing=1;
 FontGlyph glyph={};glyph.width=8;glyph.height=18;
 short map[256];for(auto&v:map)v=-1;for(int c='a';c<='z';c++)map[c]=0;map[':']=0;
 Texture texture={};FontSlot fonts[2]={};for(auto&f:fonts){f.loaded=1;f.pHeader=&header;f.pGlyphs=&glyph;f.pCharMap=map;f.pTexture=&texture;}
 Font_SetBlendMode(1);g_fonts=fonts;g_fontCount=2;int color[2]={-1,-1};g_spriteCount1=0;
 Font_DrawText(1,(char*)"Drone: On",12,12,color,0);
 unsigned broken=g_spriteCount1;g_spriteCount1=0;
 Font_DrawText(1,(char*)CameraOperator::Label(true),12,12,color,0);
 CHECK(g_spriteCount1==broken+2);CHECK(g_spriteCount1==8);
 for(const char*p=CameraOperator::Label(false);*p;p++)CHECK(*p==' '||map[(unsigned char)*p]>=0);
 if(argc>1){FILE*f=fopen(argv[1],"w");CHECK(f);for(unsigned i=0;i<g_spriteCount1;i++){auto&r=g_spriteLayer1[i].dst;fprintf(f,"%d %d %d %d\n",r.x,r.y,r.w,r.h);}fclose(f);}
 puts("operator label lowercase-only glyph regression passes; native atlas unavailable");return 0;
}
