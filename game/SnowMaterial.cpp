#include "port/types.h"
#include "SnowMaterial.h"
#include "SnowGlint.h"
#include "LightField.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "SceneNode.h"
#include "FixedPoint.h"
#include "TrackCollisionData.h"
#include "Car.h"
#include "Game.h"
#include "port/sys.h"
#include <cmath>
#include <stdlib.h>
#include <vector>

// Collision query (game/TrackCollision.cpp) and the stage light state
// (game/StageObjects.cpp) the merged snow glints already use.
int Track_GetGroundHeight(FixVector *pPoint, FixVector *pNormal, short *pTri, unsigned short *pSurface, int defaultY);
void StageObject_GetRotatedStageLightVector(FixVector *pOut);
int StageObject_GetWeatherLightIntensity(void);
extern TrackTriangle *g_trackTriangles;

// Peak per-channel lift at amount 100: a visible brightening that stays below
// full white so the surface keeps its texture.
#define SNOW_MATERIAL_LIFT_MAX 90

namespace {

struct SnowMaterialVertex {
    unsigned short index;       // vertex in the mesh
    unsigned short pad;
    FixVector world;            // world position (16.16), terrain is static
    float nx, ny, nz;           // averaged snow face normal, world space
};

struct SnowMaterialMesh {
    Mesh *mesh;
    DWORD *originalDiffuse;     // one ARGB per vertex, snapshot at build
    std::vector<SnowMaterialVertex> tagged;
};

SceneNode *g_snowScene = NULL;
std::vector<SnowMaterialMesh> g_snowMeshes;
int g_snowBuilt = 0;
int g_snowApplied = 0;
int g_snowTaggedTotal = 0;

void SnowMaterial_TagMesh(SceneNode *pNode, Mesh *pMesh)
{
    int vertexCount = pMesh->field_0x10;
    int triCount = pMesh->triangleCount;
    if (vertexCount <= 0 || triCount <= 0 || pMesh->pVertexData == NULL || pMesh->pTriangles == NULL)
        return;
    if (*(unsigned int *)pMesh->field_0x108 == 0xffffffff)
        return;   // scene shadow mesh, not visible terrain

    MeshVertexF *verts = (MeshVertexF *)pMesh->pVertexData;
    std::vector<unsigned short> triIndex(triCount * 3);
    std::vector<unsigned char> triSnow(triCount);
    std::vector<unsigned char> vertexFlags(vertexCount, 0);
    std::vector<float> normalAccum(vertexCount * 3, 0.f);
    int snowTris = 0;

    for (int t = 0; t < triCount; t++) {
        MeshTriangle *tri = &pMesh->pTriangles[t];
        FixVector w[3];
        for (int i = 0; i < 3; i++) {
            unsigned short v = tri->vertexIndex[i];
            triIndex[t * 3 + i] = v;
            FixVector local;
            if (v >= vertexCount) { local.x = local.y = local.z = 0; }
            else {
                local.x = (int)(verts[v].x * 65536.f);
                local.y = (int)(verts[v].y * 65536.f);
                local.z = (int)(verts[v].z * 65536.f);
            }
            FixMatrix_TransformPoint(&w[i], &local, &pNode->world);
        }
        // Geometric face normal in world space: reliable for the up test even
        // where vertex normals are smoothed.
        float e1x = (w[1].x - w[0].x) / 65536.f, e1y = (w[1].y - w[0].y) / 65536.f, e1z = (w[1].z - w[0].z) / 65536.f;
        float e2x = (w[2].x - w[0].x) / 65536.f, e2y = (w[2].y - w[0].y) / 65536.f, e2z = (w[2].z - w[0].z) / 65536.f;
        float fnx = e1y * e2z - e1z * e2y, fny = e1z * e2x - e1x * e2z, fnz = e1x * e2y - e1y * e2x;
        float fl = std::sqrt(fnx * fnx + fny * fny + fnz * fnz);
        float faceNy = fl > 1e-5f ? fny / fl : 0.f;

        FixVector centroid, groundNormal = {0, 0x10000, 0};
        centroid.x = (w[0].x + w[1].x + w[2].x) / 3;
        centroid.y = (w[0].y + w[1].y + w[2].y) / 3;
        centroid.z = (w[0].z + w[1].z + w[2].z) / 3;
        short hitTri = -1;
        unsigned short surface = 0;
        int groundY = Track_GetGroundHeight(&centroid, &groundNormal, &hitTri, &surface, centroid.y);
        int snow = SnowMaterial_AcceptTriangle(hitTri >= 0, surface, centroid.y, groundY, faceNy);
        triSnow[t] = (unsigned char)snow;
        if (snow) {
            snowTris++;
            for (int i = 0; i < 3; i++) {
                unsigned short v = triIndex[t * 3 + i];
                if (v < vertexCount) {
                    normalAccum[v * 3 + 0] += fnx;
                    normalAccum[v * 3 + 1] += fny;
                    normalAccum[v * 3 + 2] += fnz;
                }
            }
        }
    }
    if (snowTris == 0)
        return;

    int tagged = SnowMaterial_AggregateVertexTags(triIndex.data(), triSnow.data(), triCount,
                                                  vertexFlags.data(), vertexCount);
    if (tagged == 0)
        return;

    SnowMaterialMesh rec;
    rec.mesh = pMesh;
    rec.originalDiffuse = (DWORD *)malloc(sizeof(DWORD) * vertexCount);
    if (rec.originalDiffuse == NULL)
        return;
    for (int i = 0; i < vertexCount; i++)
        rec.originalDiffuse[i] = verts[i].diffuseColour;
    for (int i = 0; i < vertexCount; i++) {
        if (!SnowMaterial_VertexIsSnow(vertexFlags[i]))
            continue;
        SnowMaterialVertex sv;
        sv.index = (unsigned short)i;
        sv.pad = 0;
        FixVector local;
        local.x = (int)(verts[i].x * 65536.f);
        local.y = (int)(verts[i].y * 65536.f);
        local.z = (int)(verts[i].z * 65536.f);
        FixMatrix_TransformPoint(&sv.world, &local, &pNode->world);
        float nl = std::sqrt(normalAccum[i * 3 + 0] * normalAccum[i * 3 + 0] +
                             normalAccum[i * 3 + 1] * normalAccum[i * 3 + 1] +
                             normalAccum[i * 3 + 2] * normalAccum[i * 3 + 2]);
        if (nl > 1e-5f) {
            sv.nx = normalAccum[i * 3 + 0] / nl;
            sv.ny = normalAccum[i * 3 + 1] / nl;
            sv.nz = normalAccum[i * 3 + 2] / nl;
        } else {
            sv.nx = 0.f; sv.ny = 1.f; sv.nz = 0.f;
        }
        rec.tagged.push_back(sv);
    }
    g_snowTaggedTotal += (int)rec.tagged.size();
    g_snowMeshes.push_back(rec);
}

void SnowMaterial_Walk(SceneNode *pNode)
{
    for (; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->type == SCENE_NODE_MESH && pNode->pObject != NULL)
            SnowMaterial_TagMesh(pNode, (Mesh *)pNode->pObject);
        if (pNode->pFirstChild != NULL)
            SnowMaterial_Walk(pNode->pFirstChild);
    }
}

