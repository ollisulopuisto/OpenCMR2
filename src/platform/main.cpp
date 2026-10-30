// Process entry point: sets up the platform and runs the game's main loop
// (CMain::Initialize, which was WinMain's body).

#include "platform/platform.h"
#include "port/sys.h"
#include "diagnostics/timing.h"
#include "main.h"
#include "platform/enhancements.h"
#include "platform/key_remapping.h"
#include <cstring>
#include <cstdio>
#include "build_identity.h"

// On Android, SDLActivity calls SDL_main, which SDL_main.h renames main to.
#ifndef __ANDROID__
#define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL_main.h>

int main(int argc, char **argv)
{
    std::printf("OpenCMR2 build: %s\n", OPENCMR2_BUILD_ID);
    for(int i=1;i<argc;i++)if(!std::strcmp(argv[i],"--version"))return 0;
    for(int i=1;i<argc;i++)if(!std::strcmp(argv[i],"--help")||!std::strcmp(argv[i],"--help-enhancements")){std::puts(EnhancementHelp());std::puts(KeyRemappingHelp());return 0;}
    for(int i=1;i<argc;i++)if(!std::strcmp(argv[i],"--help-controls")){std::puts(KeyRemappingHelp());return 0;}
    SDL_SetMainReady();
    if (!Platform_Init(argc, argv))
        return Platform_WasSetupCanceled() ? 0 : 1;
    for(int i=1;i<argc;i++)if(!std::strcmp(argv[i],"--prepare-only")){Platform_Shutdown();return 0;}
    Diagnostics::StartProfileFromEnvironment();
    unsigned int code = CMain::Initialize(Sys_GetCommandLine());
    Diagnostics::StopProfile();
    Platform_Shutdown();
    return (int)code;
}
