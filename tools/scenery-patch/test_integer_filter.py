import unittest,hashlib
from PIL import Image
from integer_filter import levels
class Tests(unittest.TestCase):
 def test_constant_and_sizes(self):
  for size in [(1,1),(1,8),(8,1),(4,8)]:
   im=Image.new('RGBA',size,(50,100,150,255));chain=levels(im)
   self.assertEqual(chain[0].size,(size[0]*2,size[1]*2));self.assertEqual(chain[-1].size,(1,1))
   for mip in chain:self.assertEqual(set(mip.getdata()),{(50,100,150,255)})
 def test_premultiplied_no_hidden_rgb(self):
  im=Image.new('RGBA',(2,2),(255,0,0,0));im.putpixel((1,1),(0,100,0,255))
  for mip in levels(im):
   for r,g,b,a in mip.getdata():self.assertEqual((r,b),(0,0))
 def test_repeat_seam(self):
  im=Image.new('RGBA',(2,1));im.putdata([(0,0,0,255),(200,200,200,255)])
  self.assertEqual([p[0]for p in levels(im,(True,False))[0].getdata()][:4],[50,50,150,150])
  self.assertEqual([p[0]for p in levels(im)[0].getdata()][:4],[0,50,150,200])
 def test_fixed_digest_and_no_mutation(self):
  im=Image.new('RGBA',(4,4));im.putdata([(i*13%256,i*23%256,i*7%256,0 if i%3 else 255)for i in range(16)]);raw=im.tobytes();dig=hashlib.sha256(b''.join(x.tobytes()for x in levels(im))).hexdigest()
  self.assertEqual(dig,'57a8f84357b57179751a31fc4a960d47e6ca8d3b725db489d8fd677371a0414d');self.assertEqual(raw,im.tobytes())
if __name__=='__main__':unittest.main()
