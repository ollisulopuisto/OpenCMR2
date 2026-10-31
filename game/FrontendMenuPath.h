#ifndef OPENCMR2_FRONTEND_MENU_PATH_H
#define OPENCMR2_FRONTEND_MENU_PATH_H

// PORT: the boot render state (Game_DrawBootCameraAndFPS) can draw the
// freshly-entered main menu before its first update ran; the letter/dot
// animation dereferences g_pMenuPath unconditionally (native launch segfault:
// EXC_BAD_ACCESS at 0x8 in FrontendAnimation_EvaluatePathSpline - NULL+8 -
// stack in the 2026-11-01-000237 .ips). FrontendMenu_EnterMain now guarantees
// a path through this helper; the update's own branches stay the sole writers
// afterwards, so menu morphs and the cheat path behave exactly as before.
inline void FrontendMenuPath_Ensure(int **ppPath, int *pInit, int *firstModePath) {
    if (*ppPath == 0) {
        *pInit = 1;
        *ppPath = firstModePath;
    }
}

#endif
