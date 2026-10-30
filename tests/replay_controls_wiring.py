#!/usr/bin/env python3
"""Replay controls: clock hook, update/draw sites and key filter exist and are replay-only. Source wiring only, not a native check."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("g_replayClock.Frame(CMain::GetFrameTime(), ReplayControls_TimeScale())" in rd("game/Car.cpp"), "sim clock hook")
rally = rd("game/RallyData.cpp")
need("float ReplayControls_TimeScale(void) { return ReplayControls_Active() ? g_replayControls.Scale() : 1.0f; }" in rally, "scale must be 1 outside replay")
need("if (!ReplayControls_Active()) { g_replayControls.Reset(); return; }" in rally, "state resets when not replaying")
need("ReplayControls_Draw();" in rally and "void ReplayControls_Draw(void)\n{\n    if (!ReplayControls_Active()) return;" in rally, "OSD only while replaying")
need("ReplayControls_Update();" in rd("game/Race.cpp"), "update site")
need("ReplayControls_KeyIsReplayKey" in rd("game/Input.cpp"), "keys must not reach the game")
need("ReplayKeyUsable(key) &&" in rally, "bound driving keys are never taken")
need("int View_CycleReplayCamera(unsigned char view)" in rd("game/Car.cpp") and "ReplayCamera_Next(mode, available)" in rd("game/Car.cpp"), "camera cycle computes the wrap before switching")
need("View_CycleReplayCamera(0);" in rally and "View_FindFreeModeSlot" not in rally.split("static void ReplayControls_CycleCamera")[1].split("void ReplayControls_Update")[0], "no double switch through the game step")
need("ReplayStepsTotal(packets, count)" in rally, "total from the recorded packets")
print("replay controls wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
