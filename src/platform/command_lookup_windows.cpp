// The game replaces <windows.h> and DWORD/LONG with compatibility types.
// Compile SDK calls in a separate target without the game include paths.
#ifdef _WIN32
#include <windows.h>
#include "command_lookup_windows.h"

bool Windows_CommandExists(const char *name)
{
    if (name == nullptr || *name == '\0')
        return false;
    char path[MAX_PATH];
    return SearchPathA(nullptr, name, ".exe", MAX_PATH, path, nullptr) != 0;
}
#endif
