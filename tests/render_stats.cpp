// OpenCMR2: renderer frame statistics (draw calls, vertices, triangles) for
// the in-game debug overlay. Links gfx_core.cpp directly with stubbed platform
// calls, like tests/movie_render_state.cpp.
#include "render/gfx_internal.h"
#include <cstdio>
#include <cstring>

extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
int Platform_GetSettingInt(const char *, int value) { return value; }
const char *Platform_GetSetting(const char *, const char *value) { return value; }
extern "C" void Sys_Log(const char *, ...) {}
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 640; *h = 480; }
extern "C" unsigned int Sys_GetTicks() { return 0; }

// Force the null (headless) backend: no GPU executor to link/stub.
static bool headlessSet = (setenv("OPENCMR2_HEADLESS", "1", 1) == 0);

// The GPU backend is compiled into Gfx_SetVideoMode but never selected with the
// headless variable above, so stub it out to keep the link minimal.
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }

int main(void)
{
    (void)headlessSet;
    if (!Gfx_SetVideoMode(640, 480, FALSE)) {
        puts("FAIL: video mode");
        return 1;
    }

    GfxTLVertex v[3] = {};
    WORD idx[3] = { 0, 1, 2 };

    // The runtime filtering setter must survive the null executor and leave
    // the frame flow intact.
    Gfx_SetAnisotropy(8);

    // One non-indexed triangle list (3 verts, 1 tri) and one indexed
    // triangle list (3 verts, 3 indices, 1 tri).
    Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, v, 3);
    Gfx_DrawIndexedPrimitiveTL(GFX_TRIANGLELIST, v, 3, idx, 3);
    Gfx_Present(FALSE);

    int failures = 0;
    unsigned int draws = Gfx_GetFrameDrawCalls();
    unsigned int verts = Gfx_GetFrameVertices();
    unsigned int tris = Gfx_GetFrameTriangles();

    if (draws != 2) { printf("FAIL: draw calls = %u, want 2\n", draws); failures++; }
    if (verts != 6) { printf("FAIL: vertices = %u, want 6\n", verts); failures++; }
    if (tris != 2) { printf("FAIL: triangles = %u, want 2\n", tris); failures++; }

    // Stats must not accumulate across frames: present an empty frame and the
    // last-frame stats should reset to zero.
    Gfx_Present(FALSE);
    if (Gfx_GetFrameDrawCalls() != 0 || Gfx_GetFrameVertices() != 0 || Gfx_GetFrameTriangles() != 0) {
        puts("FAIL: stats not reset between frames");
        failures++;
    }

    Gfx_Shutdown();
    if (failures == 0) {
        puts("render stats: draw calls, vertices and triangles tracked per frame");
        return 0;
    }
    return 1;
}
