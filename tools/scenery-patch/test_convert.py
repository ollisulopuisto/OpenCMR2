# SPDX-License-Identifier: GPL-3.0-or-later
"""No original game artwork. All DDS/archives are generated synthetic fixtures."""
import gzip, hashlib, importlib.util, io, json, shutil, struct, subprocess, sys
import tempfile, unittest
from pathlib import Path
from PIL import Image, ImageDraw

HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location('patcher', HERE / 'convert.py')
patcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patcher)


def dds():
    image = Image.new('RGBA', (32, 32))
    draw = ImageDraw.Draw(image)
    draw.ellipse((4, 2, 28, 22), fill=(40, 160, 60, 255))
    draw.rectangle((14, 18, 18, 31), fill=(100, 70, 40, 255))
    data = io.BytesIO()
    image.save(data, format='DDS')
    return data.getvalue()


def bfl(entries):
    body = bytearray()
    directory = bytearray()
    for leaf, raw in entries:
        offset = len(body)
        body.extend(raw)
        name = leaf.encode('ascii')
        directory.extend(struct.pack('<III', len(raw), offset, len(name)))
        directory.extend(name)
        directory.extend(bytes((-len(name)) % 4))
    return gzip.compress(b'CMPR' + bytes(4) + body + directory + struct.pack('<I', len(body)))


class Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)
        self.input = self.base / 'game'
        self.input.mkdir()
        self.script = self.base / 'convert.py'
        shutil.copy(HERE / 'convert.py', self.script)
        self.output = self.base / 'private' / 'scenery-pack'
        self.raw = dds()
        self.index = {}
        categories = ['individual-tree', 'spectator', 'forest-strip', 'shadow-mask']
        for name, category in zip(['frahit', 'fralot', 'finhit', 'finlot'], categories):
            leaf = ('tre_md1' if category=='individual-tree' else name) + '.dds'
            (self.input / (name + '.bfl')).write_bytes(bfl([(leaf, self.raw)]))
            self.index[name + '.bfl/' + leaf] = {
                'sha256': hashlib.sha256(self.raw).hexdigest(),
                'width': 32, 'height': 32,
                'category': category, 'kind': 'tree' if category != 'spectator' else 'spectator'
            }
        self.write_index()
        self.before = {p.name: p.read_bytes() for p in self.input.iterdir()}

    def tearDown(self):
        self.tmp.cleanup()

    def write_index(self):
        self.script.with_name('source-index.json').write_text(json.dumps(self.index))

    def run_patch(self, *args, ok=True):
        current = {str(p.relative_to(self.input)): p.read_bytes() for p in self.input.rglob('*') if p.is_file()}
        run = subprocess.run([sys.executable, str(self.script), '--input-dir', str(self.input),
                              '--output-dir', str(self.output), *args], capture_output=True, text=True)
        self.assertEqual(run.returncode == 0, ok, run.stdout + run.stderr)
        self.assertEqual(current, {str(p.relative_to(self.input)): p.read_bytes() for p in self.input.rglob('*') if p.is_file()})
        return run

    def test_full_output_and_feet(self):
        self.run_patch()
        rows = self.output.joinpath('manifest.tsv').read_text().splitlines()
        self.assertEqual(len(rows), 3)
        people = sorted(self.output.glob('spectators/*/*/*.tga'))
        self.assertEqual(len(people), 12)
        first = Image.open(people[0]).convert('RGBA')
        for frame in people:
            image = Image.open(frame).convert('RGBA')
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(image.crop((0, 45, 64, 64)).tobytes(), first.crop((0, 45, 64, 64)).tobytes())
        objs=list(self.output.glob('trees/*/*/*.obj'));self.assertTrue(objs)
        self.assertTrue(objs[0].read_text().startswith('# opencmr2-source-canvas-1 32 32\n'))
        self.assertTrue('source-fitted-interpolated' in self.output.joinpath('coverage.json').read_text())
        self.assertFalse(list(self.output.rglob('*.dds')))
        self.assertFalse(list(self.output.rglob('*.bfl')))
        self.run_patch(ok=False)

    def test_tree_generation_is_deterministic(self):
        self.run_patch();before={str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()}
        shutil.rmtree(self.output);self.run_patch()
        self.assertEqual(before,{str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()})

    def test_unsupported_tree_stays_original(self):
        key=next(iter(self.index));rec=self.index.pop(key)
        key='frahit.bfl/tre_fr1.dds';self.index[key]=rec;self.write_index()
        (self.input/'frahit.bfl').write_bytes(bfl([('tre_fr1.dds',self.raw)]))
        self.run_patch();self.assertFalse(list(self.output.glob('trees/*/*/*.obj')))
        self.assertEqual(len(self.output.joinpath('manifest.tsv').read_text().splitlines()),2)
        self.assertTrue('retained-original' in self.output.joinpath('coverage.json').read_text())

    def test_check_only(self):
        self.run_patch('--check-only')
        self.assertFalse(self.output.exists())

    def test_missing_duplicate_and_symlink(self):
        one = self.input / 'frahit.bfl'
        one.unlink()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        one.write_bytes(bfl([('tre_md1.dds', self.raw)]))
        nested = self.input / 'nested'
        nested.mkdir()
        shutil.copy(one, nested / one.name)
        self.run_patch(ok=False)
        (nested / one.name).unlink()
        one.unlink()
        one.symlink_to(self.base / 'outside.bfl')
        (self.base / 'outside.bfl').write_bytes(bfl([('tre_md1.dds', self.raw)]))
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())

    def test_wrong_hash_and_dimensions(self):
        key = next(iter(self.index))
        self.index[key]['sha256'] = '0' * 64
        self.write_index()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        self.index[key]['sha256'] = hashlib.sha256(self.raw).hexdigest()
        self.index[key]['width'] = 99
        self.write_index()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())

    def test_inside_and_nonempty_output(self):
        self.output = self.input / 'pack'
        self.run_patch(ok=False)
        self.output = self.base / 'pack'
        self.output.mkdir()
        self.output.joinpath('mine').write_text('retain')
        self.run_patch(ok=False)
        self.assertEqual(self.output.joinpath('mine').read_text(), 'retain')

    def test_corrupt_archive(self):
        for raw in [b'not gzip', gzip.compress(b'bad'),
                    gzip.compress(b'CMPR' + bytes(4) + struct.pack('<I', 99999)),
                    bfl([('../x', self.raw)]), bfl([('frahit.dds', self.raw), ('frahit.dds', self.raw)])]:
            path = self.base / 'broken.bfl'
            path.write_bytes(raw)
            with self.assertRaises(Exception):
                patcher.read_archive(path, 'frahit.bfl', self.index)

    def test_generation_failure_leaves_no_partial_output(self):
        # A valid transparent DDS passes input identity but has no tree silhouette.
        empty = io.BytesIO()
        Image.new('RGBA', (32, 32)).save(empty, format='DDS')
        raw = empty.getvalue()
        key = next(iter(self.index))
        self.index[key]['sha256'] = hashlib.sha256(raw).hexdigest()
        self.write_index()
        (self.input / 'frahit.bfl').write_bytes(bfl([('tre_md1.dds', raw)]))
        self.before['frahit.bfl'] = (self.input / 'frahit.bfl').read_bytes()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        self.assertFalse(list(self.output.parent.glob('.cmr2-patch-*')))


if __name__ == '__main__':
    unittest.main()
