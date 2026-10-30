import hashlib, json, os, struct, tempfile, unittest
from pathlib import Path
import importlib.util
spec = importlib.util.spec_from_file_location('stage_inventory', Path(__file__).with_name('stage_inventory.py'))
si = importlib.util.module_from_spec(spec); spec.loader.exec_module(si)

def archive(members):
    data = b''; entries = []
    for name, payload in members:
        entries.append((len(payload), len(data), name)); data += payload
    directory = b''
    for size, offset, name in entries:
        raw = name.encode('ascii')
        directory += struct.pack('<III', size, offset, len(raw)) + raw + b'\0' * ((len(raw) + 3) // 4 * 4 - len(raw))
    return data + directory + struct.pack('<I', len(data))

class Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.d = Path(self.tmp.name)
        self.game = self.d / 'game'; (self.game / 'sub').mkdir(parents=True)
        self.inner = archive([('spc_x1.dds', b'spectator-bytes')])
        self.raw = archive([('tre_a1.dds', b'tree-bytes'), ('bld_b1a.dds', b'building'), ('stage.cin', b'cin'), ('inner.bfl', self.inner)])
        (self.game / 'sub' / 'FINHIT.BFL').write_bytes(self.raw)
    def tearDown(self): self.tmp.cleanup()
    def test_lists_members_with_classes_hashes_and_nested(self):
        doc = si.inventory(self.game, self.d / 'inv.json')
        by = {a['origin']: a for a in doc['archives']}
        top = by['sub/FINHIT.BFL']
        self.assertEqual(top['class_counts'], {'archive': 1, 'stage-data': 1, 'texture': 1, 'tree': 1})
        tree = next(m for m in top['members'] if m['name'] == 'tre_a1.dds')
        self.assertEqual(tree['sha256'], hashlib.sha256(b'tree-bytes').hexdigest())
        self.assertEqual(by['sub/FINHIT.BFL!inner.bfl']['class_counts'], {'spectator': 1})
    def test_no_asset_bytes_in_output_and_originals_unchanged(self):
        before = (self.game / 'sub' / 'FINHIT.BFL').read_bytes()
        si.inventory(self.game, self.d / 'inv.json')
        text = (self.d / 'inv.json').read_text()
        self.assertNotIn('tree-bytes', text)
        self.assertEqual((self.game / 'sub' / 'FINHIT.BFL').read_bytes(), before)
    def test_existing_output_and_output_inside_game_refused(self):
        (self.d / 'inv.json').write_text('x')
        with self.assertRaises(ValueError): si.inventory(self.game, self.d / 'inv.json')
        with self.assertRaises(ValueError): si.inventory(self.game, self.game / 'inv.json')
        self.assertEqual((self.d / 'inv.json').read_text(), 'x')
    def test_corrupt_archive_reported_not_fatal(self):
        (self.game / 'bad.bfl').write_bytes(b'not an archive at all')
        doc = si.inventory(self.game, self.d / 'inv.json')
        self.assertEqual([f['origin'] for f in doc['parse_failures']], ['bad.bfl'])
        self.assertTrue(any(a['origin'] == 'sub/FINHIT.BFL' for a in doc['archives']))

if __name__ == '__main__': unittest.main()
