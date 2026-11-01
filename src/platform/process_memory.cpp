// Process resident memory in bytes on Windows (Sys_GetProcessMemoryBytes,
// used by the diagnostics overlay). Kept in its own target because the real
// <windows.h>/<psapi.h> clash with port/types.h, the same split as
// menu_shortcut.cpp. sys.cpp carries the Apple and Linux branches.
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>

extern "C" unsigned long long Sys_GetProcessMemoryBytes(void)
{
    PROCESS_MEMORY_COUNTERS_EX counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&counters, sizeof(counters)))
        return (unsigned long long)counters.WorkingSetSize;
    return 0;
}
#endif
