#!/usr/bin/env python3
"""Wheel-thrown specks streak along their screen-space motion (user: "more
cloudlike, triangular? not separate blobs"). The draw hook must stay scoped
to the wheel dust/chip family (0x10..0x17) and opt-out via
video.dust_speck_streak; the accepted dust cloud (plume + volume) is untouched."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
g = (r / "game/Graphics.cpp").read_text()
h = (r / "game/DustPlume.h").read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need('Sys_GetOption("video.dust_speck_streak", 1) != 0' in g, "speck streak must be ini-gated, default on")
need("pType >= &g_particleTypes[0x10] && pType <= &g_particleTypes[0x17]" in g, "streak must cover exactly the wheel dust/chip family 0x10..0x17")
need("WheelSpeck_StreakParams(sRight, sUp, speckDepth, 100" in g, "streak must align to the speck's camera-basis motion")
need("WheelSpeck_StreakParams" in h, "the pure-maths helper must live in DustPlume.h next to the other particle helpers")
need("def.field_0x20 = (short)speckAngle" in g, "streak rotates the billboard to the motion direction")
print("wheel speck streak wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
