#include "port/types.h"
#include "Graphics.h"
#include "Game.h"
#include "Frontend.h"
#include "main.h"
#include <cstdio>
#include <cstdlib>
static int videoCalls,resourceCalls;
extern "C" BOOL __wrap_Gfx_SetVideoMode(int,int,BOOL){videoCalls++;return FALSE;}
extern "C" void __wrap__Z25Scene_InitFixedMathTablesv(){resourceCalls++;fprintf(stderr,"ERROR: resource initialization after failed GPU creation\n");std::exit(86);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CGame::m_shouldExit=FALSE;CMain::m_exitCode=0;
 // Actual frontend loader, with only the graphics backend's video-mode call
 // failing. No math/sprite/language/texture setup may follow failed creation.
 CHECK(!CFrontend::LoadSplashScreens(true));
 CHECK(videoCalls==1&&resourceCalls==0&&CGame::m_shouldExit&&CMain::m_exitCode==1);
 puts("GPU failure stops frontend resources and returns failure: pass");
}
