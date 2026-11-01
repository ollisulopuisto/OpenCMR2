// Scene-matrix continuity regression for the fog-pass car lights
// (video.fog_lights): drives the PRODUCTION FogBeam_ProjectLights through
// cockpit/bonnet/bumper/chase/chase-far/side views against fine car yaw and
// pitch sweeps, and fails on any of the 2026-11-01 "headlights/backlights
// pointing all over" signatures:
//   - the behind-camera sentinel (-100,-100 px) surfacing as a lamp/halo uv
//     (near-plane-clipped anchors lost ~1 ulp in the view->world->view round
//     trip: bonnet/bumper lamps carried the sentinel);
//   - a wedge direction flip or length jump between adjacent sweep samples at
//     visible length (the transverse-formula length re-amplified any off-axis
//     angle past ~2 degrees to a full-screen wedge sweeping up/down/sideways);
//   - a halo depth reference outside (0,1] (the 3-unit forward anchor put
//     REAR lamp halos within a unit of the chase camera: the gate reference
//     went negative, the shader's halos[i].z > 0 test disabled the gate, and
//     red bubbles painted over the car unoccluded);
//   - an on-screen halo position jump between samples.
// CPU-only: no GL, no assets. Race projection constants from Game.cpp's
// SetProjection(0x25645,0x4326e,0xfa0000,0x10000) with m_projectionScale 0.5.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cstdio>
#include <cstring>
#include <cmath>
#include "Graphics.h"
#include "FixedPoint.h"
#include "SceneNode.h"
#include "LightField.h"
#include "FogBeamProjection.h"

FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;

// Test-scoped definitions of the globals/functions the compiled game TUs
// reference outside the projection path under test (View_* helpers pulled in
// via FixedPoint.cpp). Never called by this test.
extern const float g_floatHalf = 0.5f;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
Graphics *g_pGraphics;
static Graphics g_gfx;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;

