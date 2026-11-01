import hashlib
import importlib.util
import io
import json
import zipfile
from pathlib import Path
import tempfile
import unittest

import numpy as np
from PIL import Image

spec = importlib.util.spec_from_file_location('preview_trees', Path(__file__).with_name('preview_trees.py'))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def fake_tree(w=64, h=128, sparse=False):
    a = np.zeros((h, w, 4), np.uint8)
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy, r = w / 2, h * 0.35, w * 0.42
    d = np.sqrt((xx - cx) ** 2 + ((yy - cy) * 1.2) ** 2)
    crown = d < r
    if sparse:
        crown &= (np.sin(xx * 0.9) > -0.2) & (np.sin(yy * 1.1) > -0.4)
    trunk = (np.abs(xx - w / 2) < 2.5) & (yy > h * 0.6) & (yy < h * 0.97)
    a[crown, :3] = (40, 90, 30); a[crown, 3] = 255
    a[trunk, :3] = (90, 60, 40); a[trunk, 3] = 255
    buf = io.BytesIO(); Image.fromarray(a).save(buf, format='TGA')
    return buf.getvalue()

def fake_export(path):
    hits, payloads = [], {}
    for stem, sparse in (('tre_132', False), ('tre_110', True)):
        raw = fake_tree(sparse=sparse)
        sha = hashlib.sha256(raw).hexdigest()
        p = 'assets/grehit/%s/%s.tga' % (sha, stem)
        payloads[p] = raw
        hits.append({'archive': 'grehit.bfl', 'member': stem + '.tga', 'sha256': sha,
                     'bytes': len(raw), 'dimensions': [64, 128], 'export_path': p})
    with zipfile.ZipFile(path, 'w') as z:
        z.writestr('manifest.json', json.dumps({'scope': 'synthetic', 'archives': {}, 'hits': hits}))
        for p, r in payloads.items():
            z.writestr(p, r)

class PreviewTreesTests(unittest.TestCase):
    def test_sheet_renders_both_modes(self):
        with tempfile.TemporaryDirectory() as tmp:
            z = Path(tmp) / 'export.zip'; out = Path(tmp) / 'sheet.png'
            fake_export(z)
            m.main([str(z), str(out)])
            sheet = np.array(Image.open(out).convert('RGB'))
            self.assertEqual(sheet.shape[1], m.VIEW_W * len(m.ANGLES))
            self.assertEqual(sheet.shape[0], (m.VIEW_H + 18) * 2 + 4)
            top = sheet[18:18 + m.VIEW_H, 0:m.VIEW_W]          # shell row, 0-degree view
            bot = sheet[18 + (m.VIEW_H + 18):18 + (m.VIEW_H + 18) + m.VIEW_H, 0:m.VIEW_W]
            green_shell = ((top[:, :, 1] > 70) & (top[:, :, 0] < 70)).sum()
            self.assertGreater(green_shell, 3000)               # solid shell canopy covers a big area
            green_cross = ((bot[:, :, 1] > 70) & (bot[:, :, 0] < 70)).sum()
            self.assertGreater(green_cross, 300)                # sparse crossed canopy present...
            self.assertLess(green_cross, green_shell // 2)      # ...but with real alpha holes
            trunk = ((top[:, :, 0] > 70) & (top[:, :, 1] < 80) & (top[:, :, 2] < 70)).sum()
            self.assertGreater(trunk, 50)                       # trunk pixels drawn

    def test_undecodable_candidate_is_reported_not_fatal(self):
        with tempfile.TemporaryDirectory() as tmp:
            z = Path(tmp) / 'export.zip'; out = Path(tmp) / 'sheet.png'
            sha = hashlib.sha256(b'junk').hexdigest()
            p = 'assets/grehit/%s/tre_bad.dds' % sha
            with zipfile.ZipFile(z, 'w') as zz:
                zz.writestr('manifest.json', json.dumps({'scope': 'synthetic', 'archives': {}, 'hits': [
                    {'archive': 'grehit.bfl', 'member': 'tre_bad.dds', 'sha256': sha,
                     'bytes': 4, 'dimensions': None, 'export_path': p}]}))
                zz.writestr(p, b'junk')
            m.main([str(z), str(out)])  # row renders empty with a note, no crash
            self.assertTrue(out.exists())

if __name__ == '__main__':
    unittest.main()
