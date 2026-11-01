#ifndef OPENCMR2_FRANCE1_TEXTURES_H
#define OPENCMR2_FRANCE1_TEXTURES_H
struct GenericFile;struct Texture;
void France1Textures_Begin(GenericFile *archive,const char *stage);
void France1Textures_Clear();
bool France1Textures_IsVerifiedCutout(Texture*);
bool France1Textures_Apply(Texture *texture,const void *bytes,unsigned size);
#endif
bool France1Textures_CutoutAlpha(Texture*,const unsigned char**,int*,int*);
