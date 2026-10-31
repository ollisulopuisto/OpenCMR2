// Sys_GetProcessMemoryBytes: the diagnostics memory probe returns a sane
// resident size on platforms with a working query (Linux /proc, macOS
// task_info, Windows GetProcessMemoryInfo) and never crashes where the
// query is unavailable (0 allowed).
#include "port/sys.h"
#include <cstdio>
int main()
{
    unsigned long long a = Sys_GetProcessMemoryBytes();
    unsigned long long b = Sys_GetProcessMemoryBytes();
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    if (a == 0 || b == 0) { std::puts("FAIL: query returned 0 on a supported platform"); return 1; }
    // A live process holds at least a few hundred KB resident.
    if (a < 256 * 1024) { std::puts("FAIL: implausibly small resident size"); return 1; }
#endif
    std::printf("process memory probe: %llu bytes\n", a);
    return 0;
}
