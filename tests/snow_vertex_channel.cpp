// Snow material renderer-parity contract: game/Mesh_RefreshVertices copies
// MeshVertexF records (0x30 bytes) verbatim into a locked GfxVertexBuffer,
// and both renderer backends shade the same GfxVertex.diffuse channel with
// the same fixed3d shader source. Pin the layout equality the memcpy relies
// on, and check through gfx_core that bytes written as a MeshVertexF land in
// GfxVertex.diffuse for any executor to consume.
#include "render/gfx_internal.h"
#include "MeshVertex.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static_assert(sizeof(MeshVertexF) == sizeof(GfxVertex), "MeshVertexF and GfxVertex must stay layout-identical");
static_assert(offsetof(MeshVertexF, diffuseColour) == offsetof(GfxVertex, diffuse), "diffuse channel offset must match");
static_assert(offsetof(MeshVertexF, specularColour) == offsetof(GfxVertex, specular), "specular channel offset must match");

extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
int Platform_GetSettingInt(const char *, int value) { return value; }
const char *Platform_GetSetting(const char *, const char *value) { return value; }
extern "C" void Sys_Log(const char *, ...) {}
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 640; *h = 480; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
extern "C" double Sys_GetTicksPrecise(void) { return 0; }

#ifdef _WIN32
static bool headlessSet = (_putenv_s("OPENCMR2_HEADLESS", "1") == 0);
#else
static bool headlessSet = (setenv("OPENCMR2_HEADLESS", "1", 1) == 0);
#endif

GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }

static void check(bool ok, const char *msg) {
    if (!ok) { fprintf(stderr, "snow_vertex_channel: %s\n", msg); exit(1); }
}

int main(void)
{
    (void)headlessSet;
    check(Gfx_SetVideoMode(640, 480, FALSE) != 0, "video mode");

    GfxVertexBuffer *vb = Gfx_CreateVertexBuffer(3);
    check(vb != NULL, "vertex buffer");

    // Write two vertices as MeshVertexF bytes, the way Mesh_RefreshVertices does.
    MeshVertexF src[2];
    memset(src, 0, sizeof src);
    src[0].diffuseColour = 0xff102030;
    src[0].specularColour = 0xff000000;
    src[1].diffuseColour = 0xffa0b0c0;   // the lifted snow colour
    src[1].specularColour = 0xff000000;
    GfxVertex *lock = Gfx_LockVertexBuffer(vb);
    check(lock != NULL, "lock");
    memcpy(lock, src, sizeof src);       // 0x30-stride verbatim copy
    Gfx_UnlockVertexBuffer(vb);

    check(vb->vertices[0].diffuse == 0xff102030, "first vertex diffuse must land unchanged");
    check(vb->vertices[1].diffuse == 0xffa0b0c0, "lifted vertex diffuse must land unchanged");
    check(vb->vertices[1].specular == 0xff000000, "specular channel must land unchanged");

    Gfx_DestroyVertexBuffer(vb);
    printf("snow_vertex_channel: ok\n");
    return 0;
}
