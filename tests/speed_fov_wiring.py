#!/usr/bin/env python3
"""speed_fov must be applied (and eased) once per rendered view in View_SetPlayerProjection, not in the logic-step camera blend."""
import re, pathlib
root = pathlib.Path(__file__).resolve().parent.parent / "game"
car = (root / "Car.cpp").read_text(); fp = (root / "FixedPoint.cpp").read_text()
m = re.search(r"void View_SetPlayerProjection\(.*?\n}\n", fp, re.S); assert m
assert "View_SpeedFovZoom(i, g_viewZooms[i])" in m.group(0), "projection must apply speed fov"
b = re.search(r"void View_BlendCameraStates\(.*?\n}\n", car, re.S); assert b
assert "View_SpeedFovZoom" not in b.group(0), "camera blend must not apply speed fov (stale cadence)"
assert "Sys_GetTicksPrecise" in re.search(r"int View_SpeedFovZoom\(.*?\n}\n", car, re.S).group(0), "easing must use the real-time clock"
print("speed fov wiring ok")
