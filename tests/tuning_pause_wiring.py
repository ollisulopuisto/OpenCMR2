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
gm = (pathlib.Path(__file__).resolve().parent.parent / "game" / "Game.cpp").read_text()
m = re.search(r"if \(CGameInfo::IsInRaceMenuOpen\(\) != 0\) \{.*?return FALSE;", gm, re.S)
assert m, "in-race menu branch of UpdateInRaceCallbackMachine not found"
b = m.group(0)
assert "RunStateUpdateCallbacks" not in b, "this branch skips the state callbacks: that is why the Esc-menu pause never ran the panel"
assert b.index("InRaceMenu_UpdateStateMachineFrame") < b.index("TuningPanel_Update();") < b.index("InRaceMenu_AdvanceScreenEntries"), "panel update must follow the menu input read and precede the draw"
assert "View_RefreshForTuning();" in b
assert "Race_ApplySpeedEffectParams();" in b, "tunnel/streak/vapor targets are only set in the race step: refresh them while paused"
assert "Gfx_SetRaceWorld(CGameInfo::IsInRaceMenuOpen()==0 || (TuningPanel_PreviewActive()" in rd, "the Esc menu turns every race effect off in the renderer unless the panel is open"
assert "Race_ApplySpeedEffectParams();" in body or "Race_ApplySpeedEffectParams(); }" in body
print("tuning pause wiring ok")
