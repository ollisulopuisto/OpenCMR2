import unittest,tempfile,hashlib,io
from pathlib import Path
from unittest.mock import patch
from PIL import Image
import uprez_prepare as u
class Tests(unittest.TestCase):
 def test_source_refusal(self):
  with self.assertRaises(ValueError):u.encode(b'wrong',u.INDEX[0])
 def test_cutout_refusal(self):
  im=Image.new('RGBA',(8,8),(22,33,44,0));buf=io.BytesIO();im.save(buf,format='DDS');raw=buf.getvalue();row=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),width=8,height=8,repeat=[True,True])
  with self.assertRaises(ValueError):u.encode(raw,row)
 def test_missing_archive_no_write(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);(root/'game').mkdir()
   with self.assertRaises(ValueError):u.prepare(root/'game',root/'cache')
   self.assertFalse((root/'cache').exists())
 def test_atomic_partial_countries_reuse_tamper(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);data=root/'game';data.mkdir();(data/'finhit.bfl').write_bytes(b'fixture')
   row=dict(archive='finhit.bfl',member='ground.dds',output='ground.rgba',sha256='a'*64,output_sha256=hashlib.sha256(b'rgba').hexdigest())
   with patch.object(u,'INDEX',[row]),patch.object(u._export,'members',return_value=[('ground.dds',b'input')]),patch.object(u,'encode',return_value=b'rgba'):
    pack=u.prepare(data,root/'cache');self.assertEqual(pack,u.prepare(data,root/'cache'));(pack/'ground.rgba').write_bytes(b'tamper')
    with self.assertRaises(ValueError):u.prepare(data,root/'cache')
    self.assertEqual((data/'finhit.bfl').read_bytes(),b'fixture')
if __name__=='__main__':unittest.main()
