#ifndef OPENCMR2_SCENERY_PACK_H
#define OPENCMR2_SCENERY_PACK_H
struct StageObject;
struct GenericFile;
struct Texture;
void SceneryPack_RegisterSource(Texture *texture, const void *bytes, unsigned size);
// Visual-only overrides. Original meshes/collision records remain untouched.
void SceneryPack_RegisterArchive(GenericFile *archive, const char *path);
bool SceneryPack_Draw(StageObject *object, const float *billboardMatrix);
void SceneryPack_Clear();
void SceneryPack_Advance(int step16_16);
// Player car state for reactive spectators (16.16 world position and speed).
void SceneryPack_SetReactionSource(int x16_16,int z16_16,int speed16_16,bool valid);
void SceneryPack_LogStatus();
// Last actual spectator frame draw, not a promise that a mapping is active.
bool SceneryPack_GetAnimationStatus(unsigned long long *tick,unsigned *frame,unsigned *changes);
void SceneryPack_GetTreeStatus(unsigned *replacements,unsigned *wind,unsigned *originalSources);
#endif
