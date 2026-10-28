#ifndef OPENCMR2_SCENERY_PACK_H
#define OPENCMR2_SCENERY_PACK_H
struct StageObject;
struct GenericFile;
// Visual-only overrides. Original meshes/collision records remain untouched.
void SceneryPack_RegisterArchive(GenericFile *archive, const char *path);
bool SceneryPack_Draw(StageObject *object, const float *billboardMatrix);
void SceneryPack_Clear();
void SceneryPack_Advance(int step16_16);
#endif
