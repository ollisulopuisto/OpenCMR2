#ifndef OPENCMR2_WEATHER_SIDECAR_H
#define OPENCMR2_WEATHER_SIDECAR_H

// PORT: video.weather_particle_amount (0..300, default 200) overdoes the
// precipitation density without touching the original g_weatherParticles[400]
// array, any ViewWeatherState, or the replay rand stream. The port owns a
// sidecar array, simulates it with the same per-view wrap maths on its own
// LCG, and draws each view's sidecar slice right after the original slice
// through the same quad/billboard path.
//
// Dial semantics: 0 hides precipitation (original sim still runs untouched,
// only the draw is skipped); 100 is exactly the original 400; 200 adds 400
// sidecar particles; 300 adds 800. Density scales in multiples of the fixed
// wrap volume - there is no clustering toward the car.

#include "StageWeatherParticle.h"

inline constexpr int kWeatherSidecarMax = 800;   // 2x the original pool

inline int WeatherSidecar_ClampAmount(int amount) {
    return amount < 0 ? 0 : amount > 300 ? 300 : amount;
}
// Extra simulated particles beyond the original 400.
inline int WeatherSidecar_ActiveCount(int amount) {
    int a = WeatherSidecar_ClampAmount(amount);
    return a <= 100 ? 0 : (a - 100) * 4;
}
inline bool WeatherSidecar_PrecipitationDrawn(int amount) {
    return WeatherSidecar_ClampAmount(amount) > 0;
}
// A view's sidecar slice keeps the same proportions as its original share of
// the 400 (firstParticle/particleCapacity), so per-view splits stay coherent.
inline void WeatherSidecar_ViewSlice(int firstParticle, int particleCapacity, int active,
                                     int &sideFirst, int &sideCount) {
    sideFirst = active * firstParticle / 400;
    sideCount = active * particleCapacity / 400;
}
// The original draw count is the ramped particleCount >> 16; the sidecar
// draws the same fraction of its slice (weather ramp hits both equally).
inline int WeatherSidecar_DrawCount(int originalDrawCount, int particleCapacity, int active) {
    if (particleCapacity <= 0) return 0;
    int sideCount = active * particleCapacity / 400;
    return (int)((long long)originalDrawCount * sideCount / particleCapacity);
}

extern StageWeatherParticle g_weatherSidecar[kWeatherSidecarMax];

int WeatherSidecar_Amount(void);                 // ini option, clamped
void WeatherSidecar_Reset(void);                 // stage load: own-LCG init
struct ViewWeatherState;
void WeatherSidecar_Integrate(ViewWeatherState *rec, int view);

#endif
