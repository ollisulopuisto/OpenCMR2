#include "port/types.h"
#include "Font.h"
#include "Sprite.h"
#include <cstdio>
#include <cstring>
int main(int argc, char **argv) {
    FontSlot *saved=g_fonts; unsigned savedCount=g_fontCount;
    FontHeader header={}; header.glyphCount=1; header.lineHeight=40;header.spaceWidth=12;header.charSpacing=4;
    FontGlyph glyph={};glyph.width=24;glyph.height=40;
    short map[256];for(auto &v:map)v=0;
    FontSlot font={};font.loaded=1;font.pHeader=&header;font.pGlyphs=&glyph;font.pCharMap=map;
    g_fonts=&font;g_fontCount=1;g_spriteCount1=0;
    BYTE white[4]={255,255,255,255};
    Font_DrawTextScaled(0,"AB CD",12,11,10,30,white);
    int fail=0;
    for(unsigned i=0;i<g_spriteCount1;++i) {
        const auto &r=g_spriteLayer1[i].dst;
        if(r.h>10 || r.y!=11 || r.x<12 || r.x+r.w>42) ++fail;
    }
    for(unsigned i=0;i<g_spriteCount1;++i) {
        const auto &r=g_spriteLayer1[i].dst;
        printf("rect %d %d %d %d\n",r.x,r.y,r.w,r.h);
    }
    if(argc>1) {
        // Rasterize the actual queued glyph rectangles with a synthetic atlas
        // glyph. This verifies spacing/scaling, not proprietary font artwork.
        unsigned char pixels[48*28*3]; memset(pixels,16,sizeof(pixels));
        for(unsigned i=0;i<g_spriteCount1;++i) {
            const auto &r=g_spriteLayer1[i].dst;
            for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x)
                if(x==0||y==0||y==r.h/2||x==r.w-1)
                    for(int c=0;c<3;++c) pixels[((r.y+y)*48+r.x+x)*3+c]=240;
        }
        FILE *out=fopen(argv[1],"wb");if(!out)return 1;
        fprintf(out,"P6\n48 28\n255\n");fwrite(pixels,1,sizeof(pixels),out);fclose(out);
    }
    if(g_spriteCount1!=4) ++fail;
    unsigned count=g_spriteCount1;
    Font_DrawTextScaled(1,"bad slot",0,0,10,30,white);
    Font_DrawTextScaled(0,"zero height",0,0,0,30,white);
    if(g_spriteCount1!=count) ++fail;
    g_fonts=saved;g_fontCount=savedCount;
    printf("diagnostic font: %s, bounded10px glyphs and width clip\n",fail?"FAIL":"pass");return fail?1:0;
}
