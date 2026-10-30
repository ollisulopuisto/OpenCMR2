#!/usr/bin/env python3
"""The F10 panel and the camera readout must not draw on sprite layer 4: Game_DrawSceneViewport draws layer 4 before the 3D scene (background layer), so the scene hides it."""
import re, pathlib
root = pathlib.Path(__file__).resolve().parent.parent
g = (root / "game" / "Game.cpp").read_text()
m = re.search(r"Tri2D_DrawLayer\(4\);\s*Sprite_DrawLayer\(4\);", g)
assert m, "layer 4 draw call not found"
for n in (3, 2, 1):
    assert g.index("Sprite_DrawLayer(%d);" % n) > m.end(), "layer %d must be drawn after layer 4 (after the scene)" % n
r = (root / "game" / "RallyData.cpp").read_text()
assert re.search(r"kTuneOverlayLayer\s*=\s*3;", r), "overlay layer constant missing or not 3"
a = r.index("void TuningPanel_Draw(void)"); b = r.index("// Returns 1 when the wiper")
panel = r[a:b]
assert "kTuneOverlayLayer" in panel and not re.search(r"(Font_DrawTextScaled|Sprite_FillRect)\([^;]*,\s*4\);", panel), "panel draws on layer 4"
c = r.index("Sprite_FillRect(NULL, bp, bbg"); readout = r[c - 600:c + 1200]
assert not re.search(r"(Font_DrawTextScaled|Sprite_FillRect)\([^;]*,\s*4\);", readout), "camera readout draws on layer 4"
assert "kTuneOverlayLayer" in readout


# The pause wash/menu layer must finish before the dedicated overlay queue is populated.
late=g.index("    DebugOverlays_DrawLate();")
assert late>g.index("    Sprite_DrawLayer(1);",g.index("Gfx_EndWorldEffects();"))
assert g.index("Tri2D_DrawLayer(3);Sprite_DrawLayer(3);",late)>late
assert r.count("    TuningPanel_Draw();")==1

print("background/late overlay layer wiring ok")
