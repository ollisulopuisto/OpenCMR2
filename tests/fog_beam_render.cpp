// Fog beam wedge rendered-pixel regression (video.fog_lights): the REAL
// production endpoint calculation (game/FogBeamProjection.h) runs over real
// fixed-point matrix/projection primitives (game/FixedMatrixCore.cpp) for a
// beam that crosses the near plane - the 2026-11-01 rear-lamp streak case -
// and the resulting GfxFogLight uniforms drive the REAL fog.frag through
// gfx_core recording -> real GL executor with readback. Linux/software GL is
// NOT native Mac OpenGL/Metal acceptance; the SDL_GPU path shades the same
// fog.frag source (uniform layout pinned by tests/fog_uniform_layout.py).
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/FixedMatrixCore.cpp"
#include "FogBeamProjection.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

static SDL_Window *testWindow;
extern "C" int Sys_GetOption(const char *, int fallback) { return fallback; }
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
const char *Platform_GetSetting(const char *key, const char *fallback) {
    return !strcmp(key, "video.graphics_api") ? "opengl" : fallback;
}
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
GfxExecutor *Gfx_CreateGpuExecutor() { return nullptr; }
bool Gfx_GpuIsAvailable(void) { return false; }
extern "C" unsigned int Sys_GetTicks() { return 0; }
extern "C" double Sys_GetTicksPrecise(void) { return 0; }
extern "C" void Sys_GetDesktopSize(int *w, int *h) { *w = 96; *h = 64; }

// Globals the production path reads.
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
Graphics *g_pGraphics;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;   // definition bypasses access control
const float g_floatHalf = 0.5f;

static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }

static const int W = 96, H = 64;
static GfxVertexBuffer *vb;
static GfxTexture *whiteTex;

static void putQuad()
{
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock");
    const float px[4] = { -1.f, 1.f, -1.f, 1.f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.99f;   // far: raw depth ~0.993
        v[i].nz = -1.f;
        v[i].diffuse = 0xff242022;                        // dark night road (ARGB)
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}

static void drawAndRead(DWORD *pixels)
{
    Gfx_SetPostFog(TRUE, 70, 100);   // re-arm per frame: the executor one-shots post state
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_EndWorldEffects();   // emits the race-world command that runs the fog pass
    DWORD dummy[4];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);   // marks the request
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}

static int red(const DWORD *f, int x, int y) { return (int)((f[y * W + x] >> 16) & 0xff); }   // ARGB red byte

static void dumpPpm(const char *prefix, const char *tag, const DWORD *pixels)
{
    if (!prefix) return;
    FILE *f = fopen((std::string(prefix) + "-" + tag + ".ppm").c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) {
        DWORD px = pixels[i];
        fputc((px >> 16) & 0xff, f); fputc((px >> 8) & 0xff, f); fputc(px & 0xff, f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("fog beam rendered pixels", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");

    // Production projection state for FogBeam_ProjectLights: 96x64, symmetric
    // perspective (w = z). Independent of the ortho used for the scene quad.
    static Graphics gfx;
    memset(&gfx, 0, sizeof gfx);
    gfx.resX = W; gfx.resY = H;
    g_pGraphics = &gfx;
    memset(&g_texManager, 0, sizeof g_texManager);
    g_texManager.fixedProjection[0] = 0x10000;
    g_texManager.fixedProjection[5] = 0x10000;
    g_texManager.fixedProjection[10] = 0x10000;
    g_texManager.fixedProjection[11] = 0x10000;

    // Chase camera at the origin looking +Z. Rear lamp 2 units left, 8 ahead,
    // beam 30 units backwards: the beam end sits 22 units BEHIND the camera -
    // the exact geometry that produced the upper-left streak.
    static SceneNode view;
    memset(&view, 0, sizeof view);
    FixMatrix_Identity(&view.world);
    LightField_Clear();
    FixVector lampPos = {-0x20000, 0, 0x80000};        // (-2, 0, 8)
    FixVector lampDir = {0, 0, -0x10000};              // backwards
    require(LightField_Add(&lampPos, &lampDir, 0x10000, 0x10000, 0x1200, 0) == 1, "light add");
    GfxFogLight lights[LIGHT_FIELD_MAX];
    int n = FogBeam_ProjectLights(lights, LIGHT_FIELD_MAX, &view, (float)W, (float)H);
    require(n == 1, "one projected light");
    require(lights[0].x > 0.30f && lights[0].x < 0.45f, "lamp u");
    require(lights[0].y > 0.45f && lights[0].y < 0.55f, "lamp v");
    require(lights[0].beamX < -0.9f && lights[0].beamY > -0.05f && lights[0].beamY < 0.05f,
            "wedge direction horizontal-left, no upward drift");

    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    GfxMatrix proj = { 1,0,0,0, 0,1,0,0, 0,0,1.f/1.5f,0, 0,0,-0.5f/1.5f,1 };
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &proj);
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetRaceWorld(TRUE);
    Gfx_SetPostFog(TRUE, 70, 100);   // his reference settings: amount 70, glow 100

    vb = Gfx_CreateVertexBuffer(4);
    require(vb != NULL, "vertex buffer");
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0);
    require(whiteTex != NULL, "white texture");
    {   GfxLockedRect lr;
        require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock white");
        memset(lr.pixels, 0xff, 4);
        Gfx_UnlockTexture(whiteTex, 0, 0);
    }

    const char *prefix = argc > 1 ? argv[1] : nullptr;
    static DWORD control[W * H], lit[W * H];
    // 1. Control: fog only, no lights.
    Gfx_SetFogLights(nullptr, 0);
    putQuad();
    drawAndRead(control);
    // 2. Lit: fog + the near-plane-crossing beam.
    Gfx_SetFogLights(lights, n);
    putQuad();
    drawAndRead(lit);

    dumpPpm(prefix, "control", control);
    dumpPpm(prefix, "lit", lit);
    require(memcmp(control, lit, sizeof control) != 0, "beam changes pixels");
    // The wedge runs horizontally LEFT from the lamp (lamp at px 36,32):
    // a pixel on the beam path brightens well beyond the control.
    require(red(lit,12,32) > red(control,12,32) + 24, "beam path brightens");
    // The reported bug: the streak ran to the upper-left corner. It must not:
    require(red(lit,2,2) <= red(control,2,2) + 8, "no upper-left streak (corner)");
    require(red(lit,12,8) <= red(control,12,8) + 8, "no upward drift above the beam");
    // No backward shine: right of the lamp stays at the control level.
    require(red(lit,84,32) <= red(control,84,32) + 8, "no backward shine");
    printf("fog_beam_render ok\n");
    return 0;
}
