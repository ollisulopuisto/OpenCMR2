# Per-condition race grade (video.color_grading) wiring: the race viewport's
# per-frame sun hook drives the grade from live stage signals, and the race
# frame wrapper restores the restrained menu recipe afterwards so the race
# grade cannot leak into menus. The mapping itself is behaviourally pinned in
# grade_conditions.cpp; this file pins the call sites.
import pathlib, sys

root = pathlib.Path(__file__).resolve().parents[1]

def rd(p): return (root / p).read_text()

hook = rd("game/StageObjects.cpp")
race = rd("game/RallyData.cpp")
settings = rd("src/render/effect_settings.h")

def need(cond, msg):
    if not cond:
        print("grade_conditions_wiring:", msg)
        sys.exit(1)

need("StageObject_UpdateConditionGrade(param_2)" in hook,
     "the per-frame race hook must update the condition grade for every view")
need("StageObject_GetViewWeatherStateByte(view)" in hook,
     "the grade must read the view's weather kind (wet mapping)")
need("StageObject_GetRotatedStageLightVector" in hook,
     "the grade must read the sun vector (golden-hour elevation)")
need("g_stageLighting" in hook and "lightColour" in hook,
     "the grade must read the blended stage light colour (golden-hour warmth)")
need("GradeStrength" in settings and "ApplyGrade" in settings,
     "effect_settings must share the grade strength rule and menu recipe")
need("EffectSettings::ApplyGrade();" in race[race.index("Gfx_SetRaceWorld(FALSE);"):],
     "the race frame wrapper must restore the menu grade after race frames")
print("grade_conditions_wiring OK")
