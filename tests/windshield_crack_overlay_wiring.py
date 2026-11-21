#!/usr/bin/env python3
"""Wiring checks for video.windshield_crack_overlay (default off, only when no interior glass part)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
wg = rd("game/WindshieldGlass.cpp"); wd = rd("game/WindshieldDirt.cpp"); en = rd("src/platform/enhancements.h"); rb = rd("rebuild.sh")
need('Sys_GetOption("video.windshield_crack_overlay", 0) == 0' in wg, "overlay must default off")
need("g_wgGlassPartCount[car] != 0" in wg, "overlay must only act when no interior glass part was selected")
need("WindshieldCrackPane::Draw(pMesh, group, world" in wg, "pane must use the behaviour-tested production drawer")
need("WindshieldGlass_DrawCrackPane(" in wd and "Windshield_DrawCrackPane(bit)" in rd("game/Game.cpp"), "crack pane must be drawn from the 3D pass")
need("if (stateSlot2 >= 1)" not in wg and "if (stateSlot3 >= 1)" not in wg and "state < 1 ||" not in wg,
     "cached viewport damage must not gate live exterior materials")
need("--windshield-crack-overlay" in en and "video.windshield_crack_overlay" in en, "CLI flag must map to the option")
need("--windshield-crack-overlay" in rb, "rebuild.sh must pass the flag through")
print("windshield crack overlay wiring ok")

# Status belongs to the latest cockpit draw for a car, not a frame-global sum.
draw = wd.split("void Windshield_DrawCrackPane(int view)", 1)[1].split("void Windshield_ResetAll", 1)[0]
need(draw.index("WindshieldGlass_DrawCrackPane(") < draw.index("WindshieldGlass_DrawSidePanes("),
     "resetting front pane must run before side-pane diagnostic aggregation")
front = wg.split("int WindshieldGlass_DrawCrackPane(int car", 1)[1].split("int WindshieldGlass_DrawSidePanes", 1)[0]
need("g_wgPaneStatus[car] = {}" in front, "each cockpit draw must reset its car diagnostic")
need("g_wgPaneStatus[car].Record(count," in wg, "production draws must aggregate their results")
