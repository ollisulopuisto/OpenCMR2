#include "port/types.h"
#include "Input.h"
#include "Menu.h"
#include <cstdio>
#include <cstring>
extern void FrontendMenu_BuildControls(),FrontendMenu_BuildDeviceConfiguration(),FrontendMenu_BuildDeviceBindings();
extern Menu *FrontendMenu_GetControls(),*FrontendMenu_GetDeviceConfiguration(),*FrontendMenu_GetDeviceBindings();
extern Menu *Menu_Update(Menu*,unsigned);
extern void Menu_QueueDefaultAction();
extern ControllerData g_controlsCopy[6];
extern int g_controlsSelectedSlot;
struct ControlsDeviceSelection{unsigned short slots[8],lastDevice;};extern ControlsDeviceSelection g_controlsDeviceSelection;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 FrontendMenu_BuildControls();FrontendMenu_BuildDeviceConfiguration();FrontendMenu_BuildDeviceBindings();
 Menu *controls=FrontendMenu_GetControls(),*config=FrontendMenu_GetDeviceConfiguration(),*bindings=FrontendMenu_GetDeviceBindings();
 CHECK(controls->itemCount==4&&controls->items[0].pSubMenu==config&&controls->items[1].pSubMenu==config);
 CHECK(!strcmp(config->items[6].text,"Rebind driving controls")&&config->items[6].pSubMenu==bindings);
 CHECK(!strcmp(bindings->items[10].text,"Save bindings")&&bindings->items[10].pSubMenu==controls);
 // Real menu select follows the exposed link, not an invented UI shortcut.
 // Its device-poll callback depends on an active SDL window, so isolate just
 // construction+navigation here; capture actual function is tested separately.
 config->pfnCallback1=nullptr;config->pfnCallback3=nullptr;config->cursor=6;
 CHECK(Menu_Update(config,0x10)==bindings);
 Menu_SetActionLatch(0);Menu_QueueDefaultAction();
 // Actual enter+save callbacks on a synthetic keyboard preserve edited keys.
 CInput::m_availableDevices[0].field_0x0=1;g_controlsSelectedSlot=0;g_controlsDeviceSelection.slots[0]=0;
 bindings->pfnCallback0(bindings,0);for(int i=0;i<9;i++)CHECK(bindings->items[i].enabled&&bindings->items[i].visible);
 g_controlsCopy[0]=CInput::m_controllerInfo[0];g_controlsCopy[0].field_0x13e[2]=0x1e;
 bindings->cursor=10;CHECK(Menu_Update(bindings,0x10)==controls);CHECK(CInput::m_controllerInfo[0].field_0x13e[2]==0x1e);
 puts("driving menu actual construction/rebind select/save path pass; native pixels/window polling separate");return 0;
}
