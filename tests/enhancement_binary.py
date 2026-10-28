"""Run actual binary entry/platform/helper dispatch in an isolated synthetic release."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

BINARY = Path(sys.argv.pop(1)).resolve()
OVERLAY = len(sys.argv)>1 and sys.argv[1]=='overlay'
if OVERLAY: sys.argv.pop(1)


class BinaryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='cmr cli spaces ')
        self.root = Path(self.tmp.name)
        self.exe = self.root / BINARY.name
        shutil.copy2(BINARY, self.exe)
        # Build rpath uses adjacent SDL; preserve that source's existing dependencies.
        for lib in BINARY.parent.glob('*SDL*'):
            if lib.is_file():
                shutil.copy2(lib, self.root / lib.name)
        self.tools = self.root / 'enhancements/scenery-patch'
        self.tools.mkdir(parents=True)
        self.game = self.root / 'owned data'
        self.profile = self.root / 'profile'
        self.profile.mkdir()
        (self.profile / 'opencmr2.ini').write_text('[video]\ndriver_name_overrides=0\n')
        (self.profile / 'progress.pps').write_bytes(b'valuable profile')
        (self.profile / 'pps').mkdir()
        (self.profile / 'pps/progress.pps').write_bytes(b'valuable profile')
        for file in ['Game/Bigfiles/common.bfl', 'FrontEnd/common.bfl', 'Sounds/surface.bfl', 'CountrySpecific/Europe/englishtext.bfl', 'SetupRep/640/common.bfl']:
            path = self.game / file
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'original')
        self.env = dict(os.environ, SDL_VIDEODRIVER='dummy', OPENCMR2_HEADLESS='1')
        self.env.pop('OPENCMR2_DATA', None)

    def tearDown(self):
        self.assertEqual((self.profile / 'progress.pps').read_bytes(), b'valuable profile')
        self.assertEqual((self.profile / 'opencmr2.ini').read_text(), '[video]\ndriver_name_overrides=0\n')
        self.tmp.cleanup()

    def run_binary(self, *args):
        return subprocess.run([str(self.exe), '--data', str(self.game), '--user-dir', str(self.profile), '--prepare-only', *args], env=self.env, capture_output=True, text=True, timeout=30)

    def helper(self, fail=False):
        (self.tools / 'prepare.py').write_text('''import argparse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--input-dir');p.add_argument('--cache-dir');p.add_argument('--mode');p.add_argument('--cars');p.add_argument('--mods-dir');a=p.parse_args()
''' + ("raise SystemExit(7)\n" if fail else '''root=Path(a.cache_dir);out=root/('cars-test' if a.cars else 'scenery-test');out.mkdir(parents=True,exist_ok=True)
(out/'manifest.tsv').write_text('synthetic')
(out/'sub.cin').write_text('replacement car')
print(('OPENCMR2_CARS=' if a.cars else 'OPENCMR2_PACK=')+str(out))
'''))

    @unittest.skipIf(OVERLAY, 'main entry covered by binary tests')
    def test_help_no_profile(self):
        r = subprocess.run([str(self.exe), '--help'], capture_output=True, text=True, env=self.env)
        self.assertEqual(r.returncode, 0)
        self.assertIn('--cars LIST', r.stdout)
        self.assertIn('--trees original|shell|crossed', r.stdout)
        self.assertFalse((self.profile / 'opencmr2.log').exists())

    @unittest.skipIf(OVERLAY, "main entry covered by binary tests")
    def test_dispatch_repeat_and_no_shell_expansion(self):
        self.helper()
        for _ in range(2):
            r = self.run_binary('--trees', 'crossed', '--spectators', 'animated', '--driver-names', 'real')
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertIn('scenery-test', r.stdout)
        r = self.run_binary('--cars', 'one', '--mods-dir', 'not a shell; $(touch bad)')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertFalse((self.root / 'bad').exists())

    @unittest.skipIf(OVERLAY, "main entry covered by binary tests")
    def test_failure_no_launch(self):
        self.helper(fail=True)
        r = self.run_binary('--trees', 'shell')
        self.assertNotEqual(r.returncode, 0)
        self.assertIn('exited 7', r.stderr)

    @unittest.skipIf(OVERLAY, "main entry covered by binary tests")
    def test_invalid_and_conflicting_args(self):
        for args in [('--trees', 'invalid'), ('--cars', 'one'), ('--trees', 'shell', '--scenery-dir', str(self.root))]:
            self.assertNotEqual(self.run_binary(*args).returncode, 0)

    @unittest.skipUnless(OVERLAY, 'separate platform IO probe')
    def test_actual_overlay_save_and_write_isolation(self):
        self.helper()
        r = self.run_binary('--cars', 'synthetic', '--mods-dir', 'owned zips')
        self.assertEqual(r.returncode, 0, r.stderr+r.stdout)
        self.assertEqual((self.profile/'enhancements/cars-test/sub.cin').read_text(), 'replacement car')
        self.assertEqual((self.profile/'Game/Cars/sub.cin').read_text(), 'user write')
        self.assertEqual((self.profile/'pps/progress.pps').read_bytes(), b'valuable profile')


if __name__ == '__main__':
    unittest.main()
