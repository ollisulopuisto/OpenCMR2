#!/usr/bin/env python3
"""Source wiring: the mote 'queued' counter only counts quads Billboard_Add accepted; refusals are counted by cause."""
import pathlib, re
g = (pathlib.Path(__file__).resolve().parents[1] / 'game/Graphics.cpp').read_text()
add = g[g.index('void Billboard_Add(BillboardDef'):g.index('// Release callback: empties the billboard queue')]
assert 'g_billboardRefusedDisabled' in add and 'g_billboardRefusedFull' in add, 'refusal causes must be counted in Billboard_Add'
loop = g[g.index('const int queuedBefore = g_unk0x006dd784;'):g.index('// Camera-facing cloud sheets are fallback only')]
assert re.search(r'Billboard_Add\(&def, texture\);\s*if \(g_particleTypeCount > SPEED_MOTE_TYPE .*?\{\s*if \(g_unk0x006dd784 > queuedBefore\) \+\+g_speedMotesDrawn; else \+\+g_speedMotesRefused;', loop, re.S), 'queued counter must follow acceptance'
assert 'g_billboardPeakQueue' in g[g.index('void Billboard_Draw(SceneNode'):g.index('// Takes a free texture slot')]
assert 'billboards_submitted' not in g, 'old label overstated: queued is not drawn/visible'
assert 'Gfx_GetBlendDepthState' in g and 'billboard draw state:' in g
print('billboard queue wiring ok')