static void matFromYPR(FixMatrix *m, double px, double py, double pz, double yawDeg, double pitchDeg) {
    double cy = cos(yawDeg*M_PI/180), sy = sin(yawDeg*M_PI/180), cp = cos(pitchDeg*M_PI/180), sp = sin(pitchDeg*M_PI/180);
    m->right.x=(int)(cy*65536); m->right.y=0; m->right.z=(int)(-sy*65536);
    m->up.x=(int)(sy*sp*65536); m->up.y=(int)(cp*65536); m->up.z=(int)(cy*sp*65536);
    m->forward.x=(int)(sy*cp*65536); m->forward.y=(int)(-sp*65536); m->forward.z=(int)(cy*cp*65536);
    m->position.x=(int)(px*65536); m->position.y=(int)(py*65536); m->position.z=(int)(pz*65536);
}
struct ViewDef { const char *name; double x,y,z,yaw,pitch; };
static const ViewDef VIEWS[] = {
    {"cockpit", 0.0,1.05,0.35, 0,4}, {"bonnet", 0.0,0.95,1.75, 0,6},
    {"bumper",  0.0,0.55,2.35, 0,2}, {"chase",  0.0,2.40,-6.8, 0,14},
    {"chase-far",0.0,3.00,-11.0,0,11},{"side",   7.5,1.60,0.0, -90,6},
};
int failures = 0;
static void fail(const char *view, double ang, bool isYaw, int li, const char *what, float a, float b) {
    printf("FAIL %-9s %s=%+.0f L%d %s (%.3f vs %.3f)\n", view, isYaw?"yaw":"pitch", ang, li, what, a, b);
    failures++;
}
int main() {
    memset(&g_gfx,0,sizeof g_gfx); g_gfx.resX=1920; g_gfx.resY=1080; g_pGraphics=&g_gfx;
    memset(&g_texManager,0,sizeof g_texManager);
    g_texManager.fixedProjection[0]=(int)(2.33711243*0.5*65536.0);
    g_texManager.fixedProjection[5]=(int)(4.19720459*0.5*65536.0);
    g_texManager.fixedProjection[10]=(int)(250.0/249.0*65536.0);
    g_texManager.fixedProjection[11]=0x10000;
    g_texManager.fixedProjection[14]=(int)(-250.0/249.0*65536.0);
    const double P33=250.0/249.0, P43=-250.0/249.0;
    const double lampYF=0.55, lampZF=2.05, lampYR=0.70, lampZR=-2.0, lampXF=0.65, lampXR=0.62;
    for (const ViewDef &v : VIEWS) {
        SceneNode view; memset(&view,0,sizeof view);
        matFromYPR(&view.world, v.x,v.y,v.z, v.yaw,v.pitch);
        for (int axis=0; axis<2; axis++) {
            double lo = axis==0 ? -40 : -12, hi = axis==0 ? 40 : 12, step = 1.0;
            GfxFogLight prev[LIGHT_FIELD_MAX]; int prevN = -1;
            for (double a=lo; a<=hi+0.001; a+=step) {
                FixMatrix car; matFromYPR(&car, 0,0,0, axis==0?a:0, axis==1?a:0);
                LightField_Clear();
                struct {double x,y,z,dx,dy,dz;} L[4] = {
                    {-lampXF,lampYF,lampZF, 0,0,1}, {lampXF,lampYF,lampZF, 0,0,1},
                    {-lampXR,lampYR,lampZR, 0,0,-1}, {lampXR,lampYR,lampZR, 0,0,-1}};
                for (auto &l : L) {
                    // Car at the world origin: rotation only, no translation.
                    FixVector lp={(int)(l.x*65536),(int)(l.y*65536),(int)(l.z*65536)};
                    FixVector ld={(int)(l.dx*65536),(int)(l.dy*65536),(int)(l.dz*65536)};
                    FixVector wp,wd; FixMatrix_RotateVector(&wp,&lp,&car); FixMatrix_RotateVector(&wd,&ld,&car);
                    LightField_Add(&wp,&wd,0x10000, 0x10000,0x10000,0x10000);
                }
                GfxFogLight fl[LIGHT_FIELD_MAX];
                int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
                for (int i=0;i<n;i++) {
                    // sentinel check: the behind-camera projector returns exactly
                    // (-100 px, -100 px) -> (-100/1920, -100/1080); real off-screen
                    // projections are fine (the beam can still enter the frame).
                    if (fabsf(fl[i].x - (-100.0f/1920.0f)) < 0.002f && fabsf(fl[i].y - (-100.0f/1080.0f)) < 0.002f)
                        fail(v.name,a,axis==0,i,"lamp uv is the behind-camera sentinel", fl[i].x, fl[i].y);
                    // halo pinned to the lamp: any divergence or invalid ref is a regression
                    if ((fl[i].haloX != fl[i].x || fl[i].haloY != fl[i].y) && (fl[i].haloRef <= 0.0f || fl[i].haloRef > 1.0f))
                        fail(v.name,a,axis==0,i,"halo gate invalid (ungated bubble)", fl[i].haloRef, 0);
                    if (fabsf(fl[i].haloX - (-100.0f/1920.0f)) < 0.002f && fabsf(fl[i].haloY - (-100.0f/1080.0f)) < 0.002f)
                        fail(v.name,a,axis==0,i,"halo uv is the behind-camera sentinel", fl[i].haloX, fl[i].haloY);
                    if (prevN == n && i < prevN) {
                        // direction flip at visible length on BOTH samples
                        if (fl[i].beamLen > 0.05f && prev[i].beamLen > 0.05f) {
                            float dot = fl[i].beamX*prev[i].beamX + fl[i].beamY*prev[i].beamY;
                            if (dot < 0.0f) fail(v.name,a,axis==0,i,"wedge direction flip at visible length", prev[i].beamLen, fl[i].beamLen);
                            if (fabsf(fl[i].beamLen - prev[i].beamLen) > 0.15f)
                                fail(v.name,a,axis==0,i,"wedge length jump", prev[i].beamLen, fl[i].beamLen);
                        }
                        float hdx = fl[i].haloX-prev[i].haloX, hdy = fl[i].haloY-prev[i].haloY;
                        bool onscreen = fl[i].haloX > -0.05f && fl[i].haloX < 1.05f && fl[i].haloY > -0.05f && fl[i].haloY < 1.05f &&
                                        prev[i].haloX > -0.05f && prev[i].haloX < 1.05f && prev[i].haloY > -0.05f && prev[i].haloY < 1.05f;
                        if (onscreen && sqrtf(hdx*hdx+hdy*hdy) > 0.10f)
                            fail(v.name,a,axis==0,i,"halo position jump on screen", prev[i].haloX, fl[i].haloX);
                    }
                }
                memcpy(prev, fl, sizeof fl); prevN = n;
            }
        }
    }
    printf(failures==0 ? "SWEEP CLEAN\n" : "SWEEP: %d failures\n", failures);
    return failures != 0;
}
