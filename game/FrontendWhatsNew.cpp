// PORT: "What's new" changelog screen, in its own unit so the runtime test
// can link and pixel-dump the real draw and transitions. Content lives in
// game/WhatsNew.h: bump kWhatsNewRevision and add lines when user-visible
// changes ship. The screen appears once per revision on the main menu
// (ui.whatsnew_seen) and stays reachable from the Screen options page.
#include "port/types.h"
#include "Menu.h"
#include "Graphics.h"
#include "Frontend.h"
#include "FrontendDraw.h"
#include "FrontendMenus.h"
#include "Font.h"
#include "WhatsNew.h"
#include "port/sys.h"

extern Menu *FrontendMenu_GetActiveMenu(void);

Menu g_menuWhatsNew;
Menu *g_whatsNewReturn;
static int g_whatsNewSeenAtOpen;
static bool g_whatsNewShowAll;

void FrontendMenu_EnterWhatsNew(Menu *m, int)
{
    if (g_whatsNewReturn != NULL)
        m->pParent = g_whatsNewReturn;
    g_whatsNewSeenAtOpen = Sys_GetOption("ui.whatsnew_seen", 0);
}

// Seen is recorded on dismissal, not when the screen is queued: notes the
// user never saw are never marked acknowledged. Re-show loops are still
// impossible - the dismissal marks the revision before the main menu's
// enter callback can queue the screen again.
void FrontendMenu_ExitWhatsNew(Menu *, int)
{
    Sys_SetOption("ui.whatsnew_seen", kWhatsNewRevision);
}

void FrontendMenu_DrawWhatsNew(Menu *)
{
    int n;
    const WhatsNewLine *lines = WhatsNew_Lines(n);
    int x = (int)(g_pGraphics->resX * 60) / 640;
    int y = (int)(g_pGraphics->resY * 60) / 480;
    Font_DrawText(2, (char *)"What's new in OpenCMR2", x, y, (int *)g_colourWhite0x00524968, 0x11);
    y += (int)(g_pGraphics->resY * 36) / 480;
    for (int i = 0; i < n; i++) {
        if (!g_whatsNewShowAll && lines[i].revision <= g_whatsNewSeenAtOpen)
            continue;   // auto-show after an upgrade lists only what is new
        Font_DrawText(1, (char *)lines[i].text, x, y, (int *)g_colourText0x0052496c, 0x11);
        y += (int)(g_pGraphics->resY * 24) / 480;
    }
    Font_DrawText(1, CFrontend::GetTextString(0x67), x, (int)(g_pGraphics->resY * 420) / 480,
                  (int *)g_colourWhite0x00524968, 0x11);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

Menu *FrontendMenu_GetWhatsNew(void)
{
    Menu_Init(&g_menuWhatsNew, (char *)"What's new", -1, 0, FrontendMenu_GetMain(), NULL, 1, 0, 1);
    Menu_AddItemType1(&g_menuWhatsNew, 0, 0x67, NULL, 0xe);   // type 1 with no action takes the back path
    Menu_SetCallbacks(&g_menuWhatsNew, FrontendMenu_EnterWhatsNew, NULL, (MenuCallback)FrontendMenu_DrawWhatsNew,
                      FrontendMenu_ExitWhatsNew);
    Menu_ValidateCursor(&g_menuWhatsNew, 0);
    return &g_menuWhatsNew;
}

void FrontendMenu_OpenWhatsNew(Menu *, MenuItem *)
{
    g_whatsNewReturn = FrontendMenu_GetActiveMenu();
    g_whatsNewShowAll = true;
    Menu_SetNextAction(FrontendMenu_GetWhatsNew());
}

void FrontendMenu_BuildWhatsNew(void)
{
    (void)FrontendMenu_GetWhatsNew();
}

// Auto-show once per changelog revision when the main menu is entered; the
// Screen page row reopens it any time.
void WhatsNew_MaybeAutoShow(void)
{
    int seen = Sys_GetOption("ui.whatsnew_seen", 0);
    if (seen >= kWhatsNewRevision)
        return;
    g_whatsNewReturn = NULL;   // keep the built parent: the main menu
    g_whatsNewShowAll = false;
    Menu_SetNextAction(FrontendMenu_GetWhatsNew());
}
