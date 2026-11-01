import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
from test_convert import bfl

spec = importlib.util.spec_from_file_location('exporter_greece', Path(__file__).with_name('export-greece-trees.py'))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def dds(w, h, payload=b'x'):
    head = b'DDS ' + struct.pack('<II', 124, 0) + struct.pack('<II', h, w)
    return head + payload

def make_archives(tmp, tamper=False):
    """Synthetic grehit/grelot whose identity members hash-match a patched manifest."""
    index = json.loads((m.HERE / 'uprez-textures.json').read_text(encoding='utf-8'))
    rows = [r for r in index['sources'] if r['archive'] in m.ARCHIVES]
    built = {}
    for name in m.ARCHIVES:
        members = []
        for r in rows:
            if r['archive'] != name: continue
            body = b'id-' + r['member'].encode()
            r['sha256'] = hashlib.sha256(body).hexdigest()
            if tamper and r['member'].startswith('bsh'):
                body += b'tampered'  # archive differs; manifest keeps the original hash
            members.append((r['member'], body))
        members.append(('tre_132.dds', dds(64, 128)))
        members.append(('tre_110.tga', struct.pack('<12xHH', 64, 128)))
        members.append(('unrelated.dds', b'private'))
        (tmp / name).write_bytes(bfl(members))
        built[name] = members
    (m.HERE / 'uprez-textures.json').write_text(json.dumps(index), encoding='utf-8')
    return built

class ExportGreeceTreesTests(unittest.TestCase):
    def setUp(self):
        self._orig = (m.HERE / 'uprez-textures.json').read_bytes()
    def tearDown(self):
        (m.HERE / 'uprez-textures.json').write_bytes(self._orig)

    def test_export_verified_pair_read_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            make_archives(data)
            before = {p.name: p.read_bytes() for p in data.iterdir()}
            out = root / 'greece.zip'
            report = m.export(data, out)
            self.assertEqual(len(report['hits']), 10)  # tre_132/tre_110 + tre_s1_a/b/c per archive
            dims = {h['member']: h['dimensions'] for h in report['hits']}
            self.assertEqual(dims['tre_132.dds'], [64, 128])
            self.assertEqual(dims['tre_110.tga'], [64, 128])
            self.assertEqual(dims['tre_s1_a.dds'], None)  # synthetic identity body has no DDS header
            with zipfile.ZipFile(out) as z:
                self.assertNotIn(b'private', b''.join(z.read(n) for n in z.namelist()))
                self.assertEqual(len(z.namelist()), 11)  # 10 assets + manifest
            self.assertEqual(before, {p.name: p.read_bytes() for p in data.iterdir()})
            with self.assertRaises(ValueError): m.export(data, out)
            with self.assertRaises(ValueError): m.export(data, data / 'bad.zip')

    def test_tampered_identity_member_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            make_archives(data, tamper=True)
            with self.assertRaises(ValueError) as ctx:
                m.export(data, root / 'greece.zip')
            self.assertIn('identity member differs', str(ctx.exception))
            self.assertFalse((root / 'greece.zip').exists())

    def test_missing_archive_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            make_archives(data)
            (data / 'grelot.bfl').unlink()
            with self.assertRaises(ValueError) as ctx:
                m.export(data, root / 'greece.zip')
            self.assertIn('Need exactly one grelot.bfl', str(ctx.exception))

    def test_symlink_archive_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            make_archives(data)
            real = (data / 'grehit.bfl').read_bytes(); (data / 'grehit.bfl').unlink()
            (root / 'elsewhere.bfl').write_bytes(real)
            (data / 'grehit.bfl').symlink_to(root / 'elsewhere.bfl')
            with self.assertRaises(ValueError) as ctx:
                m.export(data, root / 'greece.zip')
            self.assertIn('Need exactly one grehit.bfl', str(ctx.exception))

if __name__ == '__main__':
    unittest.main()
