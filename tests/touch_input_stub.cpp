// Test stub for the small render tests that compile gfx_core.cpp directly.
// gfx_core.cpp's Present() calls TouchOverlay_RecordFrame, which queries the
// input layer. The game gets the real implementations from
// src/input/input_sdl.cpp; these no-op versions keep the overlay inactive,
// matching a desktop run without a touch device. Platform_GetSetting is NOT
// stubbed here: each of these test files already provides its own.
#include "port/input.h"

int Input_TouchOverlayActive(void) { return 0; }
void Input_ReloadTouchConfig(void) {}
int Input_GetTouchPads(TouchPadView *, int, int *, int *) { return 0; }
