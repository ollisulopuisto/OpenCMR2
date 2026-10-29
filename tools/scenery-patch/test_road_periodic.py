import unittest
from PIL import Image,ImageChops
import road_periodic as p
class Tests(unittest.TestCase):
 def test_explicit_policy_and_opaque_required(self):
  for policy in (None,(False,False),'repeat'):
   with self.assertRaises(ValueError):p.resize(Image.new('RGB',(8,8)),(16,16),policy)
  with self.assertRaises(ValueError):p.resize(Image.new('RGBA',(8,8)),(16,16),(True,True))
 def test_constant_and_small_mips(self):
  im=Image.new('RGB',(9,5),(30,70,110));levels=p.mip_chain(im,(True,True))
  self.assertEqual([i.size for i in levels],[(9,5),(4,2),(2,1),(1,1)])
  for level in levels:self.assertEqual(set(level.getdata()),{(30,70,110,255)})
 def test_repeat_translation_equivariance(self):
  im=Image.new('RGB',(8,8));im.putdata([(x*31,y*29,(x+y)*13)for y in range(8)for x in range(8)])
  for policy in ((True,False),(False,True),(True,True)):
   dx,dy=(2 if policy[0]else 0),(2 if policy[1]else 0)
   actual=p.resize(ImageChops.offset(im,dx,dy),(16,16),policy)
   expected=ImageChops.offset(p.resize(im,(16,16),policy),dx*2,dy*2)
   self.assertIsNone(ImageChops.difference(actual.convert("RGB"),expected.convert("RGB")).getbbox())
 def test_repeat_matches_large_tiled_reference(self):
  im=Image.new('RGB',(8,8));im.putdata([(x*31,y*29,(x+y)*13)for y in range(8)for x in range(8)])
  for n in (1,2,4,16):
   tiled=Image.new('RGB',(72,72))
   for y in range(9):
    for x in range(9):tiled.paste(im,(x*8,y*8))
   ref=tiled.resize((n*9,n*9),Image.Resampling.LANCZOS).crop((n*4,n*4,n*5,n*5)).convert('RGBA')
   self.assertIsNone(ImageChops.difference(ref.convert("RGB"),p.resize(im,(n,n),(True,True)).convert("RGB")).getbbox())
if __name__=='__main__':unittest.main()
