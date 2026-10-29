#!/usr/bin/env python3
"""The live tuning panel must also run while the race is paused (race logic step does not run then)."""
import re, sys, pathlib
src = (pathlib.Path(__file__).resolve().parent.parent / "game" / "Race.cpp").read_text()
m = re.search(r"void Race_UpdatePauseState\(.*?\n}\n", src, re.S)
assert m, "Race_UpdatePauseState not found"
body = m.group(0)
assert "TuningPanel_Update();" in body, "paused race does not update the tuning panel"
assert body.index("UpdateAllAvailableDevices") < body.index("TuningPanel_Update();") < body.index("Menu_Update"), "tuning update must follow the input read and precede the pause menu update"
assert src.count("TuningPanel_Update();") == 2, "expected one call in the logic step and one in the pause state"
rd = (pathlib.Path(__file__).resolve().parent.parent / "game" / "RallyData.cpp").read_text()
m = re.search(r"void RallyData_DrawProjectedChallengeScene\(.*?\n}\n", rd, re.S)
assert m, "paused scene render not found"
b = m.group(0)
assert "TuningPanel_DrawPaused();" in b and b.index("DrawSceneViewport") < b.index("TuningPanel_DrawPaused();") < b.index("Graphics_PresentFrameAndResetCounters"), "panel must be drawn after the paused scene and before present"
assert rd.count("TuningPanel_Draw();") == 2, "expected the per-view overlay draw and the paused-draw fallback only"
print("tuning pause wiring ok")
