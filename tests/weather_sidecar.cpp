// Pins the weather sidecar's pure slice/dial math (game/WeatherSidecar.h).
#include "port/types.h"
#include "WeatherSidecar.h"
#include <cstdio>
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main() {
    // Dial clamp and semantics: 0 hides, 100 = original only, 200 = +400, 300 = +800.
    CHECK(WeatherSidecar_ClampAmount(-5) == 0);
    CHECK(WeatherSidecar_ClampAmount(999) == 300);
    CHECK(WeatherSidecar_ActiveCount(0) == 0);
    CHECK(WeatherSidecar_ActiveCount(50) == 0);
    CHECK(WeatherSidecar_ActiveCount(100) == 0);
    CHECK(WeatherSidecar_ActiveCount(150) == 200);
    CHECK(WeatherSidecar_ActiveCount(200) == 400);
    CHECK(WeatherSidecar_ActiveCount(300) == 800);
    CHECK(WeatherSidecar_ActiveCount(300) <= kWeatherSidecarMax);
    CHECK(!WeatherSidecar_PrecipitationDrawn(0));
    CHECK(WeatherSidecar_PrecipitationDrawn(100));
    // Slices keep the original 400-share proportions and never exceed the pool.
    int f, c;
    WeatherSidecar_ViewSlice(100, 200, 400, f, c);   // half-pool view at 200%
    CHECK(f == 100 && c == 200);
    WeatherSidecar_ViewSlice(0, 400, 800, f, c);     // full-pool view at 300%
    CHECK(f == 0 && c == 800);
    WeatherSidecar_ViewSlice(300, 100, 800, f, c);   // tail view at 300%
    CHECK(f == 600 && c == 200);
    CHECK(f + c <= kWeatherSidecarMax);
    WeatherSidecar_ViewSlice(0, 0, 800, f, c);       // zero-capacity view
    CHECK(c == 0);
    // Draw count follows the original ramp fraction of the slice.
    CHECK(WeatherSidecar_DrawCount(150, 200, 400) == 150);   // ramped 150 of 200 -> same 150 of the 200-strong slice
    CHECK(WeatherSidecar_DrawCount(100, 200, 400) == 100);
    CHECK(WeatherSidecar_DrawCount(0, 200, 400) == 0);
    CHECK(WeatherSidecar_DrawCount(50, 0, 400) == 0);        // zero-capacity guard
    CHECK(WeatherSidecar_DrawCount(200, 400, 800) == 400);   // full ramp at 300% doubles the original draw
    std::printf(fails ? "FAILED\n" : "weather sidecar math ok\n");
    return fails ? 1 : 0;
}
