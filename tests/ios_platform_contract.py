"""Source/build contract only. Does not prove an Apple build or device run."""
import json
import pathlib
import plistlib
import unittest

root = pathlib.Path(__file__).resolve().parents[1]

class IOSContract(unittest.TestCase):
    def test_ios_requested_text_keeps_software_keyboard_enabled(self):
        source = (root/'src/platform/sys.cpp').read_text()
        startup = source.split('#elif defined(TARGET_OS_IOS) && TARGET_OS_IOS', 1)[1].split('#endif', 1)[0]
        self.assertNotIn('SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0"', startup)
        offered = source.split('} else if (s_entryOffered) {', 1)[1].split('} else {', 1)[0]
        self.assertIn('SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0"', offered)
        self.assertIn('SDL_ResetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD)', offered)

    def test_bundle_identifier_is_configurable_and_shared_with_plist(self):
        source = (root/'src/CMakeLists.txt').read_text()
        self.assertIn('set(OPENCMR2_IOS_BUNDLE_IDENTIFIER "io.github.pablocpas.OpenCMR2" CACHE STRING', source)
        self.assertIn('MACOSX_BUNDLE_GUI_IDENTIFIER "${OPENCMR2_IOS_BUNDLE_IDENTIFIER}"', source)
        self.assertIn('XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${OPENCMR2_IOS_BUNDLE_IDENTIFIER}"', source)
        plist = plistlib.loads((root/'res/Info-iOS.plist').read_bytes())
        self.assertEqual(plist['CFBundleIdentifier'], '${MACOSX_BUNDLE_GUI_IDENTIFIER}')

    def test_presets_distinguish_simulator_device_and_local_signing(self):
        p = {v['name']: v for v in json.loads((root/'CMakePresets.json').read_text())['configurePresets']}
        self.assertEqual(p['ios-sim']['cacheVariables']['CMAKE_OSX_SYSROOT'], 'iphonesimulator')
        self.assertEqual(p['ios-device']['cacheVariables']['CMAKE_OSX_SYSROOT'], 'iphoneos')
        self.assertEqual(p['ios-device']['inherits'], 'ios-sim')
        self.assertEqual(p['ios-sim']['cacheVariables']['OPENCMR2_METALFX'], 'OFF')
        self.assertEqual(p['ios-device-xcode']['generator'], 'Xcode')
        self.assertEqual(p['ios-device-xcode']['cacheVariables']['OPENCMR2_IOS_SIGNING'], 'ON')

    def test_bundle_is_universal_landscape_and_exposes_owned_data(self):
        p = plistlib.loads((root/'res/Info-iOS.plist').read_bytes())
        self.assertEqual(p['UIDeviceFamily'], [1, 2])
        self.assertTrue(p['UIFileSharingEnabled'])
        self.assertTrue(p['LSSupportsOpeningDocumentsInPlace'])
        self.assertNotIn('$(', p['CFBundleExecutable'])
        self.assertTrue(all('Landscape' in s for s in p['UISupportedInterfaceOrientations']))

    def test_ios_disables_desktop_helpers_and_uses_documents(self):
        src = (root/'src/platform/sys.cpp').read_text()
        self.assertIn('SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS)', src)
        command = src.split('BOOL Sys_CommandExists', 1)[1].split('int Sys_RunAndWait', 1)[0]
        run = src.split('int Sys_RunAndWait', 1)[1].split('void Sys_RequestQuit', 1)[0]
        self.assertIn('TARGET_OS_IOS', command)
        self.assertIn('return FALSE;', command)
        self.assertIn('TARGET_OS_IOS', run)
        self.assertIn('return -1;', run)
        self.assertIn('NOT IOS', (root/'src/CMakeLists.txt').read_text())

if __name__ == '__main__':
    unittest.main()
