// Reproduction of the 2026-11-01 15:35 user screenshots ("lights are still
// funny"): shot3 chase rear wash, shot1/shot2 rolled car-mounted beams.
// Local investigation harness on the fog_beam_render GL skeleton.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "../src/render/gfx_core.cpp"
#include "../src/render/gfx_gl.cpp"
#include "../src/diagnostics/observer.cpp"
#include "../game/FixedMatrixCore.cpp"
#include "FogBeamProjection.h"
#include <cmath>
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

extern const float g_floatHalf = 0.5f;
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
Graphics *g_pGraphics;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;

static const int W = 480, H = 312;   // 1.54 aspect = user screenshot geometry
static GfxVertexBuffer *vb;
static GfxTexture *whiteTex;

static void fail(const char *msg) { fprintf(stderr, "FAIL: %s\n", msg); exit(1); }
static void require(bool ok, const char *msg) { if (!ok) fail(msg); }

static void matFromYPR(FixMatrix *m, double px, double py, double pz, double yawDeg, double pitchDeg, double rollDeg) {
    double cy=cos(yawDeg*M_PI/180), sy=sin(yawDeg*M_PI/180);
    double cp=cos(pitchDeg*M_PI/180), sp=sin(pitchDeg*M_PI/180);
    double cr=cos(rollDeg*M_PI/180), sr=sin(rollDeg*M_PI/180);
    double rx=cy, ry=0, rz=-sy;
    double ux=sy*sp, uy=cp, uz=cy*sp;
    double fx=sy*cp, fy=-sp, fz=cy*cp;
    double nrx=rx*cr+ux*sr, nry=ry*cr+uy*sr, nrz=rz*cr+uz*sr;
    double nux=ux*cr-rx*sr, nuy=uy*cr-ry*sr, nuz=uz*cr-rz*sr;
    m->right.x=(int)(nrx*65536); m->right.y=(int)(nry*65536); m->right.z=(int)(nrz*65536);
    m->up.x=(int)(nux*65536); m->up.y=(int)(nuy*65536); m->up.z=(int)(nuz*65536);
    m->forward.x=(int)(fx*65536); m->forward.y=(int)(fy*65536); m->forward.z=(int)(fz*65536);
    m->position.x=(int)(px*65536); m->position.y=(int)(py*65536); m->position.z=(int)(pz*65536);
}
static void addCarLights(double yaw, double pitch, double roll, int rearRed) {
    FixMatrix car; matFromYPR(&car, 0,0,0, yaw,pitch,roll);
    LightField_Clear();
    const double lampYF=0.55, lampZF=2.05, lampYR=0.70, lampZR=-2.0, lampXF=0.65, lampXR=0.62;
    struct {double x,y,z,dx,dy,dz; int r,g,b;} L[4] = {
        {-lampXF,lampYF,lampZF, 0,0,1, 0x10000,0x10000,0x10000}, {lampXF,lampYF,lampZF, 0,0,1, 0x10000,0x10000,0x10000},
        {-lampXR,lampYR,lampZR, 0,0,-1, 0x10000,0x1200,0}, {lampXR,lampYR,lampZR, 0,0,-1, 0x10000,0x1200,0}};
    for (auto &l : L) {
        FixVector lp={(int)(l.x*65536),(int)(l.y*65536),(int)(l.z*65536)};
        FixVector ld={(int)(l.dx*65536),(int)(l.dy*65536),(int)(l.dz*65536)};
        FixVector wp,wd; FixMatrix_RotateVector(&wp,&lp,&car); FixMatrix_RotateVector(&wd,&ld,&car);
        LightField_Add(&wp,&wd,0x10000, rearRed?l.r:0x10000, rearRed?l.g:0x10000, rearRed?l.b:0x10000);
    }
}
static void putQuad()
{
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock");
    const float px[4] = { -1.f, 1.f, -1.f, 1.f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 1.99f;
        v[i].nz = -1.f;
        v[i].diffuse = 0xff242022;
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}
// Near "hill": a wall on the left at z=0.8, in FRONT of the shot-3 rear
// lamps (z~9-11) and covering the wedge's mid-span screen area. The wedge's
// depth gate tests only the LAMP depth, so if the beam crosses BEHIND the
// hill, the wedge must be occluded there - the lamp-only gate cannot do that.
static void putHill() {
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock hill");
    const float px[4] = { -1.f, -0.35f, -1.f, -0.35f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = 0.80f;
        v[i].nz = -1.f;
        v[i].diffuse = 0xff203018;   // dark green hill (ARGB)
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}
// Mid-depth wall on the RIGHT, behind the lamps but in front of the front
// beams' far segments: the wedge tail past the beam/wall intersection must
// NOT paint it, but the lamp-anchored depth gate passes the whole wall.
static float g_wallZ = 1.73f;   // discriminating depth: raw past the lamp ref, before the beam-end ref (2026-11-01 llvmpipe 480x312: old gate 13,782 wall px vs segment gate 3,710; z<=1.72 blocks BOTH gates, z>=1.74 passes both)
static void putWall() {
    GfxVertex *v = Gfx_LockVertexBuffer(vb);
    require(v != NULL, "lock wall");
    const float px[4] = { 0.15f, 1.0f, 0.15f, 1.0f };
    const float py[4] = { -1.f, -1.f, 1.f, 1.f };
    for (int i = 0; i < 4; i++) {
        memset(&v[i], 0, sizeof v[i]);
        v[i].x = px[i]; v[i].y = py[i]; v[i].z = g_wallZ;
        v[i].nz = -1.f;
        v[i].diffuse = 0xff301820;   // dark red-brown wall (ARGB)
        v[i].specular = 0xff000000;
    }
    Gfx_UnlockVertexBuffer(vb);
}
static void drawWallSceneAndRead(DWORD *pixels) {
    Gfx_SetPostFog(TRUE, 70, 100);
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    putQuad();  Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    putWall();  Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_EndWorldEffects();
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}
static void drawSceneAndRead(DWORD *pixels) {
    Gfx_SetPostFog(TRUE, 70, 100);
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    putQuad();  Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    putHill();  Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_EndWorldEffects();
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}
static void drawAndRead(DWORD *pixels) {
    Gfx_SetPostFog(TRUE, 70, 100);
    Gfx_SetRaceWorld(TRUE);
    Gfx_Clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff181c20, 1.0f);
    Gfx_SetTexture(0, whiteTex);
    Gfx_DrawPrimitiveVB(GFX_TRIANGLESTRIP, vb, 0, 4);
    Gfx_EndWorldEffects();
    static DWORD dummy[W * H];
    (void)Gfx_ReadBackBuffer(0, 0, W, H, dummy);
    Gfx_Present(0);
    require(Gfx_ReadBackBuffer(0, 0, W, H, pixels), "readback");
}
static int red(const DWORD *f, int x, int y) { return (int)((f[y * W + x] >> 16) & 0xff); }
static int green(const DWORD *f, int x, int y) { return (int)((f[y * W + x] >> 8) & 0xff); }
static int blue(const DWORD *f, int x, int y) { return (int)(f[y * W + x] & 0xff); }
static void dumpPpm(const char *prefix, const char *name, const DWORD *pixels) {
    if (!prefix) return; char path[256]; snprintf(path, sizeof path, "%s-%s.ppm", prefix, name);
    FILE *f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) { DWORD px = pixels[i]; fputc((px>>16)&0xff,f); fputc((px>>8)&0xff,f); fputc(px&0xff,f); }
    fclose(f);
}
static void measure(const char *name, const DWORD *control, const DWORD *lit) {
    int lifted = 0, leftLifted = 0, maxLift = 0; long sumLift = 0;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int lr = red(lit,x,y), cr = red(control,x,y);
        int d = lr - cr;
        if (d > 16) { lifted++; if (x < W/2) leftLifted++; }
        if (d > maxLift) maxLift = d;
        if (d > 0) sumLift += d;
    }
    printf("%-22s red-lift>16: %3d/%d (%5.1f%%)  left-half: %3d  maxLift %3d  totalLift %ld\n",
           name, lifted, W*H, 100.0*lifted/(W*H), leftLifted, maxLift, sumLift);
}

