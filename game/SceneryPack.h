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
void SceneryPack_LogStatus();
#endif
