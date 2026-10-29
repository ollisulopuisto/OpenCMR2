#!/usr/bin/env python3
"""FOV dolly wiring: only chase cameras, never the drone operator, visual offset only, motes no longer spawned, panel row present."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
f = rd("game/FixedPoint.cpp")
need("pObj->type >= 4 && pObj->type <= 6 && !CameraOperator_Enabled(pObj)" in f, "dolly must apply to chase types 4..6 and skip the drone operator")
need("base.x = FixMul(base.x, dolly)" in f and "base.z = FixMul(base.z, dolly)" in f, "dolly must scale the camera offset")
i = f.index("View_FovDollyScale(pObj)"); need(f.index("ChaseCamera_GetViewOffset(&base, pObj);") < i, "dolly must come after the offset is read")
need("FovDolly_Widening(View_SpeedFovMagnification" in f and "FovDolly_Scale(widening" in f, "must use the pure helpers")
need("int View_SpeedFovMagnification(BYTE view)" in rd("game/Car.cpp"), "speed FOV accessor missing")
need('"video.fov_dolly", 0, 100, 10, 100' in rd("game/TuningPanel.h"), "panel row missing or default not 100")
need("SpeedMotes_Step(" not in rd("game/Race.cpp"), "motes must not be spawned")
need("video.speed_motes" not in rd("game/TuningPanel.h"), "mote rows must be gone from the panel")
need("FOV dolly" in rd("game/RallyData.cpp") and "View_GetFovDolly" in rd("game/RallyData.cpp"), "overlay line missing")
print("fov dolly wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
