#!/usr/bin/env python3
"""Menu row regressions reported natively: the What's new Options row must be
type 2 (a type-1 select's back path clobbers the screen transition with the
parent, so the screen never opened and Options ejected the user), and the
quit confirmation must be vertical (horizontal layout read Up/Down as value
adjustment, trapping the cursor on No)."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
src = (r / "game/FrontendMenus.cpp").read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("Menu_AddItemType2(&g_menu0x0081f118, FrontendMenu_WhatsNewText(), -1, FrontendMenu_GetWhatsNew()," in src,
     "What's new Options row must be type 2 with the screen as submenu")
need("Menu_AddItemType1(&g_menu0x0081f118, FrontendMenu_WhatsNewText()" not in src,
     "What's new row as type 1 leaves the screen unreachable (back path clobbers the transition)")
need("Menu_Init(&g_menu0x00823df8, 0, 3, 0, &g_menu0x0081d6d8, NULL, 1, 1, 0);" in src,
     "quit confirmation must be vertical (layout 0) so Up/Down move the cursor")
print("menu rows wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
