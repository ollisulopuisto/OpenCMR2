"""Asset/build contract, not actool or device acceptance."""
import json
from pathlib import Path
import struct
import unittest
import plistlib
import runpy
import tempfile
from unittest.mock import patch
root = Path(__file__).resolve().parents[1]
class AppIcon(unittest.TestCase):
    def test_sizes_and_no_alpha(self):
        catalog = root / 'res/Assets.xcassets/AppIcon.appiconset'
        data = json.loads((catalog / 'Contents.json').read_text())
        expected = {('ipad', str(s), str(k)) for s in (20,29,40,76) for k in (1,2)}
        expected |= {('ipad','83.5','2'), ('ios-marketing','1024','1')}
        actual = set()
        for image in data['images']:
            size = image['size'].split('x')[0]; scale = image['scale'].removesuffix('x')
            actual.add((image['idiom'], size, scale))
            raw = (catalog / image['filename']).read_bytes()
            self.assertEqual(raw[:8], b'\x89PNG\r\n\x1a\n')
            width,height,depth,colour = struct.unpack('>IIBB', raw[16:26])
            self.assertEqual((width,height), (round(float(size)*int(scale)),)*2)
            self.assertEqual((depth,colour), (8,2)) # RGB, no alpha channel
            at = 8
            while at < len(raw):
                length = struct.unpack('>I',raw[at:at+4])[0]
                self.assertNotEqual(raw[at+4:at+8], b'tRNS')
                at += 12+length
        self.assertTrue(expected <= actual)
    def test_unsigned_metadata_merge_preserves_bundle_identity(self):
        with tempfile.TemporaryDirectory() as temporary:
            bundle = Path(temporary)
            info = bundle / 'Info.plist'
            info.write_bytes(plistlib.dumps({'CFBundleIdentifier': 'existing.id', 'UIFileSharingEnabled': True}))
            def actool(command, check):
                self.assertIn('AppIcon', command)
                self.assertIn('iphonesimulator', command)
                partial = Path(command[command.index('--output-partial-info-plist') + 1])
                partial.write_bytes(plistlib.dumps({'CFBundleIcons': {'CFBundlePrimaryIcon': {'CFBundleIconName': 'AppIcon'}}}))
            argv = ['compile_ios_appicon.py', '--catalog', 'catalog', '--bundle', str(bundle),
                    '--platform', 'iphonesimulator', '--minimum', '13.0']
            with patch('sys.argv', argv), patch('subprocess.run', side_effect=actool):
                runpy.run_path(str(root/'tools/compile_ios_appicon.py'), run_name='__main__')
            data = plistlib.loads(info.read_bytes())
            self.assertEqual(data['CFBundleIdentifier'], 'existing.id')
            self.assertTrue(data['UIFileSharingEnabled'])
            self.assertEqual(data['CFBundleIcons']['CFBundlePrimaryIcon']['CFBundleIconName'], 'AppIcon')

    def test_build_routes(self):
        source = (root/'src/CMakeLists.txt').read_text()
        self.assertIn('XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon', source)
        self.assertIn('compile_ios_appicon.py', source)
        self.assertIn('--bundle $<TARGET_BUNDLE_DIR:opencmr2>', source)
if __name__ == '__main__': unittest.main()
