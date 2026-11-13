"""Check the Win32 SDK boundary without requiring a Windows SDK on the host."""
import os
import shutil
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class WindowsCommandLookup(unittest.TestCase):
    def test_sdk_call_is_isolated_from_game_compat_headers(self):
        source = (ROOT / 'src/platform/sys.cpp').read_text()
        self.assertFalse('SearchPathA(' in source, 'sys.cpp must not call undeclared Win32 APIs')
        self.assertIn('Windows_CommandExists(name)', source)
        cmake = (ROOT / 'src/CMakeLists.txt').read_text()
        self.assertIn('add_library(opencmr2_command_lookup STATIC platform/command_lookup_windows.cpp)', cmake)
        self.assertIn('PUBLIC opencmr2_command_lookup', cmake)

    @unittest.skipUnless(os.name == "posix" and shutil.which("g++"), "host probe needs POSIX g++")
    def test_production_helper_uses_sdk_declaration_and_handles_missing_commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            # Deliberately no declaration in the production header: the helper
            # must include the SDK header, not obtain it through game headers.
            (tmp / 'windows.h').write_text('''#include <cstddef>
using DWORD = unsigned long;
#define MAX_PATH 260
extern "C" DWORD SearchPathA(const char *, const char *, const char *, DWORD, char *, char **);
''')
            (tmp / 'probe.cpp').write_text('''#include <cassert>
#include <cstring>
#include "windows.h"
#include "command_lookup_windows.h"
static int calls;
static DWORD result;
extern "C" DWORD SearchPathA(const char *path, const char *name, const char *ext, DWORD size, char *out, char **part) {
    ++calls;
    assert(path == nullptr && std::strcmp(name, "ffmpeg") == 0);
    assert(std::strcmp(ext, ".exe") == 0);
    assert(size == MAX_PATH && out != nullptr && part == nullptr);
    return result;
}
int main() {
    assert(!Windows_CommandExists(nullptr));
    assert(!Windows_CommandExists(""));
    assert(calls == 0);
    result = 0; assert(!Windows_CommandExists("ffmpeg"));
    result = 42; assert(Windows_CommandExists("ffmpeg"));
    result = 1024; assert(Windows_CommandExists("ffmpeg"));
    assert(calls == 3);
}
''')
            subprocess.run(['g++', '-std=c++17', '-D_WIN32', '-I' + str(tmp),
                            '-I' + str(ROOT / 'src/platform'),
                            str(ROOT / 'src/platform/command_lookup_windows.cpp'),
                            str(tmp / 'probe.cpp'), '-o', str(tmp / 'probe')], check=True)
            subprocess.run([str(tmp / 'probe')], check=True)


if __name__ == '__main__':
    unittest.main()
