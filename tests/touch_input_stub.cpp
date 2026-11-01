// Test stub for the small render tests that compile gfx_core.cpp directly.
// gfx_core.cpp's Present() calls TouchOverlay_RecordFrame, which queries the
// input and platform layers. The game gets the real implementations from
// src/input/input_sdl.cpp and src/platform/sys.cpp; these no-op versions keep
// the overlay inactive, matching a desktop run without a touch device.
#include "port/input.h"
#include "platform/platform.h"

int Input_TouchOverlayActive(void) { return 0; }
void Input_ReloadTouchConfig(void) {}
int Input_GetTouchPads(TouchPadView *, int, int *, int *) { return 0; }

const char *Platform_GetSetting(const char *, const char *fallback) { return fallback; }
