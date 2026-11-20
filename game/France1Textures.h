#ifndef OPENCMR2_FRANCE1_TEXTURES_H
#define OPENCMR2_FRANCE1_TEXTURES_H
struct GenericFile;struct Texture;
void France1Textures_Begin(GenericFile *archive,const char *stage);
void France1Textures_Clear();
bool France1Textures_IsVerifiedCutout(Texture*);
bool France1Textures_Apply(Texture *texture,const void *bytes,unsigned size);
// 64x32 CPU alpha grid of a verified cutout sheet (bsh_sh1*, tresh*), sampled
// from the same hash-verified RGBA the GPU upload used. False when not cached.
bool France1Textures_CutoutAlpha(Texture*,const unsigned char**,int*,int*);

#endif
