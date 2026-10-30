import hashlib,json,tempfile,unittest
from pathlib import Path
from PIL import Image
from uprez import run,upscale

class Tests(unittest.TestCase):
 def test_dimensions_and_mode(self):
  self.assertEqual(upscale(Image.new('RGB',(7,13))).size,(14,26))
  self.assertEqual(upscale(Image.new('RGB',(7,13))).mode,'RGBA')
 def test_transparent_hidden_rgb_does_not_bleed(self):
  a=Image.new('RGBA',(4,4),(0,255,0,0));a.putpixel((1,1),(255,0,0,255));b=upscale(a)
  for r,g,blue,alpha in b.getdata():
   if alpha:self.assertEqual((g,blue),(0,0))
 def test_opaque_stays_opaque(self):
  self.assertEqual(upscale(Image.new('RGBA',(4,4),(77,33,11,255))).getchannel('A').getextrema(),(255,255))
 def fixture(self,d):
  p=d/'source.png';Image.new('RGBA',(8,16),(40,90,10,255)).save(p);h=hashlib.sha256(p.read_bytes()).hexdigest();inv=d/'inventory.json';inv.write_text(json.dumps([dict(archive='test.bfl',texture='source.png',path=str(p),sha256=h)]));return p,h,inv
 def test_source_unchanged_and_output_readable(self):
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);p,h,i=self.fixture(d);m=run(i,d/'out');self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(),h);self.assertEqual(m[0]['output_dimensions'],[16,32])
 def test_existing_output_refused(self):
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);p,h,i=self.fixture(d);(d/'out').mkdir()
   with self.assertRaises(FileExistsError):run(i,d/'out')
 def test_bad_source_hash_refused_before_writes(self):
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);p,h,i=self.fixture(d);p.write_bytes(b'changed')
   with self.assertRaises(ValueError):run(i,d/'out')
   self.assertFalse((d/'out').exists())
 def test_output_path_escape_refused(self):
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);p,h,i=self.fixture(d);rows=json.loads(i.read_text());rows[0]['archive']='../outside';i.write_text(json.dumps(rows))
   with self.assertRaises(ValueError):run(i,d/'out')
   self.assertFalse((d/'out').exists())

if __name__=='__main__':unittest.main()
