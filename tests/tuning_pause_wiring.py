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
print("tuning pause wiring ok")
