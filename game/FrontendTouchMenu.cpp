// PORT: touch controls settings submenu, opened from a literal row at the
// bottom of the main Options menu. Writes the int form of the input.touch_*
// settings (LoadTouchConfig accepts both ints and the original strings) and
// asks the input layer to re-read them. Labels are provisional English
// literals: localization lands with the settled touch design, like the rest
// of the placeholder visuals.
#include "Menu.h"
#include "port/sys.h"
#include "port/input.h"

#include <stdio.h>
#include <string.h>

Menu g_menuTouchControls;

static char s_touchMenuTitle[] = "Touch controls";
static char s_touchMenuRow[] = "Touch controls...";
static char s_modeRow[48];
static char s_handRow[48];
static char s_padsRow[48];
static char s_barRow[48];

extern Menu g_menu0x0081f118; // main Options menu
extern void FrontendMenu_ReturnToParent(Menu *pMenu, int param);

static const char *TouchModeText(int v) { return v == 1 ? "classic" : "floating"; }
static const char *TouchHandText(int v) { return v == 1 ? "left" : "right"; }
static const char *TouchPadsText(int v) { return v == 1 ? "on" : v == 2 ? "off" : "auto"; }
static const char *TouchBarText(int v) { return v == 1 ? "hide" : "clamp"; }

static void FrontendMenu_RefreshTouchRows(void)
{
    sprintf(s_modeRow, "Pad style: %s", TouchModeText(Sys_GetOption("input.touch_mode", 0)));
    sprintf(s_handRow, "Handedness: %s", TouchHandText(Sys_GetOption("input.touch_handed", 0)));
    sprintf(s_padsRow, "Touch controls: %s", TouchPadsText(Sys_GetOption("input.touch_controls", 0)));
    sprintf(s_barRow, "Pads in black bars: %s", TouchBarText(Sys_GetOption("input.touch_bar_policy", 0)));
}

static void FrontendMenu_CycleTouchSetting(Menu *, MenuItem *item, const char *key, int count)
{
    int value = (Sys_GetOption(key, 0) + 1) % count;
    Sys_SetOption(key, value);
    Input_ReloadTouchConfig();
    FrontendMenu_RefreshTouchRows();
    (void)item;
}

void FrontendMenu_CycleTouchMode(Menu *m, MenuItem *item) { FrontendMenu_CycleTouchSetting(m, item, "input.touch_mode", 2); }
void FrontendMenu_CycleTouchHand(Menu *m, MenuItem *item) { FrontendMenu_CycleTouchSetting(m, item, "input.touch_handed", 2); }
void FrontendMenu_CycleTouchPads(Menu *m, MenuItem *item) { FrontendMenu_CycleTouchSetting(m, item, "input.touch_controls", 3); }
void FrontendMenu_CycleTouchBar(Menu *m, MenuItem *item) { FrontendMenu_CycleTouchSetting(m, item, "input.touch_bar_policy", 2); }

void FrontendMenu_EnterTouchControls(Menu *, int)
{
    FrontendMenu_RefreshTouchRows();
}

void FrontendMenu_BuildTouchControls(void)
{
    Menu_Init(&g_menuTouchControls, s_touchMenuTitle, 0, 0, &g_menu0x0081f118, NULL, 1, 0, 1);
    Menu_AddItemType1(&g_menuTouchControls, s_modeRow, -1, (MenuItemAction)FrontendMenu_CycleTouchMode, -1);
    Menu_AddItemType1(&g_menuTouchControls, s_handRow, -1, (MenuItemAction)FrontendMenu_CycleTouchHand, -1);
    Menu_AddItemType1(&g_menuTouchControls, s_padsRow, -1, (MenuItemAction)FrontendMenu_CycleTouchPads, -1);
    Menu_AddItemType1(&g_menuTouchControls, s_barRow, -1, (MenuItemAction)FrontendMenu_CycleTouchBar, -1);
    Menu_AddItemType1(&g_menuTouchControls, 0, 0x67, (MenuItemAction)FrontendMenu_ReturnToParent, -1);
    Menu_SetCallbacks(&g_menuTouchControls, (MenuCallback)FrontendMenu_EnterTouchControls, NULL, NULL, NULL);
    Menu_ValidateCursor(&g_menuTouchControls, 0);
    FrontendMenu_RefreshTouchRows();
}

// The Options-menu row: literal text like the What's new row above it.
char *FrontendMenu_TouchControlsText(void)
{
    return s_touchMenuRow;
}
