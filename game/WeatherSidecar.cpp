// PORT: precipitation sidecar - see WeatherSidecar.h for the contract. This
// mirrors the original wrap logic from StageObject_IntegrateViewDeformationGrid
// (CMR2 0x0045f9d0) and the init from StageWeather_DistributeViewParticles
// (CMR2 0x0045eed0) on a port-owned array; the originals run untouched.
#include "WeatherSidecar.h"
#include "StageWeather.h"
#include "PrecipitationQuad.h"
#include "FixedPoint.h"
#include "port/sys.h"

StageWeatherParticle g_weatherSidecar[kWeatherSidecarMax];

extern PrecipitationQuad g_precipitation;
extern FixVector g_weatherWindDirection;
extern int g_weatherWindStrength;
extern int g_stageObjectTimeStep;

int WeatherSidecar_Amount(void) {
    return WeatherSidecar_ClampAmount(Sys_GetOption("video.weather_particle_amount", 200));
}

// Own LCG (xorshift32): the gameplay rand stream and replay determinism stay
// untouched. Render-only positions; deterministic per reset.
static unsigned int g_weatherSidecarLcg = 0x9e3779b9u;
static unsigned int WeatherSidecar_Rand(void) {
    g_weatherSidecarLcg ^= g_weatherSidecarLcg << 13;
    g_weatherSidecarLcg ^= g_weatherSidecarLcg >> 17;
    g_weatherSidecarLcg ^= g_weatherSidecarLcg << 5;
    return g_weatherSidecarLcg;
}
static int WeatherSidecar_RandFix(void) {   // 0..0x10000 like RAND_FIX()
    return (int)(WeatherSidecar_Rand() >> 16) & 0xffff;
}

void WeatherSidecar_Reset(void) {
    g_weatherSidecarLcg = 0x9e3779b9u;
    for (int i = 0; i < kWeatherSidecarMax; i++) {
        StageWeatherParticle *pNode = &g_weatherSidecar[i];
        pNode->x = FixMul(WeatherSidecar_RandFix(), 0x180000);
        pNode->y = FixMul(WeatherSidecar_RandFix(), 0xc0000);
        pNode->z = FixMul(WeatherSidecar_RandFix(), 0x180000);
        pNode->spin = FixMul(WeatherSidecar_RandFix(), 0x40000);
        pNode->swayAmplitude = WeatherSidecar_RandFix();
        pNode->snowPhaseDegrees = FixMul(WeatherSidecar_RandFix(), 0x1680000);
        pNode->scale = WeatherSidecar_RandFix();
        pNode->field_0x1c = 0x4b;
        pNode->textureVariant = (BYTE)(WeatherSidecar_Rand() % 3);
        pNode->wrapped = 0;
    }
}

// Wrap one view's sidecar slice exactly like the original block: box centred
// on rec->centre, terrain-following rain, swaying snow, own drift scale.
void WeatherSidecar_Integrate(ViewWeatherState *rec, int view) {
    int active = WeatherSidecar_ActiveCount(WeatherSidecar_Amount());
    if (active == 0 || (rec->kind != 1 && rec->kind != 2))
        return;
    int sideFirst, sideCount;
    WeatherSidecar_ViewSlice(rec->firstParticle, rec->particleCapacity, active, sideFirst, sideCount);
    if (sideCount <= 0)
        return;

    // Same wrap origin as the original block: box motion is velocity*dt and
    // the centre already carries the zoom drift (applied before the wrap).
    FixVector gn, sv, corr, h;
    (void)view;
    FixVecScale(&corr, &rec->velocity, g_stageObjectTimeStep);
    corr.x -= rec->centre.x; corr.y -= rec->centre.y; corr.z -= rec->centre.z;
    int wheelScale = 0;
    if (rec->kind == 2) {
        sv.x = g_weatherWindDirection.z;
        sv.y = 0;
        sv.z = -g_weatherWindDirection.x;
        FixVecScale(&sv, &sv, FixMul(g_weatherWindStrength, 0x8000));
        wheelScale = FixMul(rec->particleCount, g_precipitation.particleScale);
        if (wheelScale > 0x10000)
            wheelScale = 0x10000;
        wheelScale = FixMul(g_stageObjectTimeStep, wheelScale);
    }
    for (int i = 0; i < sideCount; i++) {
        StageWeatherParticle *pNode = &g_weatherSidecar[sideFirst + i];
        int gx = 0, gz = 0, d;
        h.x = pNode->x + corr.x;
        h.y = pNode->y + corr.y;
        h.z = pNode->z + corr.z;
        if (rec->kind == 2) {
            FixVecScale(&gn, &sv, FixMul(wheelScale, pNode->scale));
            h.x += gn.x;
            h.y += gn.y;
            h.z += gn.z;
        }
        pNode->wrapped = 0;
        if (h.x > 0xc0000)
            h.x -= (unsigned int)(h.x + 0xbffff) / 0x180000 * 0x180000;
        else if (h.x < -0xc0000)
            h.x += (unsigned int)(0xbffff - h.x) / 0x180000 * 0x180000;
        if (rec->kind == 1) {
            d = 0xc0000 - h.x;
            if (d >= 0x4cccc)
                gx = (unsigned int)d / 0x4cccc;
            d = 0xc0000 - h.z;
            if (d >= 0x4cccc)
                gz = (unsigned int)d / 0x4cccc;
            if (gx < 5 && gz < 5)
                h.y += rec->relativeGroundHeights[gx * 5 + gz];
        }
        if (h.y > 0x60000) {
            do {
                h.y -= 0xc0000;
            } while (h.y > 0x60000);
        } else if (h.y < -0x60000) {
            do {
                h.y += 0xc0000;
            } while (h.y < -0x60000);
            pNode->wrapped = 1;
        }
        if (rec->kind == 1 && gx < 5 && gz < 5) {
            h.y -= rec->relativeGroundHeights[gx * 5 + gz];
            pNode->splashGroundHeight = rec->groundHeights[gx * 5 + gz] + 0x1999;
        }
        if (h.z > 0xc0000) {
            do {
                h.z -= 0x180000;
            } while (h.z > 0xc0000);
        } else if (h.z < -0xc0000) {
            do {
                h.z += 0x180000;
            } while (h.z < -0xc0000);
        }
        pNode->x = rec->centre.x + h.x;
        pNode->y = rec->centre.y + h.y;
        pNode->z = h.z + rec->centre.z;
        if (rec->kind == 2) {
            pNode->snowPhaseDegrees += FixMul(pNode->spin, g_stageObjectTimeStep);
            if (pNode->snowPhaseDegrees > 0x1680000)
                pNode->snowPhaseDegrees -= 0x1680000;
        }
    }
}
