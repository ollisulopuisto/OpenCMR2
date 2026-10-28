import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from test_convert import bfl
spec=importlib.util.spec_from_file_location('exporter',Path(__file__).with_name('export-evidence.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ExportTests(unittest.TestCase):
 def test_scope_nested_and_read_only(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);data=root/'game';data.mkdir()
   raw=bfl([('fonts\\general\\hel_20pt.pcf',b'font'),('tre_bs6.dds',b'tree'),('unrelated.dds',b'private')])
   (data/'test.bfl').write_bytes(bfl([('nested.bfl',raw)]));(data/'tre_40.tga').write_bytes(b'loose')
   before={p.name:p.read_bytes() for p in data.iterdir()};out=root/'evidence.zip';report=m.export(data,out)
   self.assertEqual(report['archives_scanned'],2);self.assertEqual(len(report['hits']),3)
   with zipfile.ZipFile(out) as z:
    self.assertNotIn(b'private',b''.join(z.read(n) for n in z.namelist()))
    self.assertEqual(len(z.namelist()),4)
   self.assertEqual(before,{p.name:p.read_bytes() for p in data.iterdir()})
   with self.assertRaises(ValueError):m.export(data,out)
   with self.assertRaises(ValueError):m.export(data,data/'bad.zip')
 def test_corrupt_archive_reported(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);data=root/'game';data.mkdir();(data/'bad.bfl').write_bytes(b'bad')
   report=m.export(data,root/'evidence.zip');self.assertEqual(len(report['parse_failures']),1)
 def test_symlink_not_followed(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);data=root/'game';data.mkdir();(root/'secret.dds').write_bytes(b'secret')
   (data/'tre_bs6.dds').symlink_to(root/'secret.dds')
   self.assertEqual(m.export(data,root/'evidence.zip')['hits'],[])