void SnowMaterial_Restore(void)
{
    for (size_t m = 0; m < g_snowMeshes.size(); m++) {
        SnowMaterialMesh *rec = &g_snowMeshes[m];
        MeshVertexF *verts = (MeshVertexF *)rec->mesh->pVertexData;
        for (size_t i = 0; i < rec->tagged.size(); i++)
            verts[rec->tagged[i].index].diffuseColour = rec->originalDiffuse[rec->tagged[i].index];
        Mesh_RefreshVertices(rec->mesh);
    }
    g_snowApplied = 0;
}

void SnowMaterial_Free(void)
{
    for (size_t m = 0; m < g_snowMeshes.size(); m++)
        free(g_snowMeshes[m].originalDiffuse);
    g_snowMeshes.clear();
    g_snowTaggedTotal = 0;
    g_snowBuilt = 0;
}

void SnowMaterial_Build(void)
{
    if (g_snowBuilt || g_snowScene == NULL || g_trackTriangles == NULL)
        return;
    g_snowBuilt = 1;
    // Runs after the scene's first world-matrix update, so node->world is valid.
    SnowMaterial_Walk(g_snowScene);
    // Stage teardown runs registered callbacks LIFO; registering here, after
    // the scene's own teardown was registered at load, means our restore runs
    // while the meshes still exist.
    CGame::RegisterCallback((void *)SnowMaterial_Reset, NULL);
    Sys_Log("snow material: %d tagged vertices in %d terrain meshes", g_snowTaggedTotal, (int)g_snowMeshes.size());
}

} // namespace

