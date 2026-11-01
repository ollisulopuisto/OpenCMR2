#!/usr/bin/env python3
"""Body-paint dirt must bind on car-overlay/mod paints only by explicit
opt-in: video.body_dirt_extra names extra stems, and unbound car textures
are logged once each so a native log can feed it. Unknown names still stay
clean by default (PR #82 design), and the option text is read per bind so
the user can add a stem without touching the binary."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
h = (r / "game/CarBodyDirt.h").read_text()
g = (r / "game/Graphics.cpp").read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("IsPaintAtlas(const char *path,const char *extraStems)" in h, "IsPaintAtlas takes the extra-stem list")
need("IsExtraAtlas" in h and "kept<4" in h.replace(" ", ""), "extra list is bounded to 4 stems")
need('"affdbodf","amldbodf","ap2dbodf","ap5dbodf","ascdbodf","asidbodf"' in h, "original six atlas stems kept")
need('IsPaintAtlas(texture->name,Sys_GetOptionText("video.body_dirt_extra",""))' in g, "bind reads video.body_dirt_extra live")
need("body dirt: unbound car texture" in g, "unbound car textures are logged once each for the next native log")
print("body dirt mapping wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