int main(int argc, char **argv) {
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow = SDL_CreateWindow("fog repro", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(testWindow != NULL, SDL_GetError());
    require(Gfx_SetVideoMode(W, H, FALSE), "video mode");
    static Graphics gfx; memset(&gfx, 0, sizeof gfx); gfx.resX = W; gfx.resY = H; g_pGraphics = &gfx;
    memset(&g_texManager, 0, sizeof g_texManager);
    g_texManager.fixedProjection[0] = 0x10000; g_texManager.fixedProjection[5] = 0x10000;
    g_texManager.fixedProjection[10] = 0x10000; g_texManager.fixedProjection[11] = 0x10000;
    GfxViewport vp = { 0, 0, W, H, 0.f, 1.f };
    Gfx_SetViewport(&vp);
    GfxMatrix identity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &identity);
    GfxMatrix proj = { 1,0,0,0, 0,1,0,0, 0,0,1.f/1.5f,0, 0,0,-0.5f/1.5f,1 };
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &proj);
    Gfx_SetLighting(FALSE); Gfx_SetCullMode(GFX_CULL_NONE);
    vb = Gfx_CreateVertexBuffer(4); require(vb != NULL, "vb");
    whiteTex = Gfx_CreateTexture(1, 1, GFX_FORMAT_BGRA8, 1, 0); require(whiteTex != NULL, "tex");
    { GfxLockedRect lr; require(Gfx_LockTexture(whiteTex, 0, 0, &lr), "lock"); memset(lr.pixels, 0xff, 4); Gfx_UnlockTexture(whiteTex, 0, 0); }
    const char *prefix = argc > 1 ? argv[1] : nullptr;
    if (argc > 2) g_wallZ = (float)atof(argv[2]);
    static DWORD control[W * H], lit[W * H];

    // Control once (no lights).
    Gfx_SetFogLights(nullptr, 0); putQuad(); drawAndRead(control);
    dumpPpm(prefix, "control", control);

    // SHOT 3: chase drone (0,2.4,-6.8 pitch 14), car yaw 25 (per screenshots).
    { SceneNode view; memset(&view,0,sizeof view); matFromYPR(&view.world, 0.0,2.40,-6.8, 0,14, 0);
      addCarLights(25,0,0, 1);
      GfxFogLight fl[LIGHT_FIELD_MAX];
      int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, (float)W,(float)H, 1.0f/1.5f, -0.5f/1.5f);
      printf("shot3 lights=%d\n", n);
      for (int i=0;i<n;i++) printf("  L%d uv=(%.3f,%.3f) beam=(%+.2f,%+.2f) len=%.3f ref=%.3f\n", i, fl[i].x, fl[i].y, fl[i].beamX, fl[i].beamY, fl[i].beamLen, fl[i].depthRef);
      if (getenv("FOG_NOQ")) for (int i=0;i<n;i++) fl[i].beamFade = 1.0f;
      if (getenv("FOG_NOSEG")) for (int i=0;i<n;i++) fl[i].beamEndRef = fl[i].depthRef;
      Gfx_SetFogLights(fl, n); putQuad(); drawAndRead(lit);
      measure("shot3 chase-yaw25", control, lit); dumpPpm(prefix, "shot3", lit);
      // Hill scene: same lights, near wall in the wedge's path.
      Gfx_SetFogLights(nullptr, 0); drawSceneAndRead(control);
      dumpPpm(prefix, "shot3-hill-control", control);
      Gfx_SetFogLights(fl, n); drawSceneAndRead(lit);
      measure("shot3 with hill", control, lit);
      // Where did the lift land? Split hill region (x<0.325W) vs rest.
      { int hillLift=0, farLift=0, hillMax=0, farMax=0;
        for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
            int d = red(lit,x,y) - red(control,x,y);
            if (x < (int)(W*0.325)) { if (d>16) hillLift++; if (d>hillMax) hillMax=d; }
            else { if (d>16) farLift++; if (d>farMax) farMax=d; }
        }
        printf("  hill region lift>16: %d (max %d), far region: %d (max %d)\n", hillLift, hillMax, farLift, farMax);
      }
      dumpPpm(prefix, "shot3-hill", lit); }

    // SHOT 1: car-mounted (0,1.05,0.35 pitch 4), car+cam roll 180.
    { SceneNode view; memset(&view,0,sizeof view); matFromYPR(&view.world, 0.0,1.05,0.35, 0,4, 180);
      addCarLights(0,0,180, 1);
      GfxFogLight fl[LIGHT_FIELD_MAX];
      int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, (float)W,(float)H, 1.0f/1.5f, -0.5f/1.5f);
      printf("shot1 lights=%d\n", n);
      for (int i=0;i<n;i++) printf("  L%d uv=(%.3f,%.3f) beam=(%+.2f,%+.2f) len=%.3f ref=%.3f\n", i, fl[i].x, fl[i].y, fl[i].beamX, fl[i].beamY, fl[i].beamLen, fl[i].depthRef);
      if (getenv("FOG_NOQ")) for (int i=0;i<n;i++) fl[i].beamFade = 1.0f;
      if (getenv("FOG_NOSEG")) for (int i=0;i<n;i++) fl[i].beamEndRef = fl[i].depthRef;
      Gfx_SetFogLights(fl, n); putQuad(); drawAndRead(lit);
      measure("shot1 car-mounted roll180", control, lit); dumpPpm(prefix, "shot1", lit); }

    // SHOT 2: car-mounted roll 90.
    { SceneNode view; memset(&view,0,sizeof view); matFromYPR(&view.world, 0.0,1.05,0.35, 0,4, 90);
      addCarLights(0,0,90, 1);
      GfxFogLight fl[LIGHT_FIELD_MAX];
      int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, (float)W,(float)H, 1.0f/1.5f, -0.5f/1.5f);
      printf("shot2 lights=%d\n", n);
      for (int i=0;i<n;i++) printf("  L%d uv=(%.3f,%.3f) beam=(%+.2f,%+.2f) len=%.3f ref=%.3f\n", i, fl[i].x, fl[i].y, fl[i].beamX, fl[i].beamY, fl[i].beamLen, fl[i].depthRef);
      if (getenv("FOG_NOQ")) for (int i=0;i<n;i++) fl[i].beamFade = 1.0f;
      if (getenv("FOG_NOSEG")) for (int i=0;i<n;i++) fl[i].beamEndRef = fl[i].depthRef;
      Gfx_SetFogLights(fl, n); putQuad(); drawAndRead(lit);
      measure("shot2 car-mounted roll90", control, lit); dumpPpm(prefix, "shot2", lit); }

    // SHOT4 (constructed discriminator): chase view, car yaw 50, FRONT lamps
    // only. Beam: lamp z~8.6 -> end z~28 (not clipped). Wall at world z=14
    // (quad z 1.464: raw 0.643 > lamp gate ref 0.632) covers the wedge's right
    // half; wedge segments past the beam/wall crossing are BEHIND the wall and
    // must not paint it - the lamp-anchored gate passes the whole wall.
    { SceneNode view; memset(&view,0,sizeof view); matFromYPR(&view.world, 0.0,2.40,-6.8, 0,14, 0);
      LightField_Clear();
      { FixMatrix car; matFromYPR(&car, 0,0,0, 50,0,0);
        const double lampYF=0.55, lampZF=2.05, lampXF=0.65;
        struct {double x,y,z;} L[2] = {{-lampXF,lampYF,lampZF},{lampXF,lampYF,lampZF}};
        for (auto &l : L) {
            FixVector lp={(int)(l.x*65536),(int)(l.y*65536),(int)(l.z*65536)};
            FixVector ld={0,0,0x10000};
            FixVector wp,wd; FixMatrix_RotateVector(&wp,&lp,&car); FixMatrix_RotateVector(&wd,&ld,&car);
            LightField_Add(&wp,&wd,0x10000, 0x10000,0x10000,0x10000);
        } }
      GfxFogLight fl[LIGHT_FIELD_MAX];
      int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, (float)W,(float)H, 1.0f/1.5f, -0.5f/1.5f);
      printf("shot4 front-lamps=%d\n", n);
      for (int i=0;i<n;i++) printf("  L%d uv=(%.3f,%.3f) beam=(%+.2f,%+.2f) len=%.3f ref=%.3f q=%.2f\n", i, fl[i].x, fl[i].y, fl[i].beamX, fl[i].beamY, fl[i].beamLen, fl[i].depthRef, fl[i].beamFade);
      if (getenv("FOG_NOQ")) for (int i=0;i<n;i++) fl[i].beamFade = 1.0f;   // harness override: reproduce main's unpatched behavior
      if (getenv("FOG_NOGATE")) for (int i=0;i<n;i++) fl[i].depthRef = -1.0f;   // harness override: disable the depth gate
      if (getenv("FOG_NOSEG")) for (int i=0;i<n;i++) fl[i].beamEndRef = fl[i].depthRef;   // harness override: constant lamp gate (old behavior)
      Gfx_SetFogLights(nullptr, 0); drawWallSceneAndRead(control);
      Gfx_SetFogLights(fl, n); drawWallSceneAndRead(lit);
      measure("shot4 front-beam wall", control, lit);
      // Beam/wall crossing: first lamp, wall plane z_view = 14.
      int wallLiftPin = 0;
      { FixMatrix invV; FixMatrix_Invert(&invV, &view.world);
        FixVector lampW = {(int)(-0.65*65536),(int)(0.55*65536),(int)(2.05*65536)};
        FixMatrix carM; matFromYPR(&carM, 0,0,0, 50,0,0);
        FixVector lampWorld, dirWorld; FixMatrix_RotateVector(&lampWorld,&lampW,&carM);
        FixVector fDir={0,0,0x10000}; FixMatrix_RotateVector(&dirWorld,&fDir,&carM);
        FixVector lampVw; FixMatrix_TransformPoint(&lampVw,&lampWorld,&invV);
        FixVector dirVw; FixMatrix_RotateVector(&dirVw,&dirWorld,&invV);
        printf("  dbg lampVw=(%.2f,%.2f,%.2f) dirVw=(%.4f,%.4f,%.4f) lampWorld=(%.2f,%.2f,%.2f)\n",
               lampVw.x/65536.0,lampVw.y/65536.0,lampVw.z/65536.0, dirVw.x/65536.0,dirVw.y/65536.0,dirVw.z/65536.0,
               lampWorld.x/65536.0,lampWorld.y/65536.0,lampWorld.z/65536.0);
        double sStar = (14.0*65536 - lampVw.z) / (double)dirVw.z;
        (void)sStar;
        FixVector crossW = { lampWorld.x + (int)(dirWorld.x*sStar), lampWorld.y + (int)(dirWorld.y*sStar), lampWorld.z + (int)(dirWorld.z*sStar) };
        FixVector vOut; FixMatrix_ProjectWorldPointToView((int*)&vOut,&crossW,&view);
        float crossU = (float)(vOut.x>>16)/(float)W, crossVf = (float)(vOut.y>>16)/(float)H;
        // a* along the wedge for light 0
        float aStar = (crossU - fl[0].x)*fl[0].beamX*(float)W/(float)H + (crossVf - fl[0].y)*fl[0].beamY;
        printf("  crossing: s*=%.1f units, uv=(%.3f,%.3f), a*=%.3f (len %.3f)\n", sStar, crossU, crossVf, aStar, fl[0].beamLen);
        for (double ss = 0.0; ss <= 27.0; ss += 3.0) {
            FixVector pw = { lampWorld.x + (int)(dirWorld.x*ss), lampWorld.y + (int)(dirWorld.y*ss), lampWorld.z + (int)(dirWorld.z*ss) };
            FixVector po; FixMatrix_ProjectWorldPointToView((int*)&po,&pw,&view);
            printf("    s=%4.1f -> uv=(%.3f,%.3f)\n", ss, (float)(po.x>>16)/(float)W, (float)(po.y>>16)/(float)H);
        }
        int wallLift=0, behindLift=0, behindMax=0, frontLift=0, behindDeep=0;
        for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
            int d = red(lit,x,y) - red(control,x,y);
            if (x <= (int)(W*0.575)) { if (d>16) frontLift++; continue; }
            if (d>4) wallLift++;
            float u=(float)x/W, v2=(float)(H-1-y)/(float)H;   // shader uv is bottom-origin
            float a = (u-fl[0].x)*fl[0].beamX*(float)W/(float)H + (v2-fl[0].y)*fl[0].beamY;
            if (a > aStar && d > 4) { behindLift++; if (d>behindMax) behindMax=d; }
            if (a > 0.25f && d > 4) behindDeep++;  // segments at beam z>~12.3: false for a just-passing wall
        }
        printf("  wall: lift>4 %d px; BEHIND crossing (a>a*): %d px (max %d); deep-false (a>0.25): %d px; off-wall lift>16: %d\n", wallLift, behindLift, behindMax, behindDeep, frontLift);
        // Regression pins (2026-11-01, llvmpipe 480x312, wall z 1.73): the
        // segment gate leaves the near-lamp sliver plus the deliberate 30%
        // occluded residue - measured wallLift 3,710 / behindDeep 1,391 /
        // frontLift 55 vs the old constant gate's 13,782 / 11,309.
        require(wallLift <= 4500, "segment gate: wall paint above sliver+residue bound");
        require(behindDeep <= 2000, "segment gate: deep-false paint past the crossing");
        require(frontLift >= 20, "segment gate: in-front beam blanked (over-suppression)");
        wallLiftPin = wallLift;
        // Where is the wall actually on screen? Scan control for its color.
        { int x0=W,x1=-1,y0=H,y1=-1,cnt=0;
          for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
            int r=red(control,x,y),g=green(control,x,y),b=blue(control,x,y);
            if (r>30&&r<70&&g>10&&g<40&&b>18&&b<50) { if(x<x0)x0=x; if(x>x1)x1=x; if(y<y0)y0=y; if(y>y1)y1=y; cnt++; }
          }
          printf("  wall screen extent: x %d..%d y %d..%d (%d px, W=%d H=%d)\n", x0,x1,y0,y1,cnt,W,H);
        }
      }
      dumpPpm(prefix, "shot4-wall", lit);

      // Sensitivity control: the same scene with beamEndRef degenerated to
      // the lamp ref (the OLD constant gate) MUST paint the wall heavily -
      // otherwise the scene cannot detect the regression at all.
      { GfxFogLight flOld[LIGHT_FIELD_MAX];
        memcpy(flOld, fl, sizeof flOld);
        for (int i = 0; i < n; i++) flOld[i].beamEndRef = flOld[i].depthRef;
        Gfx_SetFogLights(flOld, n); drawWallSceneAndRead(lit);
        int oldWall = 0;
        for (int y=0;y<H;y++) for (int x=(int)(W*0.575)+1;x<W;x++)
            if (red(lit,x,y) - red(control,x,y) > 4) oldWall++;
        printf("  wall: old constant gate lift>4 %d px (sensitivity control)\n", oldWall);
        dumpPpm(prefix, "shot4-wall-oldgate", lit);
        require(oldWall >= 10000, "wall scene sensitivity: old constant gate no longer paints the wall - scene broken");
        require(wallLiftPin * 3 <= oldWall, "segment gate: must block at least 2/3 of the old gate's wall paint"); }
      }
    printf("REPRO DONE\n");
    return 0;
}
