#!/usr/bin/env python3
"""Source wiring for the opt-in debug object mask (video.debug_object_mask). Source-only: not a pixel or native check."""
import pathlib
root = pathlib.Path(__file__).resolve().parents[1]
gpu = (root / 'src/render/gfx_gpu.cpp').read_text()
frag = (root / 'src/render/shaders/fixed.frag').read_text()
game = (root / 'game/Game.cpp').read_text()
enh = (root / 'src/platform/enhancements.h').read_text()
core = (root / 'src/render/gfx_core.cpp').read_text()
# default off: the mask pass only runs with the option and a followed car
assert 'Sys_GetOption("video.debug_object_mask", 0) != 0' in gpu and 'playerObject != 0' in gpu
# the mask key can never blend or write depth, and always depth-tests LESS_EQUAL
key = gpu[gpu.index('static uint32_t MaskKey'):]
key = key[:key.index('}')]
assert '~(1u << 4)' in key and '~(1u << 14)' in key and '1u << 13' in key and 'GFX_CMP_LESSEQUAL' in key
# only untransformed back-buffer draws of the followed car are re-issued
assert 'frame.states[c.state].objectId != maskObject' in gpu and 'target != NULL || c.transformed' in gpu
# the original pass must not be touched: the temporal vote is skipped only while masking
assert '!maskDraw && target == NULL' in gpu
# shader flag lives in the previously unused alphaTest.z slot and runs after the alpha test
assert frag.index('alphaTest.z != 0') > frag.index('alphaTest.x != 0') and 'u.alphaTest[2] = maskDraw' in gpu
# player slot comes from the active camera record's car, +1 like objectId
assert 'Gfx_SetPlayerObject((unsigned)View_GetActiveCameraFlags((BYTE)bit) + 1)' in game
assert 'c.playerObject=s_playerObject' in core and '"debug_object_mask"' in enh
print('object mask wiring ok')
