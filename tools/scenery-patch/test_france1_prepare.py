import unittest,struct,tempfile,hashlib,io
from pathlib import Path
from PIL import Image
from unittest.mock import patch
import france1_prepare as f
class Tests(unittest.TestCase):
 def test_source_refusal(self):
  with self.assertRaises(ValueError):f.encode(b'wrong',f.INDEX[0])
 def test_opaque_layout(self):
  image=Image.new('RGBA',(8,8),(20,60,100,255));out=io.BytesIO();image.save(out,format='DDS');raw=out.getvalue();row=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),width=8,height=8,repeat=[True,True])
  data=f.encode(raw,row);self.assertEqual(data[:8],b'OCMRRGBA');self.assertEqual(struct.unpack_from('<III',data,8),(16,16,5));self.assertEqual(len(data),20+sum(n*n*4 for n in [16,8,4,2,1]))
  self.assertEqual(set(tuple(data[i:i+4])for i in range(20,len(data),4)),{(20,60,100,255)})
 def test_missing_sources_no_cache(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);(root/'owned').mkdir()
   with self.assertRaises(ValueError):f.prepare(root/'owned',root/'cache')
   self.assertFalse((root/'cache').exists())
 def test_atomic_reuse_and_tamper(self):
  with tempfile.TemporaryDirectory()as tmp:
   root=Path(tmp);data=root/'owned';data.mkdir();cache=root/'cache';archive=data/'frahit.bfl';archive.write_bytes(b'fixture')
   row=dict(archive='frahit.bfl',member='a.dds',output='a.rgba',sha256='a'*64,output_sha256=hashlib.sha256(b'rgba').hexdigest())
   with patch.object(f,'INDEX',[row]),patch.object(f._export,'members',return_value=[('a.dds',b'input')]),patch.object(f,'encode',return_value=b'rgba'):
    pack=f.prepare(data,cache);self.assertEqual(pack,f.prepare(data,cache));(pack/'a.rgba').write_bytes(b'changed')
    with self.assertRaises(ValueError):f.prepare(data,cache)
    self.assertEqual(archive.read_bytes(),b'fixture')
 def test_overlap(self):
  with tempfile.TemporaryDirectory()as tmp:
   with self.assertRaises(ValueError):f.prepare(Path(tmp),Path(tmp)/'cache')
if __name__=='__main__':unittest.main()