void SnowMaterial_SetStageScene(SceneNode *pRoot)
{
    if (g_snowBuilt || g_snowApplied)
        SnowMaterial_Reset();
    g_snowScene = pRoot;
}

void SnowMaterial_Reset(void)
{
    if (g_snowApplied)
        SnowMaterial_Restore();
    SnowMaterial_Free();
    g_snowApplied = 0;
    g_snowScene = NULL;
}

void SnowMaterial_GetStats(int *pTaggedVertices, int *pApplied)
{
    if (pTaggedVertices) *pTaggedVertices = g_snowTaggedTotal;
    if (pApplied) *pApplied = g_snowApplied;
}

void SnowMaterial_UpdateFrame(void)
{
    int amount = Sys_GetOption("video.snow_material", 0);
    if (amount <= 0) {
        if (g_snowApplied)
            SnowMaterial_Restore();
        return;
    }
    if (amount > 200) amount = 200;
    if (!g_snowBuilt)
        SnowMaterial_Build();
    if (g_snowMeshes.empty())
        return;

    FixVector sunV;
    StageObject_GetRotatedStageLightVector(&sunV);
    float sunLen = std::sqrt((float)sunV.x * sunV.x + (float)sunV.y * sunV.y + (float)sunV.z * sunV.z);
    if (sunLen < 1e-3f)
        return;
    float sunX = sunV.x / sunLen, sunY = sunV.y / sunLen, sunZ = sunV.z / sunLen;
    int weatherLight = StageObject_GetWeatherLightIntensity();
    float sunLevel = weatherLight <= 0 ? 0.f : weatherLight >= 0x10000 ? 1.f : weatherLight / 65536.f;

    Car *car = Car_Get(0);
    FixVector eye = {0, 0, 0};
    if (g_viewNodes[0]) FixMatrix_GetPosition(&eye, &g_viewNodes[0]->world);
    else if (car) eye = car->position;
    float ex0 = eye.x / 65536.f, ey0 = eye.y / 65536.f, ez0 = eye.z / 65536.f;
    float scale = SNOW_MATERIAL_LIFT_MAX * (amount / 100.f);

    for (size_t m = 0; m < g_snowMeshes.size(); m++) {
        SnowMaterialMesh *rec = &g_snowMeshes[m];
        MeshVertexF *verts = (MeshVertexF *)rec->mesh->pVertexData;
        for (size_t i = 0; i < rec->tagged.size(); i++) {
            SnowMaterialVertex *sv = &rec->tagged[i];
            float wpx = sv->world.x / 65536.f, wpy = sv->world.y / 65536.f, wpz = sv->world.z / 65536.f;
            float vx = ex0 - wpx, vy = ey0 - wpy, vz = ez0 - wpz;
            float vl = std::sqrt(vx * vx + vy * vy + vz * vz);
            if (vl < 1e-3f)
                continue;
            float response = SnowGlint_Specular(sv->nx, sv->ny, sv->nz,
                                                sunX, sunY, sunZ, vx / vl, vy / vl, vz / vl,
                                                SnowGlint_FacetSeed(sv->world.x, sv->world.z)) * sunLevel;
            float boost = 0.f;
            for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX; li++) {
                FogLightSource *L = &g_fogLightSources[li];
                boost += SnowGlint_ConeBoost(wpx, wpy, wpz,
                                             L->pos.x / 65536.f, L->pos.y / 65536.f, L->pos.z / 65536.f,
                                             L->dir.x / 65536.f, L->dir.y / 65536.f, L->dir.z / 65536.f,
                                             L->intensity / 65536.f);
            }
            response += boost * 0.8f;
            if (response > 1.f) response = 1.f;
            int lift = (int)(response * scale);
            DWORD orig = rec->originalDiffuse[sv->index];
            int r = ((orig >> 16) & 0xff) + lift;
            int g = ((orig >> 8) & 0xff) + lift;
            int b = (orig & 0xff) + lift;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            verts[sv->index].diffuseColour = (orig & 0xff000000) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
        }
        Mesh_RefreshVertices(rec->mesh);
    }
    g_snowApplied = 1;
}
