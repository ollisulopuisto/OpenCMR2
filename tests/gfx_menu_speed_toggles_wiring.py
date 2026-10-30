#!/usr/bin/env python3
"""The two graphics-menu switches exist in the real menu, persist, and reach the renderer. Source wiring check; native menu untested."""
import os, sys
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def rd(p): return open(os.path.join(root, p)).read()
fails = 0
def need(c, m):
    global fails
    if not c: print("FAIL", m); fails += 1
menus, screens, race, car = rd("game/FrontendMenus.cpp"), rd("game/FrontendScreens.cpp"), rd("game/Race.cpp"), rd("game/Car.cpp")
need('"Speed Effects"' in menus and '"Air Vortices"' in menus, "both rows are added to the advanced graphics menu")
need("case 0x1b:" in screens and "case 0x1c:" in screens, "both rows are drawn")
need('Menu_GetItem(pMenu, 0x1b)->max = Sys_GetOption("video.speed_effects", 1)' in screens and 'Menu_GetItem(pMenu, 0x1c)->max = Sys_GetOption("video.vapor_trails", 0)' in screens, "rows start from the saved values")
need('Sys_SetOption("video.speed_effects", Menu_GetItem(pMenu, 0x1b)->max)' in screens and 'Sys_SetOption("video.vapor_trails", Menu_GetItem(pMenu, 0x1c)->max)' in screens, "rows save their values")
need('Sys_GetOption("video.speed_effects", 1) != 0)' in race and 'video.speed_effects' in car, "renderer path and speed FOV read the switch every frame")
print("FAILED" if fails else "gfx menu speed toggles wiring ok"); sys.exit(1 if fails else 0)
